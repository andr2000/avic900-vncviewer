#ifndef _MUTEX_H
#define _MUTEX_H

class Mutex {
public:
	Mutex() {
	};
	virtual ~Mutex() {
	};
	virtual int lock() = 0;
	virtual int unlock() = 0;
};

#endif