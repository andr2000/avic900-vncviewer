#ifndef _CLIENT_WINCE_H
#define _CLIENT_WINCE_H

#include <rfb/rfbclient.h>
#include "client.h"

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
protected:
	void SetLogging();
	rfbBool OnMallocFrameBuffer(rfbClient *client);
	void OnFrameBufferUpdate(rfbClient *client, int x, int y, int w, int h);
private:
	uint8_t *m_FrameBuffer;
	HBITMAP m_hBmp;
	int m_OrigScreenRotation;
	int m_RotationNeeded;

	static const int LOG_BUF_SZ = 256;
	static void Logger(const char *format, ...);
	int RotateScreen(int angle);
	int GetScreenRotation(int &angle);
	int RotationNeeded(int wnd_width, int wnd_height, int cl_width, int cl_height);
	void RotationSet();
	void RotationRestore();
};

#endif
