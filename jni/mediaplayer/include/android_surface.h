/*
 * video_output.h
 *
 *  Created on: 26 рту. 2014 у.
 *      Author: 1
 */

#ifndef VIDEO_OUTPUT_H_
#define VIDEO_OUTPUT_H_
#include <stdint.h>

typedef void* VIDEO_DRIVER_HANDLE;
#ifndef ANDROID_SYM_S_LOCK
# define ANDROID_SYM_S_LOCK "_ZN7android7Surface4lockEPNS0_11SurfaceInfoEb"
#endif
#ifndef ANDROID_SYM_S_LOCK2
# define ANDROID_SYM_S_LOCK2 "_ZN7android7Surface4lockEPNS0_11SurfaceInfoEPNS_6RegionE"
#endif
#ifndef ANDROID_SYM_S_UNLOCK
# define ANDROID_SYM_S_UNLOCK "_ZN7android7Surface13unlockAndPostEv"
#endif
#define ANDROID_SURFACE_RESULT_SUCCESS                                           0
#define ANDROID_SURFACE_RESULT_NOT_VALID                                        -1
#define ANDROID_SURFACE_RESULT_COULDNT_LOCK                                     -2
#define ANDROID_SURFACE_RESULT_COULDNT_UNLOCK_AND_POST          -3
#define ANDROID_SURFACE_RESULT_COULDNT_INIT_BITMAP_SURFACE      -4
#define ANDROID_SURFACE_RESULT_COULDNT_INIT_BITMAP_CLIENT       -5
#define ANDROID_SURFACE_RESULT_JNI_EXCEPTION                            -6

// _ZN7android7Surface4lockEPNS0_11SurfaceInfoEb
typedef int (*Surface_lock)(void *, void *, bool);
// _ZN7android7Surface4lockEPNS0_11SurfaceInfoEPNS_6RegionE
typedef int (*Surface_lock2)(void *, void *, void *);
// _ZN7android7Surface13unlockAndPostEv
typedef int (*Surface_unlockAndPost)(void *);

static Surface_lock s_lock;
static Surface_lock2 s_lock2;
static Surface_unlockAndPost s_unlockAndPost;
static void* library;

class AndroidSurface
{
private:
	void* 					mNativeSurface;
public:
	AndroidSurface();
	~AndroidSurface();
	int 					setSurface(void* surface);
	int 					lockSurface(int* width, int* height, int* pixelformat, int* stride, void** pixels);
	int 					unlockSurface();
	int 					ffmpegVideoOutputOpen(VIDEO_DRIVER_HANDLE* outputHandle);
	int 					ffmpegVideoOutputClose(VIDEO_DRIVER_HANDLE handle);
	int 					ffmpegVideoOutputSetSurface(VIDEO_DRIVER_HANDLE handle, void* surf);
	int 					ffmpegVideoOutputLockSurface(VIDEO_DRIVER_HANDLE handle, int* width,
															int* height, int* pixelformat, int* stride, void** pixels);
	int 					ffmpegVideoOutputUnlockSurface(VIDEO_DRIVER_HANDLE handle);
	static inline void 		*loadSurface(const char *psz_lib);
	static void 			*initLibrary();
};

typedef struct {
	uint32_t width;
	uint32_t height;
	uint32_t stride;
	uint32_t usage;
	uint32_t format;
	uint32_t* bits;
	uint32_t reserved[2];

} SurfaceInfo;
#endif /* VIDEO_OUTPUT_H_ */
