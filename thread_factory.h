#ifndef _THREAD_FACTORY_H
#define _THREAD_FACTORY_H

#ifdef WINCE
#include "thread_wince.h"
#else
#error Unsupported OS
#endif


class ThreadFactory {
public:
	static Thread *GetNewThread() {
#ifdef WINCE
		return new Thread_WinCE();
#else
#error Unsupported OS
#endif
	}
};

#endif
