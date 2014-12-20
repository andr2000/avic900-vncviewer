#include <android/log.h>
#include <GLES2/gl2.h>

#include "vncserver.h"

extern "C"
{
	#define MODULE_NAME "vncserver"

	#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO,  MODULE_NAME, __VA_ARGS__))
	#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, MODULE_NAME, __VA_ARGS__))
	#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, MODULE_NAME, __VA_ARGS__))
}

VncServer::VncServer() = default;

VncServer::~VncServer()
{
	cleanup();
}

void VncServer::cleanup()
{
	m_Terminated = true;
	if (m_WorkerThread.joinable())
	{
		m_WorkerThread.join();
	}
	if (m_RfbScreenInfoPtr)
	{
		rfbScreenCleanup(m_RfbScreenInfoPtr);
		rfbShutdownServer(m_RfbScreenInfoPtr, true);
		m_RfbScreenInfoPtr = nullptr;
	}
	releaseBuffers();
	m_Width = 0;
	m_Height = 0;
	m_PixelFormat = 0;
	m_FrameAvailable = false;
	m_EventInjector.reset();
}

void VncServer::setJavaVM(JavaVM *javaVM)
{
	m_JavaVM = javaVM;
}

void VncServer::setupNotificationClb(JNIEnv *env, jobject jObject, jclass jClass)
{
	m_Object = jObject;
	m_Class = jClass;
	m_NotificationClb = env->GetMethodID(m_Class, "onNotification", "(ILjava/lang/String;)V");
	if (!m_NotificationClb)
	{
		LOGE("Failed to get method ID for onNotification");
	}
}

const std::string VncServer::getVersion()
{
	return "libvncserver " LIBVNCSERVER_VERSION;
}

void VncServer::postEventToUI(int what, std::string text)
{
	LOGD("postEventToUI what = %d, text = %s", what, text.c_str());
	if (m_NotificationClb)
	{
		JNIEnv *env;
		m_JavaVM->GetEnv((void **)&env, JNI_VERSION_1_4);
		m_JavaVM->AttachCurrentThread(&env, 0);
		jvalue jpar[2];
		jpar[0].i = what;
		jpar[1].l = env->NewStringUTF(text.c_str());
		env->CallVoidMethodA(m_Object, m_NotificationClb, jpar);
		env->DeleteLocalRef(jpar[1].l);
	}
	else
	{
		LOGE("onNotification method ID was not set");
	}
}

bool VncServer::allocateBuffers(int width, int height, int pixelFormat)
{
	int format = 0;
	switch (pixelFormat)
	{
		case GL_RGBA:
		{
			format = AndroidGraphicBuffer::HAL_PIXEL_FORMAT_RGBA_8888;
			break;
		}
		case GL_RGB565:
		{
			format = AndroidGraphicBuffer::HAL_PIXEL_FORMAT_RGB_565;
			break;
		}
		default:
		{
			LOGE("Unsupported pixel format");
			return false;
		}
	}
	for (size_t i = 0; i < m_GraphicBuffer.size(); i++)
	{
		/* allocate buffer */
		m_GraphicBuffer[i].reset(new AndroidGraphicBuffer(width, height, format));
		if (!m_GraphicBuffer[i]->allocate())
		{
			return false;
		}
		/* add to the buffer queue */
		m_BufferQueue.add(i, m_GraphicBuffer[i].get());
	}
	return true;
}

void VncServer::releaseBuffers()
{
	for (size_t i = 0; i < m_GraphicBuffer.size(); i++)
	{
		m_GraphicBuffer[i].reset();
	}
	m_BufferQueue.release();
	m_VncBuffer = nullptr;
	m_GlBuffer = nullptr;
}

