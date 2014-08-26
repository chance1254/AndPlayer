#include "../include/android_surface.h"
#include <android/log.h>
#include <cstdlib>
#define TAG "VideoOutput.cpp"
namespace android{}
using namespace android;

AndroidSurface::AndroidSurface() {
	mNativeSurface = NULL;
}

AndroidSurface::~AndroidSurface() {
	if (mNativeSurface != NULL) {
		free(mNativeSurface);
	}
}

int AndroidSurface::setSurface(void* surface) {
	mNativeSurface = surface;
	__android_log_print(ANDROID_LOG_INFO, TAG, "Set surface");
	return ANDROID_SURFACE_RESULT_SUCCESS;
}

int AndroidSurface::lockSurface(int* width, int* height, int* pixelformat,
		int* stride, void** pixels) {
	int status = ANDROID_SURFACE_RESULT_NOT_VALID;
	SurfaceInfo info;

	if (!mNativeSurface || (!s_lock && !s_lock2))
		return ANDROID_SURFACE_RESULT_NOT_VALID;
	if (s_lock) {
		status = s_lock(mNativeSurface, &info, true);
	} else if (s_lock2) {
		status = s_lock2(mNativeSurface, &info, NULL);
	}
	if (status == ANDROID_SURFACE_RESULT_SUCCESS) {
		*width = info.width;
		*height = info.height;
		*pixelformat = info.format;
		*pixels = info.bits;
		*stride = info.stride;
	}
	//LOGI("AndroidSurface_getPixels: status = %d, {%d, %d, %x, %d, %p}\n", status, info.w, info.h, info.format, info.s, info.bits);
	return status;
}

int AndroidSurface::unlockSurface() {
	if (!mNativeSurface || (!s_unlockAndPost))
		return ANDROID_SURFACE_RESULT_NOT_VALID;
	int status = s_unlockAndPost(mNativeSurface);
	//LOGI("AndroidSurface_updateSurface: status = %d\n", status);
	return status;
}

int AndroidSurface::ffmpegVideoOutputOpen(VIDEO_DRIVER_HANDLE* outHandle) {
	*outHandle = NULL;
	if (!library) {
		return ANDROID_SURFACE_RESULT_NOT_VALID;
	}
	*outHandle = new AndroidSurface();
	return ANDROID_SURFACE_RESULT_SUCCESS;
}

int AndroidSurface::ffmpegVideoOutputClose(VIDEO_DRIVER_HANDLE handle) {
	AndroidSurface* surface = (AndroidSurface*) handle;
	if (!surface)
		return ANDROID_SURFACE_RESULT_NOT_VALID;
	free(surface);
	return ANDROID_SURFACE_RESULT_SUCCESS;
}

int AndroidSurface::ffmpegVideoOutputSetSurface(VIDEO_DRIVER_HANDLE handle,
		void* surf) {
	AndroidSurface* surface = (AndroidSurface*) handle;
	if (!surface)
		return ANDROID_SURFACE_RESULT_NOT_VALID;
	return surface->setSurface(surf);
}

int AndroidSurface::ffmpegVideoOutputLockSurface(VIDEO_DRIVER_HANDLE handle, int* width, int* height, int* pixelformat, int* stride, void** pixels)
{
    AndroidSurface* surface = (AndroidSurface*)handle;
    if (!surface)
        return ANDROID_SURFACE_RESULT_NOT_VALID;
    return surface->lockSurface(width, height, pixelformat, stride, pixels);
}

int AndroidSurface::ffmpegVideoOutputUnlockSurface(VIDEO_DRIVER_HANDLE handle)
{
    AndroidSurface* surface = (AndroidSurface*)handle;
    if (!surface)
        return ANDROID_SURFACE_RESULT_NOT_VALID;
    return surface->unlockSurface();
}
