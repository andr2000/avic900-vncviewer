#ifndef _MUTEX_FACTORY_H
#define _MUTEX_FACTORY_H

#if defined(WINCE) || defined(WIN32)
#include "mutex_wince.h"
#elif __linux__
#include "mutex_linux.h"
#else
#error Unsupported OS
#endif


class MutexFactory {
public:
	static Mutex *GetNewMutex() {
#if defined(WINCE) || defined (WIN32)
		return new Mutex_WinCE();
#elif __linux__
		return new Mutex_Linux();
#else
#error Unsupported OS
#endif
	}
};

#endif
