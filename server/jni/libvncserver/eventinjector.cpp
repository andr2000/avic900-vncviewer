#include <android/log.h>
#include <errno.h>
#include <memory>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include "uinput.h"

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
	return 0;
}

bool EventInjector::createDevice()
{
	const char* USER_INPUT_DEV = "/dev/uinput";
	int retCode = 0;

	//Open uinput device if it is not open for now
	if (m_TouchFd < 0)
	{
		m_TouchFd = ::open(USER_INPUT_DEV, O_WRONLY | O_NONBLOCK);
		if (m_TouchFd < 0)
		{
			LOGD("Fail to open %s\n", USER_INPUT_DEV);
			return false;
		}
	}

	/*
	if (m_Configured == true)
	{
		//Destroy uinput device
		m_Configured = false;
		retCode = ioctl(m_TouchFd, UI_DEV_DESTROY);
		if (retCode == -1)
		{
			LOG_DEBUG("Fail to execute UI_DEV_DESTROY %s\n", strerror(retCode));
			::close(m_TouchFd);
			m_TouchFd = -1;
			return false;
		}
	}

	//Test configuration consistency
	if ((xMinValue < 0) || (xMaxValue < 0) || (yMinValue < 0) || (yMaxValue < 0))
	{
		LOGD("X or Y dimension under zero.\n");
		::close(m_TouchFd);
		m_TouchFd = -1;
		return false;
	}
	*/

	retCode = ioctl(m_TouchFd, UI_SET_EVBIT, EV_KEY);
	if (retCode == -1)
	{
		LOGD("Fail to set EV_KEY bit %s\n", strerror(retCode));
		::close(m_TouchFd);
		m_TouchFd = -1;
		return false;
	}

	retCode = ioctl(m_TouchFd, UI_SET_EVBIT, EV_SYN);
	if (retCode == -1)
	{
		LOGD("Fail to set EV_SYN bit %s\n", strerror(retCode));
		::close(m_TouchFd);
		m_TouchFd = -1;
		return false;
	}

	retCode = ioctl(m_TouchFd, UI_SET_EVBIT, EV_ABS);
	if (retCode == -1)
	{
		LOGD("Fail to set EV_ABS bit %s\n", strerror(retCode));
		::close(m_TouchFd);
		m_TouchFd = -1;
		return false;
	}

	retCode = ioctl(m_TouchFd, UI_SET_KEYBIT, BTN_TOUCH);
	if (retCode == -1)
	{
		LOGD("Fail to set BTN_TOUCH bit %s\n", strerror(retCode));
		::close(m_TouchFd);
		m_TouchFd = -1;
		return false;
	}

	retCode = ioctl(m_TouchFd, UI_SET_ABSBIT, ABS_X);
	if (retCode == -1)
	{
		LOGD("Fail to set ABS_X bit %s\n", strerror(retCode));
		::close(m_TouchFd);
		m_TouchFd = -1;
		return false;
	}

	retCode = ioctl(m_TouchFd, UI_SET_ABSBIT, ABS_Y);
	if (retCode == -1)
	{
		LOGD("Fail to set ABS_Y bit %s\n", strerror(retCode));
		::close(m_TouchFd);
		m_TouchFd = -1;
		return false;
	}

	struct uinput_user_dev uidev;
	memset(&uidev, 0, sizeof(uidev));
	snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "uinput-touch");
	uidev.absmin[ABS_X] = 0;
	uidev.absmax[ABS_X] = 800;
	uidev.absmin[ABS_Y] = 0;
	uidev.absmax[ABS_Y] = 480;
	retCode = ::write(m_TouchFd, &uidev, sizeof(uidev));

	if (retCode == -1)
	{
		LOGD("Fail to write user device config %s \n", strerror(retCode));
		::close(m_TouchFd);
		m_TouchFd = -1;
		return false;
	}

	retCode = ioctl(m_TouchFd, UI_DEV_CREATE);
	if (retCode == -1)
	{
		LOGD("Fail to execute UI_DEV_CREATE %s\n", strerror(retCode));
		::close(m_TouchFd);
		m_TouchFd = -1;
		return false;
	}
	return true;
}

void EventInjector::scan()
{
	createDevice();
	return;
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
