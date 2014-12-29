#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "log.h"
#include "server_unix.h"

ServerUnix::ServerUnix()
{
}

ServerUnix::~ServerUnix()
{
}

int ServerUnix::open(const char *sockName, const ClientCallback &clientCallback,
		const DataCallback &dataCallback)
{
	m_ClientCallback = clientCallback;
	m_DataCallback = dataCallback;

	if ((m_ClientCallback == nullptr) || (m_DataCallback == nullptr))
	{
		LOGE("No callbacks set");
		return -1;
	}
	m_ServerFd = socket(AF_UNIX, SOCK_STREAM, 0);
	struct sockaddr_un address;
	address.sun_family = AF_UNIX;
	strncpy(address.sun_path, sockName, sizeof(address.sun_path) - 1);
	unsigned int len = strlen(address.sun_path) + sizeof(address.sun_family);
	unlink(address.sun_path);
	if (bind(m_ServerFd, reinterpret_cast<struct sockaddr *>(&address), len) == -1)
	{
		return -1;
	}
	/* EPIPE: fd is connected to a pipe or socket whose reading end is closed.
	 * When this happens the writing process will also receive a SIGPIPE signal.
	 * (Thus, the write return value is seen only if the program catches, blocks
	 * or ignores this signal.) */
	signal(SIGPIPE, SIG_IGN);

	FD_ZERO(&m_MasterFdSet);
	FD_ZERO(&m_ReadFdSet);

	if (listen(m_ServerFd, 10) == -1)
	{
		LOGD("%s", strerror(errno));
		return -1;
	}

	/* add the listener to the master set */
	FD_SET(m_ServerFd, &m_MasterFdSet);
	/* keep track of the biggest file descriptor */
	m_FdMax = m_ServerFd;
	m_Terminated = false;
	m_ReadPos = 0;
	return 0;
}

int ServerUnix::handleReceive(int fd)
{
	int nRead;
	int toRead = sizeof(packet_t) - m_ReadPos;
	uint8_t *buffer = &(reinterpret_cast<uint8_t *>(&m_CurPacket))[m_ReadPos];

	if ((nRead = recv(fd, buffer, toRead, 0)) <= 0)
	{
		/* got error or connection closed by client */
		if (nRead < 0)
		{
			LOGE("%s", strerror(errno));
		}
		m_ClientCallback(fd, false);
		return -1;
	}
	m_ReadPos += nRead;
	if (m_ReadPos != sizeof(packet_t))
	{
		/* have to wait for more */
		return 0;
	}
	m_ReadPos = 0;
	return m_DataCallback(fd, m_CurPacket);
}

void ServerUnix::run()
{
	while (!m_Terminated)
	{
		m_ReadFdSet = m_MasterFdSet;
		if (select(m_FdMax + 1, &m_ReadFdSet, nullptr, nullptr, nullptr) == -1)
		{
			LOGD("%s", strerror(errno));
			return;
		}

		/* run through the existing connections looking for data to read */
		for (int i = 0; i <= m_FdMax; i++)
		{
			if (!FD_ISSET(i, &m_ReadFdSet))
			{
				continue;
			}
			/* new connection? */
			if (i == m_ServerFd)
			{
				struct sockaddr_storage remoteaddr;
				int addrlen = sizeof(remoteaddr);
				int clientFd = accept(m_ServerFd, reinterpret_cast<struct sockaddr *>(&remoteaddr), &addrlen);

				if (clientFd == INVALID_HANDLE)
				{
					LOGD("%s", strerror(errno));
				}
				else
				{
					if (m_ClientCallback(clientFd, true) < 0)
					{
						close(clientFd);
					}
					else
					{
						/* add to master set */
						FD_SET(clientFd, &m_MasterFdSet);
						if (clientFd > m_FdMax)
						{
							/* keep track of the max */
							m_FdMax = clientFd;
						}
					}
				}
				continue;
			}
			/* handle data from a client */
			if (handleReceive(i) < 0)
			{
				/* connection dropped */
				close(i);
				FD_CLR(i, &m_MasterFdSet);
			}
		}
	}
}

void ServerUnix::stop()
{
	m_Terminated = true;
}
