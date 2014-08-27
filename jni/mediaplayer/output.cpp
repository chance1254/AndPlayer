#include <android/log.h>
#include "output.h"

#define TAG "FFMpegOutput"


static ffmpeg_vo_t *vo = NULL;
static ffmpeg_ao_t *ao = NULL;

extern "C" int ffmpeg_vo_register(ffmpeg_vo_t *_vo)
{
    vo = _vo;
    return 0;
}

extern "C" int ffmpeg_ao_register(ffmpeg_ao_t *_ao)
{
    ao = _ao;
    return 0;
}

//-------------------- Audio driver --------------------

int Output::AudioDriver_open(AUDIO_DRIVER_HANDLE* handle, uint32_t sampleRate, int format, int channels)
{
    if (ao && ao->ffmpeg_ao_open)
        return ao->ffmpeg_ao_open(handle, sampleRate, format, channels);
    return ANDROID_AUDIOTRACK_RESULT_ERRNO;
}

int Output::AudioDriver_close(AUDIO_DRIVER_HANDLE handle)
{
    if (ao && ao->ffmpeg_ao_close)
        return ao->ffmpeg_ao_close(handle);
    return ANDROID_AUDIOTRACK_RESULT_ERRNO;
}

int Output::AudioDriver_start(AUDIO_DRIVER_HANDLE handle)
{
    if (ao && ao->ffmpeg_ao_start)
        return ao->ffmpeg_ao_start(handle);
    return ANDROID_AUDIOTRACK_RESULT_ERRNO;
}

uint32_t Output::AudioDriver_latency(AUDIO_DRIVER_HANDLE handle)
{
    if (ao && ao->ffmpeg_ao_latency)
        return ao->ffmpeg_ao_latency(handle);
    return ANDROID_AUDIOTRACK_RESULT_ERRNO;
}

int Output::AudioDriver_flush(AUDIO_DRIVER_HANDLE handle)
{
    if (ao && ao->ffmpeg_ao_flush)
        return ao->ffmpeg_ao_flush(handle);
    return ANDROID_AUDIOTRACK_RESULT_ERRNO;
}

int Output::AudioDriver_stop(AUDIO_DRIVER_HANDLE handle)
{
    if (ao && ao->ffmpeg_ao_stop)
        return ao->ffmpeg_ao_stop(handle);
    return ANDROID_AUDIOTRACK_RESULT_ERRNO;
}

int Output::AudioDriver_reload(AUDIO_DRIVER_HANDLE handle)
{
    if (ao && ao->ffmpeg_ao_reload)
        return ao->ffmpeg_ao_reload(handle);
    return ANDROID_AUDIOTRACK_RESULT_ERRNO;
}

int Output::AudioDriver_write(AUDIO_DRIVER_HANDLE handle, void *buffer, int buffer_size)
{
    if (ao && ao->ffmpeg_ao_write)
        return ao->ffmpeg_ao_write(handle, buffer, buffer_size);
    return ANDROID_AUDIOTRACK_RESULT_ERRNO;
}

//-------------------- Video driver --------------------

int Output::VideoDriver_open(VIDEO_DRIVER_HANDLE* handle)
{
    if (vo && vo->ffmpeg_vo_open)
        return vo->ffmpeg_vo_open(handle);
    return ANDROID_SURFACE_RESULT_NOT_VALID;
}

int Output::VideoDriver_close(VIDEO_DRIVER_HANDLE handle)
{
    if (vo && vo->ffmpeg_vo_close)
        return vo->ffmpeg_vo_close(handle);
    return ANDROID_SURFACE_RESULT_NOT_VALID;   
}
    
	
int Output::VideoDriver_setSurface(VIDEO_DRIVER_HANDLE handle, void* env, void *surface)
{
    if (vo && vo->ffmpeg_vo_setSurface)
        return vo->ffmpeg_vo_setSurface(handle, env, surface);
    return ANDROID_SURFACE_RESULT_NOT_VALID;
}

int Output::VideoDriver_lockSurface(VIDEO_DRIVER_HANDLE handle, int* width, int* height, int* pixelformat, int* stride, void** pixels)
{
    if (vo && vo->ffmpeg_vo_lockSurface)
        return vo->ffmpeg_vo_lockSurface(handle, width, height, pixelformat, stride, pixels);
    return ANDROID_SURFACE_RESULT_NOT_VALID;
}

int Output::VideoDriver_unlockSurface(VIDEO_DRIVER_HANDLE handle)
{
    if (vo && vo->ffmpeg_vo_unlockSurface)
        return vo->ffmpeg_vo_unlockSurface(handle);
    return ANDROID_SURFACE_RESULT_NOT_VALID;
}
