#include <android/log.h>

#include "eventinjector.h"

extern "C"
{
	#define MODULE_NAME "eventinjector"

	#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO,  MODULE_NAME, __VA_ARGS__))
	#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, MODULE_NAME, __VA_ARGS__))
	#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, MODULE_NAME, __VA_ARGS__))
}

bool EventInjector::initialize(int width, int height)
{
	return true;
}
