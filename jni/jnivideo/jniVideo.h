/*
 * jniVideo.h
 *
 *  Created on: 27 рту. 2014 у.
 *      Author: 1
 */

#ifndef JNIVIDEO_H_
#define JNIVIDEO_H_
#include <dlfcn.h>
#include <assert.h>

#include <android/native_window_jni.h>
#include <android/log.h>
#define TAG "ffmpeg_vo_surface"
#define LOGI(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)

#include "../mediaplayer/surface.h"

class AndroidSurface
{
public:
	AndroidSurface();
	~AndroidSurface();
	int setSurface(void* surf);
	int lockSurface(int* width, int* height, int* pixelformat, int* stride, void** pixels);
	int unlockSurface();
private:
	void* mNativeSurface;
};

#endif /* JNIVIDEO_H_ */