void VncServer::setVncFramebuffer()
{
	if (m_VncBuffer)
	{
		if (m_VncBuffer->unlock() != 0)
		{
			LOGE("Failed to unlock buffer");
		}
	}
	m_VncBuffer = m_BufferQueue.getConsumer();
	if (m_VncBuffer)
	{
		unsigned char *vncbuf;
		if (m_VncBuffer->lock(&vncbuf) != 0)
		{
			LOGE("Failed to lock buffer");
		}
		m_RfbScreenInfoPtr->frameBuffer = reinterpret_cast<char *>(vncbuf);
	}
	else
	{
		LOGE("Failed to get new framebuffer");
	}
}

void VncServer::bindNextProducerBuffer()
{
	m_GlBuffer = m_BufferQueue.getProducer();
	if (m_GlBuffer)
	{
		if (!m_GlBuffer->bind())
		{
			LOGE("Failed to bind graphics buffer");
		}
	}
}

rfbScreenInfoPtr VncServer::getRfbScreenInfoPtr()
{
	rfbScreenInfoPtr scr = nullptr;
	int argc = 0;
	switch (m_PixelFormat)
	{
		case GL_RGB565:
		{
			scr = rfbGetScreen(&argc, nullptr, m_Width , m_Height, 0 /* not used */ ,
				3, 2);
			scr->serverFormat.redShift = 11;
			scr->serverFormat.greenShift = 5;
			scr->serverFormat.blueShift = 0;
			scr->serverFormat.bitsPerPixel = 16;
			scr->serverFormat.trueColour = true;
			scr->serverFormat.redMax = 31;
			scr->serverFormat.greenMax = 63;
			scr->serverFormat.blueMax = 31;
			break;
		}
		case GL_RGBA:
		{
			scr = rfbGetScreen(&argc, nullptr, m_Width , m_Height, 0 /* not used */ ,
				3, 4);
			scr->serverFormat.redShift = 0;
			scr->serverFormat.greenShift = 8;
			scr->serverFormat.blueShift = 16;
			scr->serverFormat.bitsPerPixel = 32;
			scr->serverFormat.trueColour = true;
			scr->serverFormat.redMax = 255;
			scr->serverFormat.greenMax = 255;
			scr->serverFormat.blueMax = 255;
			break;
		}
	}
	return scr;
}

void clientGoneClb(rfbClientPtr cl)
{
	return VncServer::getInstance().clientGone(cl);
}

void VncServer::clientGone(rfbClientPtr cl)
{
	LOGD("Client disconnected: %s", cl->host);
	postEventToUI(CLIENT_DISCONNECTED, "Client" + std::string(cl->host) + " disconnected");
}

rfbNewClientAction clientHookClb(rfbClientPtr cl)
{
	cl->clientGoneHook=(ClientGoneHookPtr)clientGoneClb;
	return VncServer::getInstance().clientHook(cl);
}

rfbNewClientAction VncServer::clientHook(rfbClientPtr cl)
{
	LOGD("Client connected: %s", cl->host);
	postEventToUI(CLIENT_CONNECTED, "Client " + std::string(cl->host) + " connected");
	return RFB_CLIENT_ACCEPT;
}

void handlePointerEventClb(int buttonMask, int x, int y, rfbClientPtr cl)
{
	VncServer::getInstance().handlePointerEvent(buttonMask, x, y, cl);
}

void VncServer::handlePointerEvent(int buttonMask, int x, int y, rfbClientPtr cl)
{
	if (m_EventInjector)
	{
		m_EventInjector->handlePointerEvent(buttonMask, x, y, cl);
	}
}

void handleKeyEventClb(rfbBool down, rfbKeySym key, rfbClientPtr cl)
{
	VncServer::getInstance().handleKeyEvent(down, key, cl);
}

void VncServer::handleKeyEvent(rfbBool down, rfbKeySym key, rfbClientPtr cl)
{
	if (m_EventInjector)
	{
		m_EventInjector->handleKeyEvent(down, key, cl);
	}
}

void rfbDefaultLog(const char *format, ...)
{
	va_list args;
	char buf[256];
	va_start(args, format);
	vsprintf(buf, format, args);
	LOGD("%s", buf);
	va_end(args);
}

