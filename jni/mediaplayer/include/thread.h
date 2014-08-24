#ifndef THREAD_H_
#define THREAD_H_
#include <pthread.h>

class Thread
{
public:
	Thread();
	virtual ~Thread();

	void 			startThread();
	void 			asyncStartThread();
	int 			wait();
	void 			waitOnNotify();
	void 			notify();
	virtual void 	stop();
protected:
	bool 			isRunning;
	virtual void    handleRun(void* pointer);
private:
	pthread_t 		thread;
	pthread_mutex_t lock;
	pthread_cond_t  condition;
	static void* 	startThread(void* pointer);
};
#endif /* THREAD_H_ */
