#include <android/log.h>
#include <memory>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>

#include "eventinjector.h"

extern "C"
{
	#define MODULE_NAME "eventinjector"

	#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO,  MODULE_NAME, __VA_ARGS__))
	#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, MODULE_NAME, __VA_ARGS__))
	#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, MODULE_NAME, __VA_ARGS__))
}

void EventInjector::scan()
{
	/* scan all input devices */
	static const std::string DEV_INPUT = {"/dev/input/"};
	std::unique_ptr<DIR, decltype(&closedir)> dir(opendir(DEV_INPUT.c_str()), closedir);

	m_KeypadName.clear();
	m_TouchName.clear();
	if (!dir)
	{
		LOGE("Failed to open %s", DEV_INPUT.c_str());
		return;
	}
	while (true)
	{
		struct dirent* dirent = readdir(dir.get());
		if (dirent == nullptr)
		{
			break;
		}

		std::string currentPath = DEV_INPUT + dirent->d_name;

		struct stat entryInfo;
		if (stat(currentPath.c_str(), &entryInfo) < 0)
		{
			break;
		}

		/* Skip '.', '..' and hidden files */
		if (dirent->d_name[0] == '.')
		{
			continue;
		}

		if (S_ISDIR(entryInfo.st_mode))
		{
			continue;
		}

		if (S_ISCHR(entryInfo.st_mode))
		{
			probe(currentPath);
		}
	}
}

int EventInjector::open(const std::string &dev)
{
	/* open input device */
	return ::open(dev.c_str(), O_WRONLY);
}

void EventInjector::close(const int fd)
{
	::close(fd);
}

EventInjector::DEV_CHECK_RET_CODE EventInjector::isPointerDev(int fd)
{
	unsigned long evbits[NBits(EV_MAX)];
	unsigned long keybits[NBits(KEY_MAX)];
	unsigned long absbits[NBits(ABS_MAX)];
	int retCode;

	retCode = ioctl(fd, EVIOCGBIT(0, sizeof(evbits)), evbits);
	if (retCode == -1)
	{
		LOGE("Failed to get EVIOCGBIT\n");
		return FAILED;
	}

	if (!TestBit(EV_KEY, evbits))
	{
		/* No device yet */
		return NO_DEVICE;
	}

	retCode = ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybits)), keybits);
	if (retCode == -1)
	{
		LOGE("Failed to get EVIOCGBIT\n");
		return FAILED;
	}

	if ((TestBit(EV_ABS, evbits) || TestBit(EV_REL, evbits))
		&& (TestBit(BTN_TOUCH, keybits) || TestBit(BTN_LEFT, keybits)))
	{
		/* Touch (absolute pointer) test */
		retCode = ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absbits)), absbits);
		if (retCode == -1)
		{
			LOGE("Failed to get EVIOCGBIT\n");
			return FAILED;
		}

		if (TestBit(ABS_X, absbits) && TestBit(ABS_Y, absbits))
		{
			struct input_absinfo abs_data;

			retCode = ioctl(fd, EVIOCGABS(ABS_X), &abs_data);
			if (retCode == -1)
			{
				LOGE("Failed to get EVIOCGABS\n");
				return FAILED;
			}
			m_TouchValueX = abs_data.value;
			m_TouchMinX = abs_data.minimum;
			m_TouchMaxX = abs_data.maximum;

			retCode = ioctl(fd, EVIOCGABS(ABS_Y), &abs_data);
			if (retCode == -1)
			{
				LOGE("Failed to get EVIOCGABS\n");
				return FAILED;
			}
			m_TouchValueY = abs_data.value;
			m_TouchMinY = abs_data.minimum;
			m_TouchMaxY = abs_data.maximum;
			return FOUND;
		}
	}
	return NO_DEVICE;
}

void EventInjector::probe(const std::string &dev)
{
	LOGD("Probing %s", dev.c_str());
	int fd = open(dev);
	if (fd == INVALID_HANDLE)
	{
		LOGE("Failed to open %s", dev.c_str());
		return;
	}
	DEV_CHECK_RET_CODE result = isPointerDev(fd);
	if (result == FOUND)
	{
		m_TouchFd = fd;
		m_TouchName = dev;
		LOGD("Found touch device at %s", m_TouchName.c_str());
	}
	close(fd);
}
