#ifndef _FFMPEG_AUDIO_OUTPUT_H_
#define _FFMPEG_AUDIO_OUTPUT_H_
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000

extern "C" {
	
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

}

#include "output.h"
#include "time_source.h"

class MediaPlayer;
typedef struct {
	 int in_channels;
	 int out_channels;
	 int fmt_pair;
} AVAudioConvert;

namespace ffmpeg {

class AudioOutput : public TimeSource
{
public:

	AudioOutput(MediaPlayer* mediaPlayer);
	virtual ~AudioOutput();
	
	int             open(uint32_t sampleRate, int format, int channels);	
	int 			queueAudioBuffer(uint8_t* buffer, int buffer_size, double pts);
	int             close();
	int             pause();
	int             resume();
	int             flush();
	
	virtual double          getRealTime();
	
private:
	MediaPlayer* mMediaPlayer;	
	AVAudioConvert *mConvert;
	DECLARE_ALIGNED(16,uint8_t,mSampleBuffer)[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];

	AUDIO_DRIVER_HANDLE        mAudioDriverHandle;
	int          mSampleRate;
	int          mAudioFormat;
	int          mAudioChannels;
	double       mAudioLatency;
	double       mAudioCurrentPTS;
	double       mAudioCurrentPTSDrift;
	bool         mPausing;
};

}

#endif //_FFMPEG_AUDIO_OUTPUT_H_
