#ifndef _THREAD_WINCE_H
#define _THREAD_WINCE_H

#include <winbase.h>

#include "thread.h"

class Thread_WinCE : public Thread {
public:
	Thread_WinCE();
	~Thread_WinCE();
protected:
	int Start();
	void Terminate();
	int ShouldStop() {
		return InterlockedExchange(&m_Terminated, m_Terminated);
	}
private:
	LONG m_Terminated;
	HANDLE m_Thread;;
	DWORD m_ThreadID;

	static DWORD WINAPI ThreadFunc(LPVOID lpParam);
};

#endif