#ifndef FFMPEG_OUTPUT_H
#define FFMPEG_OUTPUT_H

#include <jni.h>

#include "audiotrack.h"
#include "surface.h"
#include "Errors.h"

class Output
{
public:	
	static int					AudioDriver_open(AUDIO_DRIVER_HANDLE* handle, uint32_t sampleRate, int format, int channels);
	static int					AudioDriver_close(AUDIO_DRIVER_HANDLE handle);
    static uint32_t				AudioDriver_latency(AUDIO_DRIVER_HANDLE handle);
    static int					AudioDriver_start(AUDIO_DRIVER_HANDLE handle);
	static int					AudioDriver_write(AUDIO_DRIVER_HANDLE handle, void *buffer, int buffer_size);    
    static int					AudioDriver_flush(AUDIO_DRIVER_HANDLE handle);
	static int					AudioDriver_stop(AUDIO_DRIVER_HANDLE handle);
    static int					AudioDriver_reload(AUDIO_DRIVER_HANDLE handle);
	
	static int					VideoDriver_open(VIDEO_DRIVER_HANDLE* handle);
	static int					VideoDriver_close(VIDEO_DRIVER_HANDLE handle);
	static int					VideoDriver_setSurface(VIDEO_DRIVER_HANDLE handle, void* env, void* surface);
    static int					VideoDriver_lockSurface(VIDEO_DRIVER_HANDLE handle, int* width, int* height, int* pixelformat, int* stride, void** pixels);
    static int					VideoDriver_unlockSurface(VIDEO_DRIVER_HANDLE handle);
};

#endif //FFMPEG_OUTPUT_H
