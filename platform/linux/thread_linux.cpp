#include "thread_linux.h"

Thread_Linux::Thread_Linux() {
	m_Terminated = 0;
}

Thread_Linux::~Thread_Linux() {
}

int Thread_Linux::Start() {
	m_Terminated = false;
	m_Thread = std::thread(&Thread_Linux::Run, this);
	return 1;
}

void Thread_Linux::Terminate() {
	if (m_Thread.joinable())
	{
		m_Terminated = true;
		m_Thread.join();
	}
}
