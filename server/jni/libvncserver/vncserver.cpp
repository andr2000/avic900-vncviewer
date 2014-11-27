#include <jni.h>
#include <string.h>

#include "rfb/rfbconfig.h"

extern "C"
{
	JNIEXPORT jstring JNICALL Java_com_a2k_vncserver_VncServerProto_getVersion(JNIEnv *env, jobject obj)
	{
		return env->NewStringUTF("libvncserver " LIBVNCSERVER_VERSION);
	}
}
