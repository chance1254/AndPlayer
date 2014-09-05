#include <android/audiotrack.h>

int AndroidAudioTrack_register(){}

int AndroidAudioTrack_set(int streamType,
						  uint32_t sampleRate,
						  int format,
						  int channels){}


int AndroidAudioTrack_start()
{
}

int AndroidAudioTrack_flush(){}

int AndroidAudioTrack_stop(){}

int AndroidAudioTrack_reload(){}

int AndroidAudioTrack_unregister(){}

int AndroidAudioTrack_write(void *buffer, int buffer_size){}