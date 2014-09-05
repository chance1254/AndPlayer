#include <dlfcn.h>
#include <assert.h>

#include <android/native_window_jni.h>
#include <android/log.h>
#define TAG "ffmpeg_vo_surface"

#define LOGI(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)

#include "android/surface.h"

static ffmpeg_vo_t *vo = NULL;

class AndroidSurface
{
private:
    ANativeWindow* mNativeSurface;
public:
    AndroidSurface()
        : mNativeSurface(NULL)
    {

    }
    ~AndroidSurface()
    {
		if (mNativeSurface)
		{
			ANativeWindow_release(mNativeSurface);
			mNativeSurface = NULL;
		}
    }
    int setSurface(JNIEnv* env, void* surf);
    int lockSurface(int* width, int* height, int* pixelformat, int* stride, void** pixels);
    int unlockSurface();
};

int AndroidSurface::setSurface(JNIEnv* env, void* surf)
{
    if(mNativeSurface) {
        ANativeWindow_release(mNativeSurface);
        mNativeSurface = NULL;
    }

    if(surf == NULL){
        mNativeSurface = NULL;
    } else {
        jobject jsurface = (jobject)surf;
        mNativeSurface = ANativeWindow_fromSurface(env, jsurface);
    }
    LOGI("setSurface: mNativeSurface = %p\n", mNativeSurface);
    return ANDROID_SURFACE_RESULT_SUCCESS;
}

int AndroidSurface::lockSurface(int* width, int* height, int* pixelformat, int* stride, void** pixels)
{
    int status = ANDROID_SURFACE_RESULT_NOT_VALID;
    ANativeWindow_Buffer info;

    if (!mNativeSurface)
    	return ANDROID_SURFACE_RESULT_NOT_VALID;
	ANativeWindow_lock(mNativeSurface, &info, NULL);
    //if (status == ANDROID_SURFACE_RESULT_SUCCESS)
	{
    	*width = info.width;
    	*height = info.height;
    	*pixelformat = info.format;
    	*pixels = info.bits;
    	*stride = info.stride;
    }
    //LOGI("AndroidSurface_getPixels: status = %d, {%d, %d, %x, %d, %p}\n", status, info.width, info.height, info.format, info.stride, info.bits);
    return ANDROID_SURFACE_RESULT_SUCCESS;
}

int AndroidSurface::unlockSurface()
{
    if (!mNativeSurface)
    	return ANDROID_SURFACE_RESULT_NOT_VALID;
    int status = ANativeWindow_unlockAndPost(mNativeSurface);
    //LOGI("AndroidSurface_updateSurface: status = %d\n", status);
    return status;
}

static int ffmpeg_vo_open(VIDEO_DRIVER_HANDLE* outHandle)
{
	*outHandle = new AndroidSurface();
	return ANDROID_SURFACE_RESULT_SUCCESS;
}

static int ffmpeg_vo_close(VIDEO_DRIVER_HANDLE handle)
{
    AndroidSurface* surface = (AndroidSurface*)handle;
    if (!surface)
        return ANDROID_SURFACE_RESULT_NOT_VALID;
    delete surface;
	return ANDROID_SURFACE_RESULT_SUCCESS;
}

static int ffmpeg_vo_setSurface(VIDEO_DRIVER_HANDLE handle, void* env, void* surf)
{
    AndroidSurface* surface = (AndroidSurface*)handle;
    if (!surface)
        return ANDROID_SURFACE_RESULT_NOT_VALID;
    return surface->setSurface((JNIEnv*)env, surf);
}

static int ffmpeg_vo_lockSurface(VIDEO_DRIVER_HANDLE handle, int* width, int* height, int* pixelformat, int* stride, void** pixels)
{
    AndroidSurface* surface = (AndroidSurface*)handle;
    if (!surface)
        return ANDROID_SURFACE_RESULT_NOT_VALID;
    return surface->lockSurface(width, height, pixelformat, stride, pixels);
}

static int ffmpeg_vo_unlockSurface(VIDEO_DRIVER_HANDLE handle)
{
    AndroidSurface* surface = (AndroidSurface*)handle;
    if (!surface)
        return ANDROID_SURFACE_RESULT_NOT_VALID;
    return surface->unlockSurface();
}


static ffmpeg_vo_t ffmpeg_vo =
{
        ffmpeg_vo_open,
        ffmpeg_vo_close,
        ffmpeg_vo_setSurface,
        ffmpeg_vo_lockSurface,
        ffmpeg_vo_unlockSurface
};

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    jint result = JNI_ERR;

    ffmpeg_vo_register(&ffmpeg_vo);

    LOGI("loaded");
    result = JNI_VERSION_1_4;


    return result;
}

int ffmpeg_vo_register(ffmpeg_vo_t *_vo)
{
    vo = _vo;
    return 0;
}
