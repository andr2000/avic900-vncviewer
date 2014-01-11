#include <stdlib.h>

#include "mutex.h"

Thread::Thread() {
	m_Worker = NULL;
	m_Context = NULL;
}

Thread::~Thread() {
}

void Thread::SetWorker(ThreadWorkerFunc &worker, void *context) {
	m_Worker = worker;
	m_Context = context;
}

int Thread::Run() {
	int result;

	while (!ShouldStop()) {
		result = m_Worker(m_Context);
		if (result < 0) {
			return result;
		}
	}
	return 0;
}
