#include <android/log.h>

#include "vncserver.h"

extern "C"
{
	#define MODULE_NAME "vncserver"

	#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO,  MODULE_NAME, __VA_ARGS__))
	#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, MODULE_NAME, __VA_ARGS__))
	#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, MODULE_NAME, __VA_ARGS__))
}

VncServer::~VncServer()
{
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
		jvalue jpar[2];
		jpar[0].i = what;
		jpar[1].l = env->NewStringUTF(text.c_str());
		env->CallVoidMethodA(m_Object, m_NotificationClb, jpar);
	}
	else
	{
		LOGE("onNotification method ID was not set");
	}
}
