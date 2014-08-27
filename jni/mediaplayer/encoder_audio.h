#ifndef FFMPEG_ENCODER_AUDIO_H
#define FFMPEG_ENCODER_AUDIO_H
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000

#include "thread.h"
#include "packetqueue.h"

extern "C" {
	//reverb api
	#include "../LibReverbEffect/api.h"
}

class MediaPlayer;

namespace ffmpeg {

class EncoderAudio : public Thread
{
public:
	EncoderAudio(MediaPlayer* mediaPlayer, char* output, bool bEnableReverb);
	virtual ~EncoderAudio();
	
    void					stop();

	void					enqueueAudioBuffer1(void* buffer, int size);
	void					enqueueAudioBuffer2(void* buffer, int size);
	void					setAudioRecordEffectType(int audioEffectType);

	inline int16_t clamp16(int32_t sample)
	{
		if ((sample>>15) ^ (sample>>31))
			sample = 0x7FFF ^ (sample>>31);
		return sample;
	}

protected:
	
	void					handleRun(void* ptr);
	virtual	void			doEncode() = 0;	
	
protected:
	char*					mOutPath;
	MediaPlayer*			mMediaPlayer;
	
	PacketQueue*			mAudioBuffer1;
	PacketQueue*			mAudioBuffer2;
	int16_t					mMixedSamples[AVCODEC_MAX_AUDIO_FRAME_SIZE];
	int16_t					mOutputBuffer[2 * AVCODEC_MAX_AUDIO_FRAME_SIZE];
	FILE* 					mOutFile;
	
	Reverb*					mReverb;
};

}

#endif //FFMPEG_ENCODER_AUDIO_H
