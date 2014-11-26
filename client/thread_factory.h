#ifndef _THREAD_FACTORY_H
#define _THREAD_FACTORY_H

#ifdef WINCE
#include "thread_wince.h"
#elif __linux__
#include "thread_linux.h"
#else
#error Unsupported OS
#endif


class ThreadFactory {
public:
	static Thread *GetNewThread() {
#ifdef WINCE
		return new Thread_WinCE();
#elif __linux__
		return new Thread_Linux();
#else
#error Unsupported OS
#endif
	}
};

#endif
