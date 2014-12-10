#include <android/log.h>
#include <android/bitmap.h>
#include <jni.h>
#include <string.h>
#include <stdio.h>
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

	void dumpBuffer(int n, const uint8_t *buf)
	{
		int col;
		char str[128];

		while (n)
		{
			str[0] = '\0';
			for (col = 0; (col < 16) && n; col++, n--)
			{
				char tmp[16];
				sprintf(tmp, "%02X ", *buf++);
				strcat(str, tmp);
			}
			LOGD("%s", str);
		}
	}

	JNIEXPORT jlong JNICALL Java_com_a2k_vncserver_VncJni_glGetGraphicsBuffer(JNIEnv *env, jobject obj,
		jint width, jint height)
	{
		AndroidGraphicBuffer *buf = new AndroidGraphicBuffer(width, height,
			(AndroidGraphicBuffer::GRALLOC_USAGE_HW_TEXTURE |
			AndroidGraphicBuffer::GRALLOC_USAGE_SW_READ_OFTEN),
			AndroidGraphicBuffer::HAL_PIXEL_FORMAT_RGBA_8888);
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
		memcpy(gPixels, ptr, p->getWidth() * p->getHeight() * 4);
		p->unlock();
		int n = p->getWidth() * p->getHeight() * 4;
		bool hasData = false;
		while (n--)
		{
			if (gPixels[n] != 0)
			{
				hasData = true;
				break;
			}
		}
		long long delta = currentTimeInMilliseconds() - start;
		LOGD("glOnFrameAvailable (%dx%d) is %sempty, read in %dms",
			p->getWidth(), p->getHeight(), hasData ? "not " : "",
			static_cast<int>(delta));
		dumpBuffer(64, gPixels);
	}

	JNIEXPORT void JNICALL Java_com_a2k_vncserver_VncJni_glUpdateScreen(JNIEnv *env, jobject obj, jobject bitmap)
	{
		long long start = currentTimeInMilliseconds();
		AndroidBitmapInfo info;
		int ret;
		if ((ret = AndroidBitmap_getInfo(env, bitmap, &info)) < 0)
		{
			LOGE("AndroidBitmap_getInfo() failed, error=%d", ret);
			return;
		}
		if (info.format != ANDROID_BITMAP_FORMAT_RGB_565)
		{
			LOGE("Bitmap format is not RGB_565!");
			return;
		}
		void *bitmapPixels;
		if ((ret = AndroidBitmap_lockPixels(env, bitmap, &bitmapPixels)) < 0)
		{
			LOGE("AndroidBitmap_lockPixels() failed, error=%d", ret);
			return;
		}
		int stride = info.stride;
		memcpy(gPixels, bitmapPixels, info.height * info.width * 2);
		AndroidBitmap_unlockPixels(env, bitmap);
		long long delta = currentTimeInMilliseconds() - start;
		LOGD("glUpdateScreen (%dx%d), read in %dms",
			info.width, info.height, static_cast<int>(delta));
		dumpBuffer(64, gPixels);
	}
}
