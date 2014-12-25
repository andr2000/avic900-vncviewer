#include <android/log.h>
#include <stdlib.h>
#include <sstream>

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
	std::stringstream ss;
	ss << "su -c \"chmod 777 " << m_Path << "\"";
	std::string cmd = ss.str();
	return system(cmd.c_str()) >= 0;
}

bool BrightnessHelper::setBrightness(int value)
{
	std::stringstream ss;
	ss << "su -c \"" << m_Path << " " << value << + "\"";
	std::string cmd = ss.str();
	return system(cmd.c_str()) >= 0;
}
