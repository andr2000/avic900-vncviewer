#include "mutex_wince.h"

Mutex_WinCE::Mutex_WinCE() {
	m_Mutex = CreateMutex(0, FALSE, 0);
}

Mutex_WinCE::~Mutex_WinCE() {
}

int Mutex_WinCE::lock() {
	return (WaitForSingleObject(m_Mutex, INFINITE) == WAIT_FAILED ? -1 : 0);
}

int Mutex_WinCE::unlock() {
	return ReleaseMutex(m_Mutex) ? 0 : -1;
}
