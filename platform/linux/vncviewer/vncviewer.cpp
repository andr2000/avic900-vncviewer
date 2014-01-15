#include <stdio.h>

#include "client_factory.h"
#include "config_storage.h"


int main(int argc, char *argv[]) {
	std::string exe(argv[0]);
	ConfigStorage *cfg = new ConfigStorage();
	Client *vnc_client;
	std::string ini;

	ini = exe + ".ini";
	cfg->Initialize(exe, ini);
	vnc_client = ClientFactory::GetInstance();
	if (NULL == vnc_client) {
		fprintf(stderr, "Failed to instantiate VNC client\n");
 		return -1;
	}
	fprintf(stdout, "Trying to connect to %s\n", cfg->GetServer().c_str());
	if (vnc_client->Start(static_cast<void *>(NULL)) < 0) {
	fprintf(stderr, "Failed to start VNC client\n");
		return -1;
	}
	while (1) {
		sleep(1);
	}
	if (vnc_client) {
		delete vnc_client;
	}
	if (cfg) {
		delete cfg;
	}
	return 0;
}
