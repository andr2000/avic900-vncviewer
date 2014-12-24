#include <android/log.h>
#include <memory>
#include <stdlib.h>

#include "brightness.h"

extern "C"
{
	#define MODULE_NAME "brightness"

	#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO,  MODULE_NAME, __VA_ARGS__))
	#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, MODULE_NAME, __VA_ARGS__))
	#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, MODULE_NAME, __VA_ARGS__))
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		LOGE("Usage: brightness <level>");
		return -1;
	}
	std::unique_ptr<Brightness> brightness;

	brightness.reset(new Brightness());
	if (brightness == nullptr)
	{
		return -1;
	}
	if (!brightness->initialize())
	{
		LOGE("Failed to initialize brightness module");
		return -1;
	}
	int value = atoi(argv[1]);
	brightness->setBrightness(value);
	brightness.reset();
	return 0;
}
