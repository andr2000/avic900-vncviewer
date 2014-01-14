#include <stdio.h>

#include "client_factory.h"

Client *g_vnc_client;

int main(int argc, char *argv[]) {
	std::string exe(argv[0]);
	std::string ini(argv[0]);

	g_vnc_client = ClientFactory::GetInstance();
	if (NULL == g_vnc_client) {
		fprintf(stderr, "Failed to intstaniate VNC client\n");
 		return -1;
	}

	ini += ".ini";
	if (g_vnc_client->Start(static_cast<void *>(NULL), exe, ini) < 0) {
	fprintf(stderr, "Failed to start VNC client\n");
		return -1;
	}
	while (1) {
		sleep(1);
	}
	return 0;
}
