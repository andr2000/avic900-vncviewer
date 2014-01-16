#include <stdio.h>

#include "client_factory.h"
#include "config_storage.h"

int main(int argc, char *argv[]) {
	std::string exe(argv[0]);
	ConfigStorage *cfg = ConfigStorage::GetInstance();
	Client *vnc_client;
	std::string ini, server_ip;
	bool alive;

	ini = exe + ".ini";
	cfg->Initialize(exe, ini);
	vnc_client = ClientFactory::GetInstance();
	if (NULL == vnc_client) {
		fprintf(stderr, "Failed to instantiate VNC client\n");
 		return -1;
	}
	if (vnc_client->Initialize(static_cast<void *>(NULL)) < 0) {
	fprintf(stderr, "Failed to initialize VNC client\n");
		return -1;
	}
	server_ip = vnc_client->GetServerIP();
	alive = vnc_client->IsServerAlive(server_ip);
	fprintf(stdout, "Server %s is %s alive\n", server_ip.c_str(),
			alive ? "" : "not");
	if (!alive) {
		delete vnc_client;
		delete cfg;
		return -1;
	}
	fprintf(stdout, "Trying to connect to %s\n", cfg->GetServer().c_str());
	if (vnc_client->Connect() < 0) {
		fprintf(stderr, "Failed to start VNC client\n");
		return -1;
	}
	while (1) {
		Client::event_t evt;

		sleep(10);
		fprintf(stdout, "KEY_HOME\n");
		evt.what = Client::EVT_KEY;
		evt.data.key = Client::KEY_HOME;
		vnc_client->PostEvent(evt);
		sleep(10);
		fprintf(stdout, "KEY_BACK\n");
		evt.what = Client::EVT_KEY;
		evt.data.key = Client::KEY_BACK;
		vnc_client->PostEvent(evt);
	}
	if (vnc_client) {
		delete vnc_client;
	}
	if (cfg) {
		delete cfg;
	}
	return 0;
}
