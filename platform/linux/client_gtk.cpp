#include <stdarg.h>

#include <rfb/rfbclient.h>
#include "client_gtk.h"

Client_Gtk::Client_Gtk() : Client() {
	m_FrameBuffer = NULL;
}

Client_Gtk::~Client_Gtk() {
}

void Client_Gtk::Logger(const char *format, ...) {
    va_list args;

    if(!rfbEnableClientLogging)
      return;

    va_start(args, format);
	vfprintf(stdout, format, args);
    va_end(args);
}

void Client_Gtk::SetLogging() {
	rfbClientLog = Logger;
	rfbClientErr = Logger;
}

rfbBool Client_Gtk::OnMallocFrameBuffer(rfbClient *client) {
	int width, height, depth;

	width = client->width;
	height = client->height;
	depth = client->format.bitsPerPixel;
	rfbClientLog("OnMallocFrameBuffer: w=%d h=%d d=%d\n", width, height, depth);
	client->updateRect.x = 0;
	client->updateRect.y = 0;
	client->updateRect.w = width;
	client->updateRect.h = height;

	/* create frambuffer */
	client->frameBuffer = m_FrameBuffer;
	return TRUE;
}

void Client_Gtk::OnFrameBufferUpdate(rfbClient* client, int x, int y, int w, int h) {
	rfbClientLog("OnFrameBufferUpdate: x=%d y=%d w=%d h=%d\n", x, y, w, h);
}

bool Client_Gtk::IsServerAlive(std::string &host) {
	/* TODO: this is not portable */
	std::string cmd = "/bin/ping -q -c 1 -n " + host;
	FILE* pipe = popen(cmd.c_str(), "r");
	if (!pipe) {
		return false;
	}
	char buffer[128];
	std::string result = "";
	while (!feof(pipe)) {
		if (fgets(buffer, sizeof(buffer), pipe) != NULL)
			result += buffer;
	}
	pclose(pipe);
	if (std::string::npos != result.find_first_of("unknown host") ||
		std::string::npos != result.find_first_of("100% packet loss")) {
		return false;
	}
	return true;
}
