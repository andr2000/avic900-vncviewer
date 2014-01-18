#ifndef _CLIENT_WINCE_H
#define _CLIENT_WINCE_H

#include <rfb/rfbclient.h>
#include "client.h"
#include "compat.h"

class Client_WinCE : public Client {
public:
	Client_WinCE();
	virtual ~Client_WinCE();

	static Client *GetInstance_WinCE() {
		if (NULL != Client::GetInstance()) {
			return Client::GetInstance();
		}
		m_Instance = new Client_WinCE();
		return m_Instance;
	};
	void *GetDrawingContext() {
		return static_cast<void *>(m_hBmp);
	};
	bool IsServerAlive(std::string &host);
protected:
	void SetLogging();
	rfbBool OnMallocFrameBuffer(rfbClient *client);
	void OnFrameBufferUpdate(rfbClient *client, int x, int y, int w, int h);
private:
	uint8_t *m_FrameBuffer;
	HBITMAP m_hBmp;

	static const int LOG_BUF_SZ = 256;
	static void Logger(const char *format, ...);
};

#endif
