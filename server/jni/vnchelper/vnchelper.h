#ifndef VNCHELPER_H_
#define VNCHELPER_H_

#include <memory>

#include "brightness.h"

class VncHelper
{
public:
	enum
	{
		SHUTDOWN,
		SET_BRIGHTNESS,
	};

	struct packet_t
	{
		uint32_t id;
		uint32_t data;
	};

	static VncHelper &getInstance()
	{
		static VncHelper m_Instance;
		return m_Instance;
	}
	int open(const char *pipeName);
	void run();

private:
	const int READ_TO_MS = 50;
	const int INVALID_HANDLE { -1 };
	int m_Fd { INVALID_HANDLE };
	bool m_Terminated { false };
	std::unique_ptr<Brightness> m_Brightness;

	void close();
	ssize_t read(void *buffer, size_t length);
	void processMessage(const packet_t &packet);
};

#endif /* VNCHELPER_H_ */
