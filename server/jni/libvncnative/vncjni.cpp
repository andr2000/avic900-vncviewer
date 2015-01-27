#include <android/log.h>
#include <GLES2/gl2.h>
#include <jni.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

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

	JNIEXPORT jint JNICALL Java_com_a2k_vncserver_VncJni_mkfifo(JNIEnv *env, jobject, jstring path)
	{
		const char *cpath = env->GetStringUTFChars(path, NULL);
		struct stat buf;
		int res = 0;
		if (stat(cpath, &buf) < 0)
		{
			if (mkfifo(cpath, S_IRWXU) < 0 )
			{
				LOGD("Cannot create a pipe");
				res = -1;
			}
			else
			{
				LOGD("The pipe was created");
			}
		}
		else
		{
			LOGD("Pipe already exists");
		}
		env->ReleaseStringUTFChars(path, cpath);
		return res;
	}
}
