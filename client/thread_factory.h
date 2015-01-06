#ifndef _THREAD_FACTORY_H
#define _THREAD_FACTORY_H

#if defined(WINCE) || defined(WIN32)
#include "thread_wince.h"
#elif __linux__
#include "thread_linux.h"
#else
#error Unsupported OS
#endif


class ThreadFactory {
public:
	static Thread *GetNewThread() {
#if defined(WINCE) || defined(WIN32)
		return new Thread_WinCE();
#elif __linux__
		return new Thread_Linux();
#else
#error Unsupported OS
#endif
	}
};

#endif
