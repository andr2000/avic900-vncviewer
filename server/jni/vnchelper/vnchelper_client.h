#ifndef VNCHELPER_CLIENT_H_
#define VNCHELPER_CLIENT_H_

#include <memory>

#include "brightness.h"
#include "server_unix.h"

class VncHelperClient
{
public:
	enum
	{
		SHUTDOWN,
		SET_BRIGHTNESS,
	};
	int open();
	int processMessage(const int fd, const ServerUnix::packet_t &packet);

private:
	std::unique_ptr<Brightness> m_Brightness;
};

#endif /* VNCHELPER_CLIENT_H_ */
