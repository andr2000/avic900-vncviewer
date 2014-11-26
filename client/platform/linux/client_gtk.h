#ifndef _CLIENT_GTK_H
#define _CLIENT_GTK_H

#include <rfb/rfbclient.h>
#include "client.h"

class Client_Gtk : public Client {
public:
	Client_Gtk();
	virtual ~Client_Gtk();

	static Client *GetInstance_Gtk() {
		if (NULL != Client::GetInstance()) {
			return Client::GetInstance();
		}
		m_Instance = new Client_Gtk();
		return m_Instance;
	};
	void *GetDrawingContext() {
		return NULL;
	};
protected:
	void SetLogging();
	rfbBool OnMallocFrameBuffer(rfbClient *client);
	void OnFrameBufferUpdate(rfbClient *client, int x, int y, int w, int h);
	void OnShutdown();
	long GetTimeMs();
private:
	uint8_t *m_FrameBuffer;

	static void Logger(const char *format, ...);
};

#endif
