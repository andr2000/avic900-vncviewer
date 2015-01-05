#ifndef _MUTEX_WINCE_H
#define _MUTEX_WINCE_H

#include <windows.h>

#include "mutex.h"

class Mutex_WinCE : public Mutex {
public:
	Mutex_WinCE();
	~Mutex_WinCE();
	int lock();
	int unlock();
private:
	HANDLE m_Mutex;
};

#endif