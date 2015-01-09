#include <stdio.h>
#include <termios.h>
#include <unistd.h>

#include "client_gtk.h"
#include "config_storage.h"

int main(int argc, char *argv[])
{
	std::string exe(argv[0]);
	ConfigStorage *cfg = ConfigStorage::GetInstance();
	Client *vnc_client;
	std::string ini;
	static struct termios oldt, newt;
	char c;
	bool run = true;

	ini = exe + ".ini";
	cfg->Initialize(exe, ini);
	vnc_client = new Client_Gtk();
	if (NULL == vnc_client)
	{
		fprintf(stderr, "Failed to instantiate VNC client\n");
		return -1;
	}
	if (vnc_client->Initialize() < 0)
	{
		fprintf(stderr, "Failed to initialize VNC client\n");
		return -1;
	}
	fprintf(stdout, "Trying to connect to %s\n", cfg->GetServer().c_str());
	if (vnc_client->Connect() < 0)
	{
		fprintf(stderr, "Failed to start VNC client\n");
		return -1;
	}
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	while (run)
	{
		fprintf(stdout, "q to quit, h for Home or e for Escape:\n");
		c = getchar();
		switch (c)
		{
			case 'q':
				run = false;
				break;
			case 'h':
			{
				Client::event_t evt;

				fprintf(stdout, "KEY_HOME\n");
				evt.what = Client::EVT_KEY;
				evt.data.key = Client::KEY_HOME;
				vnc_client->PostEvent(evt);
				break;
			}
			case 'e':
			{
				Client::event_t evt;

				fprintf(stdout, "KEY_BACK\n");
				evt.what = Client::EVT_KEY;
				evt.data.key = Client::KEY_BACK;
				vnc_client->PostEvent(evt);
				break;
			}
			case 'w':
			{
				Client::event_t evt;

				fprintf(stdout, "KEY_UP\n");
				evt.what = Client::EVT_KEY;
				evt.data.key = Client::KEY_UP;
				vnc_client->PostEvent(evt);
				break;
			}
			case 's':
			{
				Client::event_t evt;

				fprintf(stdout, "KEY_DOWN\n");
				evt.what = Client::EVT_KEY;
				evt.data.key = Client::KEY_DOWN;
				vnc_client->PostEvent(evt);
				break;
			}
			case 'a':
			{
				Client::event_t evt;

				fprintf(stdout, "KEY_LEFT\n");
				evt.what = Client::EVT_KEY;
				evt.data.key = Client::KEY_LEFT;
				vnc_client->PostEvent(evt);
				break;
			}
			case 'd':
			{
				Client::event_t evt;

				fprintf(stdout, "KEY_RIGHT\n");
				evt.what = Client::EVT_KEY;
				evt.data.key = Client::KEY_RIGHT;
				vnc_client->PostEvent(evt);
				break;
			}
			default:
				break;
		}
	}
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	if (vnc_client)
	{
		delete vnc_client;
	}
	return 0;
}
