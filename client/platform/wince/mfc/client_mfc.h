#ifndef _CLIENT_MFC_H
#define _CLIENT_MFC_H

#include "client_wince.h"

class Client_MFC : public Client_WinCE {
public:
	static Client *GetInstance_MFC() {
		if (NULL != Client_WinCE::GetInstance()) {
			return Client_WinCE::GetInstance();
		}
		m_Instance = new Client_MFC();
		return m_Instance;
	};

	void ShowFullScreen();
	void OnPaint(void);

protected:
	void OnFrameBufferUpdate(rfbClient *client, int x, int y, int w, int h);

	#ifdef SHOW_POINTER_TRACE
	enum trace_point_type_e {
		TRACE_POINT_DOWN,
		TRACE_POINT_UP,
		TRACE_POINT_MOVE
	};
	struct trace_point_t {
		LONG x;
		LONG y;
		trace_point_type_e type;
	};
	static const int TRACE_POINT_BAR_SZ = 10;
	std::deque< trace_point_t > m_TraceQueue;
	void AddTracePoint(trace_point_type_e type, LONG x, LONG y);
#endif

};

#endif
