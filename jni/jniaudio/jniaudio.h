/*
 * jniaudio.h
 *
 *  Created on: 28 рту. 2014 у.
 *      Author: 1
 */

#ifndef JNIAUDIO_H_
#define JNIAUDIO_H_

#include <dlfcn.h>
#include <assert.h>
#include <malloc.h>

#include <android/log.h>
#define TAG "ffmpeg_ao_audiotrack"
#define LOGI(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)

#include "../mediaplayer/audiotrack.h"

#define SIZE_OF_AUDIOTRACK 256

// _ZN7android11AudioSystem19getOutputFrameCountEPii
typedef int (*AudioSystem_getOutputFrameCount)(int *, int);
// _ZN7android11AudioSystem16getOutputLatencyEPji
typedef int (*AudioSystem_getOutputLatency)(unsigned int *, int);
// _ZN7android11AudioSystem21getOutputSamplingRateEPii
typedef int (*AudioSystem_getOutputSamplingRate)(int *, int);

// _ZN7android10AudioTrack16getMinFrameCountEPiij
typedef int (*AudioTrack_getMinFrameCount)(int *, int, unsigned int);

// _ZN7android10AudioTrackC1EijiiijPFviPvS1_ES1_ii
typedef void (*AudioTrack_ctor)(void *, int, unsigned int, int, int, int, unsigned int, void (*)(int, void *, void *), void *, int, int);
// _ZN7android10AudioTrackC1EijiiijPFviPvS1_ES1_i
typedef void (*AudioTrack_ctor_legacy)(void *, int, unsigned int, int, int, int, unsigned int, void (*)(int, void *, void *), void *, int);
// _ZN7android10AudioTrackD1Ev
typedef void (*AudioTrack_dtor)(void *);
// _ZNK7android10AudioTrack9initCheckEv
typedef int (*AudioTrack_initCheck)(void *);
typedef uint32_t (*AudioTrack_latency)(void *);
// _ZN7android10AudioTrack5startEv
typedef void (*AudioTrack_start)(void *);
// _ZN7android10AudioTrack4stopEv
typedef void (*AudioTrack_stop)(void *);
// _ZN7android10AudioTrack5writeEPKvj
typedef int (*AudioTrack_write)(void *, void  const*, unsigned int);
// _ZN7android10AudioTrack5flushEv
typedef void (*AudioTrack_flush)(void *);
static void *libmedia;
static AudioSystem_getOutputFrameCount as_getOutputFrameCount;
static AudioSystem_getOutputLatency as_getOutputLatency;
static AudioSystem_getOutputSamplingRate as_getOutputSamplingRate;

static AudioTrack_getMinFrameCount at_getMinFrameCount;
static AudioTrack_ctor at_ctor;
static AudioTrack_ctor_legacy at_ctor_legacy;
static AudioTrack_dtor at_dtor;
static AudioTrack_initCheck at_initCheck;
static AudioTrack_latency  at_latency;
static AudioTrack_start at_start;
static AudioTrack_stop at_stop;
static AudioTrack_write at_write;
static AudioTrack_flush at_flush;
static void* InitLibrary();
static int ffmpeg_ao_open(AUDIO_DRIVER_HANDLE* outHandle, uint32_t sampleRate, int format, int channels);
static int ffmpeg_ao_close(AUDIO_DRIVER_HANDLE handle);
static int ffmpeg_ao_set(AUDIO_DRIVER_HANDLE handle, int streamType, uint32_t sampleRate, int format, int channels);
static uint32_t ffmpeg_ao_latency(AUDIO_DRIVER_HANDLE handle);
static int ffmpeg_ao_start(AUDIO_DRIVER_HANDLE handle);
static int ffmpeg_ao_write(AUDIO_DRIVER_HANDLE handle, void *buffer, int buffer_size);
static int ffmpeg_ao_flush(AUDIO_DRIVER_HANDLE handle);
static int ffmpeg_ao_stop(AUDIO_DRIVER_HANDLE handle);
static int ffmpeg_ao_reload(AUDIO_DRIVER_HANDLE handle);
jint JNI_OnLoad(JavaVM* vm, void* reserved);

class AndroidAudioTrack
{
private:
        void* mAudioTrack;

public:
        AndroidAudioTrack();
        virtual ~AndroidAudioTrack();
        void close();
        int set(int streamType, uint32_t sampleRate, int format, int channels);
        uint32_t latency();
        int start();
        int write(void* buffer, int size);
        int flush();
        int stop();
        int reload();
};


#endif /* JNIAUDIO_H_ */
