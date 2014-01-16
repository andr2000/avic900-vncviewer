#ifndef _CONNECTION_WATCHDOG_H
#define _CONNECTION_WATCHDOG_H

class Client;
class ConfigStorage;
class Thread;

class ConnectionWatchdog {
public:
	ConnectionWatchdog(Client *client);
	virtual ~ConnectionWatchdog();

	void ShutDown();
	int Start();
private:
	Client *m_Client;
	Thread *m_Thread;
	ConfigStorage *m_ConfigStorage;
	std::string m_ServerIP;
	static const int MAX_TRY = 3;
	int m_MaxTryCounter;

	int PollConnection();
	static int PollConnectionCb(void *data);
};

#endif
