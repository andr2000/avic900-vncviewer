#ifndef _MUTEX_FACTORY_H
#define _MUTEX_FACTORY_H

#ifdef WINCE
#include "mutex_wince.h"
#else
#error Unsupported OS
#endif


class MutexFactory {
public:
	static Mutex *GetNewMutex() {
#ifdef WINCE
		return new Mutex_WinCE();
#else
#error Unsupported OS
#endif
	}
};

#endif
