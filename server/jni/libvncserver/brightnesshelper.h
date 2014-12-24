#ifndef BRIGHTNESS_HELPER_H_
#define BRIGHTNESS_HELPER_H_

#include <string>

class BrightnessHelper
{
public:
	BrightnessHelper() = default;
	~BrightnessHelper() = default;

	bool initialize(const std::string &path);
	bool setBrightness(int value);

private:
	const char *BINARY_NAME = "/lib/libbrightness.so";
	const int MAX_PATH = 512;
	std::string m_Path;
};

#endif /* BRIGHTNESS_HELPER_H_ */
