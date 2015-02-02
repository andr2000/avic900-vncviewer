#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "log.h"
#include "vnchelper.h"

int VncHelper::open(const char *pipeName)
{
	struct stat buf;
	if ((stat(pipeName, &buf) < 0) || (!(buf.st_mode | S_IFIFO)))
	{
		LOGD("The file \"%s\" isn't a pipe", pipeName);
		return -1;
	}
	m_Fd = ::open(pipeName, O_RDONLY);
	if (m_Fd == INVALID_HANDLE)
	{
		LOGE("%s", strerror(errno));
		return -1;
	}
	m_Brightness.reset(new Brightness());
	if (m_Brightness == nullptr)
	{
		close();
		return -1;
	}
	if (!m_Brightness->initialize())
	{
		LOGE("Failed to initialize brightness module");
		close();
		return -1;
	}
	return 0;
}

void VncHelper::close()
{
	::close(m_Fd);
}

void VncHelper::processMessage(const VncHelper::packet_t &packet)
{
	switch (packet.id)
	{
		case SHUTDOWN:
		{
			LOGD("Shutting down");
			m_Terminated = true;
			break;
		}
		case SET_BRIGHTNESS:
		{
			if (m_Brightness)
			{
				m_Brightness->setBrightness(packet.data);
			}
			break;
		}
		default:
		{
			LOGE("Unknown command received: %d", packet.id);
			break;
		}
	}
}

ssize_t VncHelper::read(void *buffer, size_t length)
{
	fd_set fdSet;
	FD_ZERO(&fdSet);
	FD_SET(m_Fd, &fdSet);
	struct timeval timeVal {0, READ_TO_MS * 1000};

	int result = select(m_Fd + 1, &fdSet, NULL, NULL, &timeVal);
	if (result < 0)
	{
		if (errno == EINTR)
		{
			return 0;
		}
		::close(m_Fd);
		return result;
	}
	else if (result == 0)
	{
		return 0;
	}
	result = ::read(m_Fd, buffer, length);
	if (result < 0)
	{
		::close(m_Fd);
	}
	return result;
}

void VncHelper::run()
{
	while (!m_Terminated)
	{
		/* read next packet */
		packet_t packet;
		ssize_t n = read(&packet, sizeof(packet));
		if (n < 0)
		{
			LOGE("%s", strerror(errno));
			break;
		}
		if (n == sizeof(packet))
		{
			processMessage(packet);
		}
	}
}
