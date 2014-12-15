#ifndef LIBVNCSERVER_VNCSERVER_H_
#define LIBVNCSERVER_VNCSERVER_H_

#include <array>
#include <jni.h>
#include <memory>
#include <mutex>
#include <string>

#include "AndroidGraphicBuffer.h"
#include "rfb/rfb.h"
#include "triplebuffer.h"

class VncServer
{
public:
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

private:
	JavaVM *m_JavaVM;
	jobject m_Object;
	jclass m_Class;
	jmethodID m_NotificationClb;

	VncServer();

	TripleBuffer<AndroidGraphicBuffer *> m_BufferQueue;
	std::array<std::unique_ptr<AndroidGraphicBuffer>, 3> m_GraphicBuffer;
	void allocateBuffers(int width, int height, int pixelFormat);
};

#endif /* LIBVNCSERVER_VNCSERVER_H_ */
