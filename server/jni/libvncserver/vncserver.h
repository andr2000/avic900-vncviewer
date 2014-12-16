#ifndef LIBVNCSERVER_VNCSERVER_H_
#define LIBVNCSERVER_VNCSERVER_H_

#include <atomic>
#include <array>
#include <jni.h>
#include <memory>
#include <mutex>
#include <thread>
#include <string>

#include "AndroidGraphicBuffer.h"
#include "rfb/rfb.h"
#include "triplebuffer.h"

class VncServer
{
public:
	enum UI_EVENT
	{
		SERVER_STARTED,
		CLIENT_CONNECTED,
		CLIENT_DISCONNECTED,
	};

	static VncServer &getInstance()
	{
		static VncServer m_Instance;
		return m_Instance;
	}

	~VncServer();

	void setJavaVM(JavaVM *javaVM);
	void setupNotificationClb(JNIEnv *env, jobject jObject, jclass jClass);

	const std::string getVersion();

	void postEventToUI(int what, std::string text);

	int startServer(int width, int height, int pixelFormat);
	void bindNextProducerBuffer();
	void frameAvailable();

	rfbNewClientAction clientHook(rfbClientPtr cl);
	void clientGone(rfbClientPtr cl);

private:
	const char *DESKTOP_NAME = "Android";
	const int VNC_PORT = 5901;
	JavaVM *m_JavaVM;
	jobject m_Object;
	jclass m_Class;
	jmethodID m_NotificationClb;

	int m_Width;
	int m_Height;
	int m_PixelFormat;

	std::thread m_WorkerThread;
	std::atomic<bool> m_Terminated { true };
	void worker();

	VncServer();

	void cleanup();

	AndroidGraphicBuffer *m_VncBuffer { nullptr };
	AndroidGraphicBuffer *m_GlBuffer { nullptr };
	TripleBuffer<AndroidGraphicBuffer *> m_BufferQueue;
	std::array<std::unique_ptr<AndroidGraphicBuffer>, 3> m_GraphicBuffer;
	bool allocateBuffers(int width, int height, int pixelFormat);

	rfbScreenInfoPtr m_RfbScreenInfoPtr;
	rfbScreenInfoPtr getRfbScreenInfoPtr();
	void setVncFramebuffer();
};

#endif /* LIBVNCSERVER_VNCSERVER_H_ */
