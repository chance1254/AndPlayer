#ifndef AUDIO_OUTPUT_H_
#define AUDIO_OUTPUT_H_

extern "C" {

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

}

#include "output.h"
#include "time_source.h"
#include "audiotrack.h"
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 288000

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
	int             	open(uint32_t sampleRate, int format, int channels);
	int 				queueAudioBuffer(uint8_t* buffer, int buffer_size, double pts);
	int             	close();
	int             	pause();
	int             	resume();
	int             	flush();
	virtual double		getRealTime();
private:
	MediaPlayer* 		player;
	AVAudioConvert* 	convert;
	DECLARE_ALIGNED(16,uint8_t,sampleBuffer)[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];
	AUDIO_DRIVER_HANDLE driverHandle;
	int          		mSampleRate;
	int          		mAudioFormat;
	int          		mAudioChannels;
	double       		mAudioLatency;
	double       		mAudioCurrentPTS;
	double       		mAudioCurrentPTSDrift;
	bool         		mPausing;
};
}


#endif /* AUDIO_OUTPUT_H_ */
