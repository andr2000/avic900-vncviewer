#ifndef _CLIENT_GTK_H
#define _CLIENT_GTK_H

#include <rfb/rfbclient.h>
#include "client.h"

class Client_Gtk : public Client {
public:
	Client_Gtk();
	virtual ~Client_Gtk();

protected:
	void SetLogging();
	rfbBool OnMallocFrameBuffer(rfbClient *client);
	void OnShutdown();
	long GetTimeMs();
private:
	uint8_t *m_FrameBuffer;

	static void Logger(const char *format, ...);
};

#endif
