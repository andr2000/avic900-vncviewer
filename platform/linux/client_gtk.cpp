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
	fprintf(stdout, format, args);
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
	rfbClientLog("OnMallocFrameBuffer: w=%d h=%d d=%d\r\n", width, height, depth);
	client->updateRect.x = 0;
	client->updateRect.y = 0;
	client->updateRect.w = width;
	client->updateRect.h = height;

	/* create frambuffer */
	client->frameBuffer = m_FrameBuffer;
	return TRUE;
}

void Client_Gtk::OnFrameBufferUpdate(rfbClient* client, int x, int y, int w, int h) {
	rfbClientLog("OnFrameBufferUpdate: x=%d y=%d w=%d h=%d\r\n", x, y, w, h);
}
