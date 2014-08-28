#include "jniVideo.h"
extern "C"{
#include "../mediaplayer/surface.h"
}
#include <dlfcn.h>
#include <assert.h>
#include <stdlib.h>

#include <android/log.h>
#define TAG "ffmpeg_vo_surface"
#define LOGI(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#ifndef ANDROID_SYM_S_LOCK
# define ANDROID_SYM_S_LOCK "_ZN7android7Surface4lockEPNS0_11SurfaceInfoEb"
#endif
#ifndef ANDROID_SYM_S_LOCK2
# define ANDROID_SYM_S_LOCK2 "_ZN7android7Surface4lockEPNS0_11SurfaceInfoEPNS_6RegionE"
#endif
#ifndef ANDROID_SYM_S_UNLOCK
# define ANDROID_SYM_S_UNLOCK "_ZN7android7Surface13unlockAndPostEv"
#endif

// _ZN7android7Surface4lockEPNS0_11SurfaceInfoEb
typedef int (*Surface_lock)(void *, void *, bool);
// _ZN7android7Surface4lockEPNS0_11SurfaceInfoEPNS_6RegionE
typedef int (*Surface_lock2)(void *, void *, void *);
// _ZN7android7Surface13unlockAndPostEv
typedef int (*Surface_unlockAndPost)(void *);

typedef struct _SurfaceInfo {
	uint32_t w;
	uint32_t h;
	uint32_t s;
	uint32_t usage;
	uint32_t format;
	uint32_t* bits;
	uint32_t reserved[2];
} SurfaceInfo;

static Surface_lock s_lock;
static Surface_lock2 s_lock2;
static Surface_unlockAndPost s_unlockAndPost;
static void* library;

static inline void* loadSurface(const char* psz_lib) {
	void *p_library = dlopen(psz_lib, RTLD_NOW);
	LOGI("%s: %p\n", psz_lib, p_library);
	if (p_library) {
		s_lock = (Surface_lock) (dlsym(p_library, ANDROID_SYM_S_LOCK));
		s_lock2 = (Surface_lock2) (dlsym(p_library, ANDROID_SYM_S_LOCK2));
		s_unlockAndPost = (Surface_unlockAndPost) (dlsym(p_library,
		ANDROID_SYM_S_UNLOCK));
		LOGI("%p, %p, %p\n", s_lock, s_lock2, s_unlockAndPost);
		if ((s_lock || s_lock2) && s_unlockAndPost) {
			return p_library;
		}
		dlclose(p_library);
	}

	return NULL;
}

static inline void* initLibrary() {
	void *p_library;
	if ((p_library = loadSurface("libsurfaceflinger_client.so")))
		return p_library;
	if ((p_library = loadSurface("libgui.so")))
		return p_library;
	return loadSurface("libui.so");
}

AndroidSurface::AndroidSurface() :
		mNativeSurface(NULL) {
}

AndroidSurface::~AndroidSurface() {
	if (mNativeSurface != NULL)
		free(mNativeSurface);
}

int AndroidSurface::setSurface(void* surface) {
	mNativeSurface = surface;
	LOGI("setSurface: mNativeSurface = %p\n", mNativeSurface);
	return ANDROID_SURFACE_RESULT_SUCCESS;
}

int AndroidSurface::lockSurface(int* width, int* height, int* format,
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
		*width = info.w;
		*height = info.h;
		*format = info.format;
		*pixels = info.bits;
		*stride = info.s;
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

static int ffmpeg_vo_open(VIDEO_DRIVER_HANDLE* outHandle) {
	*outHandle = NULL;
	if (!library) {
		return ANDROID_SURFACE_RESULT_NOT_VALID;
	}
	*outHandle = new AndroidSurface();
	return ANDROID_SURFACE_RESULT_SUCCESS;
}

static int ffmpeg_vo_setSurface(VIDEO_DRIVER_HANDLE handle,
		void* env, void* surf) {
	AndroidSurface* surface = (AndroidSurface*) handle;
	if (!surface)
		return ANDROID_SURFACE_RESULT_NOT_VALID;
	return surface->setSurface(surf);
}

static int ffmpeg_vo_lockSurface(VIDEO_DRIVER_HANDLE handle,
		int* width, int* height, int* pixelformat, int* stride, void** pixels) {
	AndroidSurface* surface = (AndroidSurface*) handle;
	if (!surface)
		return ANDROID_SURFACE_RESULT_NOT_VALID;
	return surface->lockSurface(width, height, pixelformat, stride, pixels);
}

static int ffmpeg_vo_unlockSurface(VIDEO_DRIVER_HANDLE handle) {
	AndroidSurface* surface = (AndroidSurface*) handle;
	if (!surface)
		return ANDROID_SURFACE_RESULT_NOT_VALID;
	return surface->unlockSurface();
}

static int ffmpeg_vo_close(VIDEO_DRIVER_HANDLE handle)
{
    AndroidSurface* surface = (AndroidSurface*)handle;
    if (!surface)
        return ANDROID_SURFACE_RESULT_NOT_VALID;
    delete surface;
        return ANDROID_SURFACE_RESULT_SUCCESS;
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

    library = initLibrary();
    ffmpeg_vo_register(&ffmpeg_vo);

    LOGI("loaded");
    result = JNI_VERSION_1_4;


    return result;
}