int VncServer::startServer(bool root, int width, int height, int pixelFormat)
{
	m_Width = width;
	m_Height = height;
	m_PixelFormat = pixelFormat;
	m_Rooted = root;
	LOGI("Starting VNC server (%dx%d), %s", m_Width, m_Height, m_PixelFormat == GL_RGB565 ? "RGB565" : "RGBA");

	if (!allocateBuffers(m_Width, m_Height, m_PixelFormat))
	{
		return -1;
	}

	m_RfbScreenInfoPtr = getRfbScreenInfoPtr();
	if (m_RfbScreenInfoPtr == nullptr)
	{
		LOGE("Failed to get RFB screen");
		return -1;
	}
	m_RfbScreenInfoPtr->desktopName = DESKTOP_NAME;
	m_RfbScreenInfoPtr->newClientHook = (rfbNewClientHookPtr)clientHookClb;

	m_RfbScreenInfoPtr->handleEventsEagerly = true;
	m_RfbScreenInfoPtr->deferUpdateTime = 0;
	m_RfbScreenInfoPtr->port = VNC_PORT;
	m_RfbScreenInfoPtr->alwaysShared = true;
	m_RfbScreenInfoPtr->neverShared = false;

	setVncFramebuffer();

	m_RfbScreenInfoPtr->paddedWidthInBytes = m_VncBuffer->getStride() *
		m_RfbScreenInfoPtr->serverFormat.bitsPerPixel / CHAR_BIT;

	rfbLogEnable(true);
	rfbLog = rfbDefaultLog;
	rfbErr = rfbDefaultLog;
	rfbInitServer(m_RfbScreenInfoPtr);

	if (m_Rooted)
	{
		m_EventInjector.reset(new EventInjector());
		if (m_EventInjector->initialize(m_Width, m_Height))
		{
			m_RfbScreenInfoPtr->kbdAddEvent = handleKeyEventClb;
			m_RfbScreenInfoPtr->ptrAddEvent = handlePointerEventClb;
		}
		else
		{
			LOGE("Failed to initialize event injector");
		}
	}
	m_Terminated = false;
	m_WorkerThread = std::thread(&VncServer::worker, this);
	postEventToUI(SERVER_STARTED, "VNC server started");
	return 0;
}

int VncServer::stopServer()
{
	cleanup();
	postEventToUI(SERVER_STOPPED, "VNC server stopped");
	return 0;
}

void VncServer::frameAvailable()
{
	std::lock_guard<std::mutex> lock(m_FrameAvailableLock);
	m_FrameAvailable = true;
}

void VncServer::dumpFrame(char *buffer)
{
	const char *fName = "/sdcard/framebuffer.data";
	FILE *f = fopen(fName, "w+b");
	if (f)
	{
		int bytesPerPixel = m_PixelFormat == GL_RGBA ? 4 : 2;
		fwrite(buffer, 1, m_Width * m_Height * bytesPerPixel, f);
		fclose(f);
		LOGD("Frame saved at %s", fName);
	}
	else
	{
		LOGE("Failed to save frame at %s", fName);
	}
}

void VncServer::worker()
{
	while ((!m_Terminated) && rfbIsActive(m_RfbScreenInfoPtr))
	{
		rfbProcessEvents(m_RfbScreenInfoPtr, 5000);
		{
			std::lock_guard<std::mutex> lock(m_FrameAvailableLock);
			if (m_FrameAvailable)
			{
				m_FrameAvailable = false;
				setVncFramebuffer();
				rfbMarkRectAsModified(m_RfbScreenInfoPtr, 0, 0, m_RfbScreenInfoPtr->width, m_RfbScreenInfoPtr->height);
				if (DUMP_ENABLED)
				{
					static int counter = 20;
					if (--counter == 0)
					{
						counter = 20;
						dumpFrame(m_RfbScreenInfoPtr->frameBuffer);
					}
				}
			}
		}
	}
	if (m_JavaVM)
	{
		m_JavaVM->DetachCurrentThread();
	}
}
