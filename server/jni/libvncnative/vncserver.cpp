#include <android/log.h>
#include <GLES2/gl2.h>

#include "log.h"
#include "vncserver.h"

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
		rfbShutdownServer(m_RfbScreenInfoPtr, true);
		m_RfbScreenInfoPtr = nullptr;
	}
	releaseBuffers();
	m_Width = 0;
	m_Height = 0;
	m_PixelFormat = 0;
	m_FrameAvailable = false;
	m_EventInjector.reset();
	m_Brightness.reset();
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

bool VncServer::allocateBuffers(int width, int height, int pixelFormat, int fullFrameUpdate)
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
	m_BufferManager.reset(new BufferManager());
	return m_BufferManager->allocate(fullFrameUpdate ? BufferManager::TRIPLE : BufferManager::WITH_COMPARE,
		width, height, format);
}

void VncServer::releaseBuffers()
{
	m_BufferManager.reset();
	m_VncBuffer = nullptr;
	m_GlBuffer = nullptr;
	m_CmpBuffer = nullptr;
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
	m_BufferManager->getConsumer(m_VncBuffer, m_CmpBuffer);
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
	m_GlBuffer = m_BufferManager->getProducer();
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
	postEventToUI(CLIENT_DISCONNECTED, "Client " + std::string(cl->host) + " disconnected");
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

int VncServer::startServer(bool root, int width, int height, int pixelFormat, bool fullFrameUpdate)
{
	m_Width = width;
	m_Height = height;
	m_PixelFormat = pixelFormat;
	m_Rooted = root;
	LOGI("Starting VNC server (%dx%d), %s, using %sfull screen updates", m_Width, m_Height, m_PixelFormat == GL_RGB565 ? "RGB565" : "RGBA",
		fullFrameUpdate ? "" : "no ");

	if (!allocateBuffers(m_Width, m_Height, m_PixelFormat, fullFrameUpdate))
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
		if (m_EventInjector->initialize(m_Width, m_Height, m_Rotation))
		{
			m_RfbScreenInfoPtr->kbdAddEvent = handleKeyEventClb;
			m_RfbScreenInfoPtr->ptrAddEvent = handlePointerEventClb;
		}
		else
		{
			LOGE("Failed to initialize event injector");
		}
		m_Brightness.reset(new BrightnessHelper());
		if (!m_Brightness->initialize(m_PackagePath))
		{
			LOGE("Failed to initialize brightness module");
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

void VncServer::onRotation(int rotation)
{
	if (m_EventInjector)
	{
		m_EventInjector->onRotation(rotation);
	}
	m_Rotation = rotation;
}

void VncServer::setBrightness(int level)
{
	if (m_Brightness)
	{
		m_Brightness->setBrightness(level);
	}
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

void VncServer::setPackagePath(const char *packagePath)
{
	m_PackagePath = packagePath;
}

void VncServer::compare(int width, int shift, uint32_t *buffer0, uint32_t *buffer1)
{
	const int DEFAULT_MIN = 10000;
	const int DEFAULT_MAX = -1;
	int maxX = DEFAULT_MAX, maxY = DEFAULT_MAX, minX = DEFAULT_MIN, minY = DEFAULT_MIN;

	int j = 0, i = 0;
	uint32_t *bufEnd = buffer0 + m_Height * width;
	/* find first pixel which differs */
	while (buffer0 < bufEnd)
	{
		if (*buffer0++ != *buffer1++)
		{
			/* buffers have already advanced their positions, so update counters as well */
			minX = maxX = i++;
			minY = maxY = j++;
			if (++i >= width)
			{
				i = 0;
				j++;
			}
			break;
		}
		if (++i >= width)
		{
			i = 0;
			j++;
		}
	};
	/* minY found */
	while (buffer0 < bufEnd)
	{
		if (*buffer0++ != *buffer1++)
		{
			if (i <= minX)
			{
				/* can skip delta, because it is already known to be in dirty region */
				minX = i;
				int delta = maxX - minX;
				buffer0 += delta;
				buffer1 += delta;
				i += delta;
				if (delta == width - 1)
				{
					/* minX == 0, maxX == width - now find maxY */
					i = 0;
					j++;
					while (buffer0 < bufEnd)
					{
						if (*buffer0++ != *buffer1++)
						{
							maxY = j;
							int delta = width - i - 1;
							buffer0 += delta;
							buffer1 += delta;
							i += delta;
						}
						if (++i >= width)
						{
							i = 0;
							j++;
						}
					};
					break;
				}
			}
			if (i > maxX)
			{
				maxX = i;
			}
			if (j > maxY)
			{
				maxY = j;
			}
		}
		if (++i >= width)
		{
			i = 0;
			j++;
		}
	};
	/* first pixel which differ will set all min/max, so check any of those */
	if (minX != DEFAULT_MIN)
	{
		if (minX)
		{
			minX--;
		}
		if (minY)
		{
			minY--;
		}
		if (maxX < width)
		{
			maxX++;
		}
		if (maxY < m_Height)
		{
			maxY++;
		}
		if (shift)
		{
			minX <<= shift;
			maxX <<= shift;
		}
		rfbMarkRectAsModified(m_RfbScreenInfoPtr, minX, minY, maxX, maxY);
	}
}

void VncServer::worker()
{
	while ((!m_Terminated) && rfbIsActive(m_RfbScreenInfoPtr))
	{
		rfbProcessEvents(m_RfbScreenInfoPtr, 5000);
		bool update = false;
		{
			std::lock_guard<std::mutex> lock(m_FrameAvailableLock);
			if (m_FrameAvailable)
			{
				m_FrameAvailable = false;
				update = true;
				setVncFramebuffer();
			}
		}
		if (!update)
		{
			continue;
		}
		update = false;
		if (m_CmpBuffer)
		{
			unsigned char *vncbuf;
			if (m_CmpBuffer->lock(&vncbuf) != 0)
			{
					LOGE("Failed to lock buffer");
			}
			if (m_PixelFormat == GL_RGB565)
			{
				compare(m_Width / 2, 1, reinterpret_cast<uint32_t *>(vncbuf),
					reinterpret_cast<uint32_t *>(m_RfbScreenInfoPtr->frameBuffer));
			}
			else if (m_PixelFormat == GL_RGBA)
			{
				compare(m_Width, 0, reinterpret_cast<uint32_t *>(vncbuf),
					reinterpret_cast<uint32_t *>(m_RfbScreenInfoPtr->frameBuffer));
			}
			m_CmpBuffer->unlock();
		}
		else
		{
			rfbMarkRectAsModified(m_RfbScreenInfoPtr, 0, 0, m_Width, m_Height);
		}
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
	if (m_JavaVM)
	{
		m_JavaVM->DetachCurrentThread();
	}
}
