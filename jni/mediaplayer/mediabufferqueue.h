#ifndef _MEDIA_BUFFER_QUEUE_H_
#define _MEDIA_BUFFER_QUEUE_H_

#include <pthread.h>
#include <android/log.h>
#include "RefBase.h"

namespace ffmpeg {
    
template <class TYPE>
class MediaBufferQueue : public RefBase
{
public:    
    MediaBufferQueue(TYPE (*duplicate)(void*, TYPE) = NULL, void (*free)(void*, TYPE) = NULL, void* param = NULL, int capability = -1)
    {
	    pthread_mutex_init(&mLock, NULL);
	    pthread_cond_init(&mCondition, NULL);
	    mFirst = NULL;
	    mLast = NULL;
	    mDuplicateFunc = duplicate;
	    mFreeFunc = free;
	    mParam = param;
	    mCapability = capability;
	    mNbBuffers = 0;
        mAbortRequest = false;
        __android_log_print(ANDROID_LOG_DEBUG, "MediaBufferQueue", "MediaBufferQueue %p", this);
    }
    virtual ~MediaBufferQueue()
    {
        __android_log_print(ANDROID_LOG_DEBUG, "MediaBufferQueue", "~MediaBufferQueue %p", this);
	    flush();
	    pthread_mutex_destroy(&mLock);
	    pthread_cond_destroy(&mCondition);
    }

    void flush()
    {
    	MediaBufferList *buffer, *buffer1;
    	
        pthread_mutex_lock(&mLock);
    
        for(buffer = mFirst; buffer != NULL; buffer = buffer1) {
            buffer1 = buffer->mNext;        
            freeBuffer(buffer);		
        }
        mLast = NULL;
        mFirst = NULL;
        mNbBuffers = 0;
    	
        pthread_mutex_unlock(&mLock);
    }

    /* return < 0 if aborted, 0 if full and > 0 if success.  */
	int put(TYPE buffer, bool block = true)
	{
        MediaBufferList* buffer1 = duplicateBuffer(buffer);
        if (!buffer1)
            return -1;
    
        pthread_mutex_lock(&mLock);
    	
    	for(;;) {
    	    if (mAbortRequest) {
    	        freeBuffer(buffer1, mDuplicateFunc != NULL);
    	        pthread_mutex_unlock(&mLock);
                return -1;
            }
            if (mCapability > 0 && mNbBuffers >= mCapability) {
                if (block) {
                    pthread_cond_wait(&mCondition, &mLock);
                } else {
                    freeBuffer(buffer1, mDuplicateFunc != NULL);
                    pthread_mutex_unlock(&mLock);
                    return 0;
                }
            } else {
                break;
            }
    	}
        if (!mLast) {
            mFirst = buffer1;
    	}
        else {
            mLast->mNext = buffer1;
    	}
        mLast = buffer1;
        mNbBuffers++;    
    
    	pthread_cond_broadcast(&mCondition);
        pthread_mutex_unlock(&mLock);
    	
        return 1;	    
	}
	
	/* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
	int get(TYPE& buffer, bool block = true)
	{
    	MediaBufferList *buffer1;
        int ret;
    	
        pthread_mutex_lock(&mLock);
    	
        for(;;) {
            if (mAbortRequest) {
                ret = -1;
                break;
            }
    		
            buffer1 = mFirst;
            if (buffer1) {
                mFirst = buffer1->mNext;
                if (!mFirst)
                    mLast = NULL;
                mNbBuffers--;
                buffer = buffer1->mData;
                freeBuffer(buffer1, false);
                pthread_cond_broadcast(&mCondition);            
                ret = 1;
                break;
            } else if (!block) {
                ret = 0;
                break;
            } else {
    			pthread_cond_wait(&mCondition, &mLock);
    		}
        }
        pthread_mutex_unlock(&mLock);
        return ret;	    
	}
	
	int size()
    {
        return mNbBuffers;
    }
	
	void reset()
	{
	    flush();
	    mAbortRequest = false;
	}
	    
	void abort()
	{
	    pthread_mutex_lock(&mLock);
        mAbortRequest = true;
        pthread_cond_broadcast(&mCondition);
        pthread_mutex_unlock(&mLock);
	}

private:
    struct MediaBufferList
    {
        TYPE mData;
        MediaBufferList* mNext;
    };
    MediaBufferList* duplicateBuffer(TYPE buffer)
    {
        MediaBufferList* buffer1 = (MediaBufferList*)malloc(sizeof(MediaBufferList));
        if (!buffer1)
            return NULL;
        if (mDuplicateFunc) {
            buffer1->mData = mDuplicateFunc(mParam, buffer);
            if (!buffer1->mData) {
                free(buffer1);
                return NULL;
            }
            buffer1->mNext = NULL;
        } else {
            buffer1->mData = buffer;
            buffer1->mNext = NULL;
        }
        return buffer1;
    }
    
    void freeBuffer(MediaBufferList* buffer, bool freeData=true)
    {
        if (freeData && mFreeFunc) {
            mFreeFunc(mParam, buffer->mData);
        }
		buffer->mData = NULL;
        free(buffer);
    }    

private:
	MediaBufferList*		mFirst;
	MediaBufferList*		mLast;
	void(*mFreeFunc)(void*, TYPE);
	TYPE(*mDuplicateFunc)(void*, TYPE);
	void*                   mParam;
    int					mNbBuffers;
    int                 mCapability;
    bool				mAbortRequest;
	pthread_mutex_t     mLock;
	pthread_cond_t		mCondition;
};

}

#endif //_MEDIA_BUFFER_QUEUE_H_
