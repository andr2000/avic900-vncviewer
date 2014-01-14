#ifndef _THREAD_LINUX_H
#define _THREAD_LINUX_H

#include <atomic>
#include <thread>

#include "thread.h"

class Thread_Linux : public Thread {
public:
	Thread_Linux();
	~Thread_Linux();
protected:
	int Start();
	void Terminate();
	int ShouldStop() {
		return m_Terminated;
	}
private:
	std::atomic<bool> m_Terminated;
	std::thread m_Thread;;
};

#endif
