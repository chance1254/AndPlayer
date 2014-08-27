/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_SURFACE_WRAPPER_H
#define ANDROID_SURFACE_WRAPPER_H

#include <stdint.h>
#include <jni.h>

#define ANDROID_SURFACE_RESULT_SUCCESS						 0
#define ANDROID_SURFACE_RESULT_NOT_VALID					-1
#define ANDROID_SURFACE_RESULT_COULDNT_LOCK					-2
#define ANDROID_SURFACE_RESULT_COULDNT_UNLOCK_AND_POST		-3
#define ANDROID_SURFACE_RESULT_COULDNT_INIT_BITMAP_SURFACE	-4
#define ANDROID_SURFACE_RESULT_COULDNT_INIT_BITMAP_CLIENT	-5
#define ANDROID_SURFACE_RESULT_JNI_EXCEPTION				-6

#ifdef __cplusplus
extern "C" {
#endif

typedef void* VIDEO_DRIVER_HANDLE;

typedef struct _ffmpeg_vo_t_ 
{
    int (*ffmpeg_vo_open)(VIDEO_DRIVER_HANDLE* outHandle);
    int (*ffmpeg_vo_close)(VIDEO_DRIVER_HANDLE handle);
    int (*ffmpeg_vo_setSurface)(VIDEO_DRIVER_HANDLE handle, void* env, void* surface);
    int (*ffmpeg_vo_lockSurface)(VIDEO_DRIVER_HANDLE handle, int* width, int* height, int* pixelformat, int* stride, void** pixels);
    int (*ffmpeg_vo_unlockSurface)(VIDEO_DRIVER_HANDLE handle);
} ffmpeg_vo_t, *pffmpeg_vo_t;

extern int ffmpeg_vo_register(ffmpeg_vo_t* vo);

#ifdef __cplusplus
}
#endif

#endif

