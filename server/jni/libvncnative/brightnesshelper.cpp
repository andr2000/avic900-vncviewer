#include <android/log.h>
#include <memory>
#include <stdlib.h>

#include "brightnesshelper.h"
#include "log.h"

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
