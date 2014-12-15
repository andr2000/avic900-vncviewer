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

	JNIEXPORT jlong JNICALL Java_com_a2k_vncserver_VncJni_glGetGraphicsBuffer(JNIEnv *env, jobject obj,
		jint width, jint height, int pixelFormat)
	{
		int format = 0;
		switch (pixelFormat)
		{
			case GL_RGBA:
			{
				format = AndroidGraphicBuffer::HAL_PIXEL_FORMAT_RGBA_8888;
				gBytesPerPixel = 4;
				break;
			}
			case GL_RGB565:
			{
				format = AndroidGraphicBuffer::HAL_PIXEL_FORMAT_RGB_565;
				gBytesPerPixel = 2;
				break;
			}
			default:
			{
				LOGE("Unsupported pixel format");
				return 0;
			}
		}
		AndroidGraphicBuffer *buf = new AndroidGraphicBuffer(width, height,
			(AndroidGraphicBuffer::GRALLOC_USAGE_HW_TEXTURE |
			AndroidGraphicBuffer::GRALLOC_USAGE_SW_READ_OFTEN),
			format);
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
		memcpy(gPixels, ptr, p->getWidth() * p->getHeight() * gBytesPerPixel);
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
			fwrite(gPixels, 1, p->getWidth() * p->getHeight() * gBytesPerPixel, f);
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
