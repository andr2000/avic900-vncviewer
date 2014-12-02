#include <android/log.h>
#include <android/bitmap.h>
#include <jni.h>
#include <string.h>
#include <sys/time.h>

#include "rfb/rfbconfig.h"

#define MODULE_NAME "vncserver"


extern "C"
{
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO,  MODULE_NAME, __VA_ARGS__))
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, MODULE_NAME, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, MODULE_NAME, __VA_ARGS__))

	JNIEXPORT jstring JNICALL Java_com_a2k_vncserver_VncServerProto_getVersion(JNIEnv *env, jobject obj)
	{
		return env->NewStringUTF("libvncserver " LIBVNCSERVER_VERSION);
	}

	long long currentTimeInMilliseconds()
	{
		struct timeval tv;
		gettimeofday(&tv, NULL);
		return ((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
	}

	JNIEXPORT void JNICALL Java_com_a2k_vncserver_VncServerProto_updateScreen(JNIEnv *env, jobject obj, jobject bitmap)
	{
		static long long lastFpsTime = 0;
		static int frameCounter = 0;

		int delta = (int)(currentTimeInMilliseconds() - lastFpsTime);
		if (delta > 1000)
		{
			double fps = (((double)frameCounter)/delta) * 1000;
			frameCounter = 0;
			lastFpsTime = currentTimeInMilliseconds();
			LOGD("FPS: %f", fps);
		}
		frameCounter++;

		AndroidBitmapInfo info;
		int ret;
		if ((ret = AndroidBitmap_getInfo(env, bitmap, &info)) < 0)
		{
			LOGE("AndroidBitmap_getInfo() failed, error=%d", ret);
			return;
		}
		if (info.format != ANDROID_BITMAP_FORMAT_RGB_565)
		{
			LOGE("Bitmap format is not RGBA_8888!");
			return;
		}
		void *bitmapPixels;
		if ((ret = AndroidBitmap_lockPixels(env, bitmap, &bitmapPixels)) < 0)
		{
			LOGE("AndroidBitmap_lockPixels() failed, error=%d", ret);
			return;
		}
		int stride = info.stride;
		int pixelsCount = info.height * info.width;
		AndroidBitmap_unlockPixels(env, bitmap);
	}
}
