#include <android/log.h>
#include <memory>
#include <stdlib.h>

#include "brightnesshelper.h"

extern "C"
{
	#define MODULE_NAME "brightnesshelper"

	#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO,  MODULE_NAME, __VA_ARGS__))
	#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, MODULE_NAME, __VA_ARGS__))
	#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, MODULE_NAME, __VA_ARGS__))
}

bool BrightnessHelper::initialize(const std::string &path)
{
	m_Path = path + BINARY_NAME;
	std::unique_ptr<char> cmd;
	cmd.reset(new char[MAX_PATH]);
	sprintf(cmd.get(), "su -c \"chmod 777 %s\"", m_Path.c_str());
	return system(cmd.get()) >= 0;
}

bool BrightnessHelper::setBrightness(int value)
{
	std::unique_ptr<char> cmd;
	cmd.reset(new char[MAX_PATH]);
	sprintf(cmd.get(), "su -c \"%s %d\"", m_Path.c_str(), value);
	return system(cmd.get()) >= 0;
}
