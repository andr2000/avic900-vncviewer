#ifndef _MUTEX_LINUX_H
#define _MUTEX_LINUX_H

#include <mutex>

#include "mutex.h"

class Mutex_Linux : public Mutex {
public:
	Mutex_Linux();
	~Mutex_Linux();
	int lock();
	int unlock();
private:
	std::mutex m_Mutex;
};

#endif
