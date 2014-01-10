#ifndef _THREAD_H
#define _THREAD_H

class Thread {
public:
	typedef int ThreadWorkerFunc(void *data);

	Thread();
	virtual ~Thread();
	void SetWorker(ThreadWorkerFunc &worker, void *context);
	virtual int Start() = 0;
protected:
	virtual int ShouldStop() = 0;
	virtual void Terminate() = 0;
	int Run();
private:
	ThreadWorkerFunc *m_Worker;
	void *m_Context;
};

#endif