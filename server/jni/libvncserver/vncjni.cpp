#include <android/log.h>
#include <jni.h>
#include <string.h>
#include <sys/time.h>

#include "AndroidGraphicBuffer.h"
#include "rfb/rfbconfig.h"

#define MODULE_NAME "vncjni"

uint8_t gPixels[1024 * 1024 * 4];

extern "C"
{
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO,  MODULE_NAME, __VA_ARGS__))
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, MODULE_NAME, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, MODULE_NAME, __VA_ARGS__))

	JNIEXPORT jstring JNICALL Java_com_a2k_vncserver_VncJni_protoGetVersion(JNIEnv *env, jobject obj)
	{
		return env->NewStringUTF("libvncserver " LIBVNCSERVER_VERSION);
	}

	long long currentTimeInMilliseconds()
	{
		struct timeval tv;
		gettimeofday(&tv, NULL);
		return ((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
	}

	JNIEXPORT jlong JNICALL Java_com_a2k_vncserver_VncJni_glGetGraphicsBuffer(JNIEnv *env, jobject obj,
		jint width, jint height)
	{
		AndroidGraphicBuffer *buf = new AndroidGraphicBuffer(width, height,
			AndroidGraphicBuffer::UsageTexture, AndroidGraphicBuffer::ARGB32);
		return reinterpret_cast<jlong>(buf);
	}

	JNIEXPORT void JNICALL Java_com_a2k_vncserver_VncJni_glPutGraphicsBuffer(JNIEnv *env, jobject obj, jlong buffer)
	{
		AndroidGraphicBuffer *p = reinterpret_cast<AndroidGraphicBuffer *>(buffer);
		delete p;
	}

	JNIEXPORT jboolean JNICALL Java_com_a2k_vncserver_VncJni_glBindGraphicsBuffer(JNIEnv *env, jobject obj, jlong buffer)
	{
		AndroidGraphicBuffer *p = reinterpret_cast<AndroidGraphicBuffer *>(buffer);
		return p->bind();
	}

	JNIEXPORT void JNICALL Java_com_a2k_vncserver_VncJni_glOnFrameAvailable(JNIEnv *env, jobject obj, jlong buffer)
	{
		long long start = currentTimeInMilliseconds();
#if 0
		AndroidGraphicBuffer *p = reinterpret_cast<AndroidGraphicBuffer *>(buffer);
		uint8_t *ptr;
		p->lock(AndroidGraphicBuffer::UsageTexture | AndroidGraphicBuffer::UsageSoftwareRead, &ptr);
		memcpy(gPixels, ptr, p->getWidth() * p->getHeight() * 4);
		p->unlock();
#endif
		long long delta = currentTimeInMilliseconds() - start;
		LOGD("glOnFrameAvailable %dms", static_cast<int>(delta));
	}
}
