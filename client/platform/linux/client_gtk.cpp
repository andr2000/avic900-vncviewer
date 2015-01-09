#include <stdarg.h>

#include <rfb/rfbclient.h>
#include "client_gtk.h"

Client_Gtk::Client_Gtk() :
	Client()
{
	m_FrameBuffer = NULL;
}

Client_Gtk::~Client_Gtk()
{
	if (m_FrameBuffer)
	{
		delete m_FrameBuffer;
	}
}

void Client_Gtk::Logger(const char *format, ...)
{
	va_list args;

	if (!rfbEnableClientLogging)
		return;

	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);
}

void Client_Gtk::SetLogging()
{
	rfbClientLog = Logger;
	rfbClientErr = Logger;
}

rfbBool Client_Gtk::OnMallocFrameBuffer(rfbClient *client)
{
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
	m_FrameBuffer = new uint8_t[width * height * depth / sizeof(uint8_t)];
	client->frameBuffer = m_FrameBuffer;
	return TRUE;
}

void Client_Gtk::OnShutdown()
{
}

long Client_Gtk::GetTimeMs()
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}
