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

void VncServer::allocateBuffers(int width, int height, int pixelFormat)
{
	for (size_t i = 0; i < m_GraphicBuffer.size(); i++)
	{
		/* allocate buffer */
		m_GraphicBuffer[i].reset(new AndroidGraphicBuffer(width, height, pixelFormat));
		/* add to the buffer queue */
		m_BufferQueue.add(i, m_GraphicBuffer[i].get());
	}
}

void VncServer::setVncFramebuffer()
{
	//m_RfbScreenInfoPtr->frameBuffer =(char *)vncbuf;
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
				3,  2);
			scr->serverFormat.redShift = 11;
			scr->serverFormat.greenShift = 5;
			scr->serverFormat.blueShift = 0;
			scr->serverFormat.bitsPerPixel = 16;
			scr->serverFormat.trueColour = false;
			break;
		}
		case GL_RGBA:
		{
			scr = rfbGetScreen(&argc, nullptr, m_Width , m_Height, 0 /* not used */ ,
				3,  4);
			scr->serverFormat.redShift = 16;
			scr->serverFormat.greenShift = 8;
			scr->serverFormat.blueShift = 0;
			scr->serverFormat.bitsPerPixel = 32;
			scr->serverFormat.trueColour = true;
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
	LOGD("Client disconnected");
	postEventToUI(CLIENT_DISCONNECTED, "Client disconnected");
}

rfbNewClientAction clientHookClb(rfbClientPtr cl)
{
	cl->clientGoneHook=(ClientGoneHookPtr)clientGoneClb;
	return VncServer::getInstance().clientHook(cl);
}

rfbNewClientAction VncServer::clientHook(rfbClientPtr cl)
{
	LOGD("Client connected");
	postEventToUI(CLIENT_CONNECTED, "Client connected");
	return RFB_CLIENT_ACCEPT;
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

int VncServer::startServer(int width, int height, int pixelFormat)
{
	m_Width = width;
	m_Height = height;
	m_PixelFormat = pixelFormat;
	LOGI("Starting VNC server (%dx%d), %s", m_Width, m_Height, m_PixelFormat == GL_RGB565 ? "RGB565" : "RGBA");
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

	rfbLogEnable(true);
	rfbLog = rfbDefaultLog;
	rfbErr = rfbDefaultLog;
	rfbInitServer(m_RfbScreenInfoPtr);

	m_Terminated = false;
	m_WorkerThread = std::thread(&VncServer::worker, this);
	postEventToUI(SERVER_STARTED, "VNC server started");
	return 0;
}

void VncServer::worker()
{
	while (!m_Terminated)
	{
		rfbRunEventLoop(m_RfbScreenInfoPtr, 40000, false);
	}
}
