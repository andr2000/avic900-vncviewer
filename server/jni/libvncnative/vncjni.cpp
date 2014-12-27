#include <android/log.h>
#include <GLES2/gl2.h>
#include <jni.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>

#include "AndroidGraphicBuffer.h"
#include "log.h"
#include "vncserver.h"

extern "C"
{
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

	void Java_com_a2k_vncserver_VncJni_init(JNIEnv *env, jobject thiz,
		jstring packagePath)
	{
		const char *nativePath = env->GetStringUTFChars(packagePath, JNI_FALSE);
		jclass clazz = env->GetObjectClass(thiz);
		VncServer::getInstance().setupNotificationClb(env,
			(jobject)(env->NewGlobalRef(thiz)),
			(jclass)(env->NewGlobalRef(clazz)));
		VncServer::getInstance().setPackagePath(nativePath);
		env->ReleaseStringUTFChars(packagePath, nativePath);
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
		jboolean root, jint width, jint height, jint pixelFormat, jboolean fullFrameUpdate)
	{
		return VncServer::getInstance().startServer(root, width, height, pixelFormat, fullFrameUpdate);
	}

	JNIEXPORT jint JNICALL Java_com_a2k_vncserver_VncJni_stopServer(JNIEnv *env, jobject obj)
	{
		return VncServer::getInstance().stopServer();
	}

	JNIEXPORT void JNICALL Java_com_a2k_vncserver_VncJni_onRotation(JNIEnv *env, jobject obj,
		jint rotation)
	{
		VncServer::getInstance().onRotation(rotation);
	}

	JNIEXPORT void JNICALL Java_com_a2k_vncserver_VncJni_setBrightness(JNIEnv *env, jobject obj,
		int level)
	{
		VncServer::getInstance().setBrightness(level);
	}
}
