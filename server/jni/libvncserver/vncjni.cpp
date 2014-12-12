#include <android/log.h>
#include <jni.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>

#include "AndroidGraphicBuffer.h"
#include "rfb/rfbconfig.h"

#define MODULE_NAME "vncjni"

#define PIXEL_FORMAT	AndroidGraphicBuffer::HAL_PIXEL_FORMAT_RGBA_8888
#define BYTES_PER_PIXEL	4
uint8_t gPixels[1024 * 1024 * BYTES_PER_PIXEL];

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
			(AndroidGraphicBuffer::GRALLOC_USAGE_HW_TEXTURE |
			AndroidGraphicBuffer::GRALLOC_USAGE_SW_READ_OFTEN),
			PIXEL_FORMAT);
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
		AndroidGraphicBuffer *p = reinterpret_cast<AndroidGraphicBuffer *>(buffer);
		uint8_t *ptr;
		p->lock(AndroidGraphicBuffer::GRALLOC_USAGE_SW_READ_OFTEN, &ptr);
		memcpy(gPixels, ptr, p->getWidth() * p->getHeight() * BYTES_PER_PIXEL);
		p->unlock();
		long long done = currentTimeInMilliseconds();
		LOGD("glOnFrameAvailable (%dx%d), done in %dms",
			p->getWidth(), p->getHeight(), static_cast<int>(done - start));
	}

	JNIEXPORT void JNICALL Java_com_a2k_vncserver_VncJni_glDumpFrame(JNIEnv *env, jobject obj, jlong buffer, jstring path)
	{
		const char *nativePath = env->GetStringUTFChars(path, JNI_FALSE);
		FILE *f = fopen(nativePath, "w+b");
		if (f)
		{
			AndroidGraphicBuffer *p = reinterpret_cast<AndroidGraphicBuffer *>(buffer);
			fwrite(gPixels, 1, p->getWidth() * p->getHeight() * BYTES_PER_PIXEL, f);
			fclose(f);
			LOGD("Frame saved at %s", nativePath);
		}
		else
		{
			LOGE("Failed to save frame at %s", nativePath);
		}
		env->ReleaseStringUTFChars(path, nativePath);
	}
}
