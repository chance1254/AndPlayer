#include <android/log.h>
#include "../include/thread.h"
#define TAG "FFMpegThread"

Thread::Thread()
{
	__android_log_print(ANDROID_LOG_INFO, TAG, "mutex init");
	pthread_mutex_init(&lock, NULL);
	pthread_cond_init(&condition, NULL);
}

Thread::~Thread()
{
}

void Thread::startThread()
{
	handleRun(NULL);
}

void Thread::asyncStartThread()
{
	__android_log_print(ANDROID_LOG_INFO, TAG, "create thread");
	pthread_create(&thread, NULL, startThread, this);
}

int Thread::wait()
{
	if(!isRunning)
	{
		return 0;
	}

	return pthread_join(thread, NULL);
}

void Thread::stop()
{
	isRunning = false;
}

void* Thread::startThread(void* ptr)
{
	 __android_log_print(ANDROID_LOG_INFO, TAG, "starting thread");
	 Thread* thread = (Thread *)ptr;
	 thread->isRunning = true;
	 thread->handleRun(ptr);
	 thread->isRunning = false;
	 __android_log_print(ANDROID_LOG_INFO, TAG, "thread ended");
}

void Thread::waitOnNotify()
{
	pthread_mutex_lock(&lock);
	pthread_cond_wait(&condition, &lock);
	pthread_mutex_unlock(&lock);
}

void Thread::notify()
{
	pthread_mutex_lock(&lock);
	pthread_cond_signal(&condition);
	pthread_mutex_unlock(&lock);
}

void Thread::handleRun(void* ptr)
{
}
