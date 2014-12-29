#ifndef SERVER_UNIX_H
#define SERVER_UNIX_H

#include <functional>
#include <unistd.h>

class ServerUnix
{
public:
	struct packet_t
	{
		uint32_t id;
		uint32_t data;
	} __attribute__ ((aligned(1)));

	typedef std::function <int(int fd, bool connected)> ClientCallback;
	typedef std::function <int(int fd, const packet_t &packet)> DataCallback;

	ServerUnix();
	~ServerUnix();

	int open(const char *sockName, const ClientCallback &clientCallback,
		const DataCallback &dataCallback);
	void run();
	void stop();

private:
	const int INVALID_HANDLE = { -1 };
	int m_ServerFd { INVALID_HANDLE };
	bool m_Terminated { false };

	fd_set m_MasterFdSet;
	fd_set m_ReadFdSet;
	int m_FdMax;

	packet_t m_CurPacket;
	int m_ReadPos { 0 };
	ClientCallback m_ClientCallback { nullptr };
	DataCallback m_DataCallback { nullptr };
	int handleReceive(int fd);
};

#endif /* SERVER_UNIX_H */
