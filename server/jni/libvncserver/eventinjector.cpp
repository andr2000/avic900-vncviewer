#include <android/log.h>
#include <errno.h>
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

int EventInjector::initialize(int width, int height)
{
	m_Width = width;
	m_Height = height;
	scan();
	if (m_TouchFd != INVALID_HANDLE)
	{
	}
}

void EventInjector::scan()
{
	/* scan all input devices */
	static const std::string DEV_INPUT = {"/dev/input/"};
	std::unique_ptr<DIR, decltype(&closedir)> dir(opendir(DEV_INPUT.c_str()), closedir);

	m_TouchName.clear();
	if (!dir)
	{
		LOGE("Failed to open %s, %s", DEV_INPUT.c_str(), strerror(errno));
		return;
	}
	while (true)
	{
		LOGE("next");
		struct dirent* dirent = readdir(dir.get());
		if (dirent == nullptr)
		{
			LOGE("dirent");
			break;
		}

		std::string dev = DEV_INPUT + dirent->d_name;

		LOGE("device: %s", dev.c_str());

		/* Skip '.', '..' and hidden files */
		if (dirent->d_name[0] == '.')
		{
			LOGE(".");
			continue;
		}

		struct stat entryInfo;
		if (stat(dev.c_str(), &entryInfo) < 0)
		{
			LOGE("stat");
			break;
		}


		if (S_ISDIR(entryInfo.st_mode))
		{
			LOGE("st_mode");
			continue;
		}

		LOGD("char %d", S_ISCHR(entryInfo.st_mode));
		if (S_ISCHR(entryInfo.st_mode))
		{
			LOGD("Probing %s", dev.c_str());
			int fd = ::open(dev.c_str(), O_RDONLY);
			if (fd == INVALID_HANDLE)
			{
				LOGE("Failed to open %s", dev.c_str(), strerror(errno));
				continue;
			}
			int result = isPointerDev(fd);
			::close(fd);
			if (result == 0)
			{
				m_TouchFd = fd;
				m_TouchName = dev;
				LOGD("Found touch device at %s", m_TouchName.c_str());
				break;
			}
		}
	}
}

int EventInjector::isPointerDev(int fd)
{
	unsigned long evbits[NBits(EV_MAX)];
	unsigned long keybits[NBits(KEY_MAX)];
	unsigned long absbits[NBits(ABS_MAX)];
	int retCode;

	retCode = ioctl(fd, EVIOCGBIT(0, sizeof(evbits)), evbits);
	if (retCode == -1)
	{
		return -1;
	}

	if (!TestBit(EV_KEY, evbits))
	{
		/* No device yet */
		return -1;
	}

	retCode = ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybits)), keybits);
	if (retCode == -1)
	{
		return -1;
	}

	if ((TestBit(EV_ABS, evbits) || TestBit(EV_REL, evbits))
		&& (TestBit(BTN_TOUCH, keybits) || TestBit(BTN_LEFT, keybits)))
	{
		/* Touch (absolute pointer) test */
		retCode = ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absbits)), absbits);
		if (retCode == -1)
		{
			return -1;
		}

		if (TestBit(ABS_X, absbits) && TestBit(ABS_Y, absbits))
		{
			struct input_absinfo abs_data;

			retCode = ioctl(fd, EVIOCGABS(ABS_X), &abs_data);
			if (retCode == -1)
			{
				return -1;
			}
			m_TouchWidth = abs_data.maximum - abs_data.minimum;

			retCode = ioctl(fd, EVIOCGABS(ABS_Y), &abs_data);
			if (retCode == -1)
			{
				return -1;
			}
			m_TouchHeight = abs_data.maximum - abs_data.minimum;
			return 0;
		}
	}
	return -1;
}
