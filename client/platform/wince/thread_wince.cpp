#include "thread_wince.h"

Thread_WinCE::Thread_WinCE()
{
	m_Terminated = 0;
	m_Thread = NULL;
	m_ThreadID = -1;
}

Thread_WinCE::~Thread_WinCE()
{
}

int Thread_WinCE::Start()
{
	m_Thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) ThreadFunc,
		static_cast<LPVOID>(this), 0, &m_ThreadID);
	return 0;
}

DWORD WINAPI Thread_WinCE::ThreadFunc(LPVOID lpParam)
{
	Thread_WinCE *thread = reinterpret_cast<Thread_WinCE *>(lpParam);
	thread->Run();
	return 0;
}

void Thread_WinCE::Terminate()
{
	InterlockedIncrement(&m_Terminated);
	/* TODO: deadlock possible if thread func exits faster */
	WaitForSingleObject(m_Thread, /*INFINITE*/2 * 1000);
	CloseHandle(m_Thread);
	m_Thread = NULL;
}

void Thread_WinCE::SleepMs(int ms)
{
	Sleep(ms);
}
