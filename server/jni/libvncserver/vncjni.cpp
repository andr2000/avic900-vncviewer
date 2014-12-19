#include <android/log.h>
#include <GLES2/gl2.h>
#include <jni.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>

#include "AndroidGraphicBuffer.h"
#include "vncserver.h"

#define MODULE_NAME "vncjni"

int gBytesPerPixel;
uint8_t gPixels[1024 * 1024 * 4];

extern "C"
{
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO,  MODULE_NAME, __VA_ARGS__))
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, MODULE_NAME, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, MODULE_NAME, __VA_ARGS__))

	jint JNI_OnLoad(JavaVM *vm, void *reserved)
	{
		JNIEnv *env;
		VncServer::getInstance().setJavaVM(vm);
		if (vm->GetEnv((void **)&env, JNI_VERSION_1_4) != JNI_OK)
		{
			LOGE("Failed to get the environment using GetEnv()");
			return -1;
		}
		return JNI_VERSION_1_4;
	}

	void Java_com_a2k_vncserver_VncJni_init(JNIEnv *env, jobject thiz)
	{
		jclass clazz = env->GetObjectClass(thiz);
		VncServer::getInstance().setupNotificationClb(env,
			(jobject)(env->NewGlobalRef(thiz)),
			(jclass)(env->NewGlobalRef(clazz)));
	}

	JNIEXPORT jstring JNICALL Java_com_a2k_vncserver_VncJni_protoGetVersion(JNIEnv *env, jobject obj)
	{
		return env->NewStringUTF(VncServer::getInstance().getVersion().c_str());
	}

	long long currentTimeInMilliseconds()
	{
		struct timeval tv;
		gettimeofday(&tv, NULL);
		return ((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
	}

	JNIEXPORT void JNICALL Java_com_a2k_vncserver_VncJni_bindNextGraphicBuffer(JNIEnv *env, jobject obj)
	{
		VncServer::getInstance().bindNextProducerBuffer();
	}

	JNIEXPORT void JNICALL Java_com_a2k_vncserver_VncJni_frameAvailable(JNIEnv *env, jobject obj)
	{
		VncServer::getInstance().frameAvailable();
	}

	JNIEXPORT jint JNICALL Java_com_a2k_vncserver_VncJni_startServer(JNIEnv *env, jobject obj,
		jboolean root, jint width, jint height, jint pixelFormat)
	{
		return VncServer::getInstance().startServer(root, width, height, pixelFormat);
	}

	JNIEXPORT jint JNICALL Java_com_a2k_vncserver_VncJni_stopServer(JNIEnv *env, jobject obj)
	{
		return VncServer::getInstance().stopServer();
	}
}
