#ifndef LIBVNCSERVER_VNCSERVER_H_
#define LIBVNCSERVER_VNCSERVER_H_

#include <jni.h>
#include <string>

#include "rfb/rfb.h"

class VncServer
{
public:
	static VncServer &getInstance()
	{
		static VncServer m_Instance;
		return m_Instance;
	}

	virtual ~VncServer();
	void setJavaVM(JavaVM *javaVM);
	void setupNotificationClb(JNIEnv *env, jobject jObject, jclass jClass);

	const std::string getVersion();

	void postEventToUI(int what, std::string text);

private:
	JavaVM *m_JavaVM;
	jobject m_Object;
	jclass m_Class;
	jmethodID m_NotificationClb;
};

#endif /* LIBVNCSERVER_VNCSERVER_H_ */
