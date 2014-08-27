#include <android/log.h>
#include "thread.h"

#define TAG "FFMpegThread"

Thread::Thread() : mRunning(false), mPausing(false), mThread(0)
{
	pthread_mutex_init(&mLock, NULL);
	pthread_cond_init(&mCondition, NULL);
}

Thread::~Thread()
{
}

void Thread::start()
{
    startAsync();
}

void Thread::startAsync()
{
    pthread_create(&mThread, NULL, startThread, this);
}

int Thread::wait()
{
	__android_log_print(ANDROID_LOG_INFO, TAG, "join thread enter, %x", mThread);
    int status = pthread_join(mThread, NULL);
	__android_log_print(ANDROID_LOG_INFO, TAG, "join thread exit, %x", mThread);
	return status;
}

void Thread::delay(int milliseconds)
{
	if (milliseconds > 0) {
		usleep(milliseconds * 1000);
	}
}

void Thread::stop()
{
	mRunning = false;
}

void Thread::pause()
{
	mPausing = true;
}

void Thread::resume()
{
	mPausing = false;
}
	
void* Thread::startThread(void* ptr)
{
	Thread* thread = (Thread *) ptr;
    __android_log_print(ANDROID_LOG_INFO, TAG, "starting thread, %x", thread->mThread);
	thread->mRunning = true;
    thread->handleRun(ptr);
	thread->mRunning = false;
	__android_log_print(ANDROID_LOG_INFO, TAG, "thread ended, %x", thread->mThread);
}

void Thread::waitOnNotify()
{
	pthread_mutex_lock(&mLock);
	pthread_cond_wait(&mCondition, &mLock);
	pthread_mutex_unlock(&mLock);
}

void Thread::notify()
{
	pthread_mutex_lock(&mLock);
	pthread_cond_signal(&mCondition);
	pthread_mutex_unlock(&mLock);
}

void Thread::handleRun(void* ptr)
{
}
