#include <string>
#include <sys/file.h>

#include "log.h"
#include "server_unix.h"
#include "vnchelper_client.h"

ServerUnix *g_Server;
VncHelperClient *g_Client;

int onClient(int fd, bool connected)
{
	if (connected)
	{
		if (g_Client)
		{
			LOGE("Only single connection supported");
			return -1;
		}
		g_Client = new VncHelperClient();
		int result = g_Client->open();
		if (result < 0)
		{
			delete g_Client;
			g_Client = nullptr;
		}
		return result;
	}
	/* single client has disconnected */
	delete g_Client;
	g_Client = nullptr;
	return 0;
}
int onDataReceived(int fd, const ServerUnix::packet_t &packet)
{
	if (!g_Client)
	{
		LOGE("Data received, but no connection");
		return -1;
	}
	int result = g_Client->processMessage(fd, packet);
	if (result < 0)
	{
		g_Server->stop();
		return 0;
	}
	return result;
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		LOGE("Usage: %s <unix-socket path>", MODULE_NAME);
		return -1;
	}
	/* check for single instance */
	{
		std::string pidFile { argv[0] };
		pidFile += ".pid";
		int fd = open(pidFile.c_str(), O_CREAT | O_RDWR, 0666);
		int rc = flock(fd, LOCK_EX | LOCK_NB);
		if (rc)
		{
			/* either would block or error */
			LOGE("Already running, exiting now");
			return 0;
		}
	}
	LOGI("Starting %s, listening on %s", MODULE_NAME, argv[1]);
	g_Server = new ServerUnix();
	if (g_Server->open(argv[1], onClient, onDataReceived) >= 0)
	{
		g_Server->run();
	}
	LOGI("Closing %s", MODULE_NAME);
	delete g_Client;
	delete g_Server;
	return 0;
}
