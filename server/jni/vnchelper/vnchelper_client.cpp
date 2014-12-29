#include "log.h"
#include "vnchelper_client.h"

int VncHelperClient::open()
{
	m_Brightness.reset(new Brightness());
	if (m_Brightness == nullptr)
	{
		return -1;
	}
	if (!m_Brightness->initialize())
	{
		LOGE("Failed to initialize brightness module");
		return -1;
	}
	return 0;
}

int VncHelperClient::processMessage(const int fd, const ServerUnix::packet_t &packet)
{
	switch (packet.id)
	{
		case SHUTDOWN:
		{
			LOGD("Shutting down");
			return -1;
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
	return 0;
}
