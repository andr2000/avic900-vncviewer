#include "mutex_linux.h"

Mutex_Linux::Mutex_Linux() {
}

Mutex_Linux::~Mutex_Linux() {
}

int Mutex_Linux::lock() {
	m_Mutex.lock();
	return 1;
}

int Mutex_Linux::unlock() {
	m_Mutex.unlock();
	return 1;
}
