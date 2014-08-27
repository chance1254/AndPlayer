#include "../include/audio_output.h"
#include <android/log.h>
#include <sys/time.h>
#include <time.h>
#include "../include/errors.h"
extern "C" {
#include "libavformat/avformat.h"
}
#define TAG "AudioOutput.cpp"
#define LOGI(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
using namespace android;
using namespace ffmpeg;
#include "../../ffmpeg/include/libavutil/time.h"

AVAudioConvert *av_audio_convert_alloc(enum AVSampleFormat out_fmt, int out_channels,
									   enum AVSampleFormat in_fmt, int in_channels,
									   const float *matrix, int flags)
{
	AVAudioConvert *convert;
	if(in_channels != out_channels)
	{
		return NULL;
	}

	convert = static_cast<AVAudioConvert*>(av_malloc(sizeof(AVAudioConvert)));
	if(!convert)
	{
		return NULL;
	}

	convert->in_channels = in_channels;
	convert->out_channels = out_channels;
	convert->fmt_pair = out_fmt + AV_SAMPLE_FMT_NB*in_fmt;

	return convert;
}

void av_audio_convert_free(AVAudioConvert* convert)
{
	av_free(convert);
}

int av_audio_convert(AVAudioConvert* convert,  void* const out[6], const int out_stride[6],
		const void* const in[6], const int  in_stride[6], int len)
{
	int channel;
	for(channel = 0; channel < convert->out_channels; ++channel)
	{
		const int is = in_stride[channel];
		const int os = out_stride[channel];
		const uint8_t *pi = static_cast<const uint8_t*>(in[channel]);
		uint8_t *po = static_cast<uint8_t *>(out[channel]);
		uint8_t *end = po + os*len;
		if(!out[channel])
		{
			continue;
		}
#define CONV(ofmt, otype, ifmt, expr)\
		if(convert->fmt_pair == ofmt + AV_SAMPLE_FMT_NB*ifmt){\
			do{\
				*(otype*)po = expr; pi += is; po += os;\
			}while(po < end);\
		}
		//FIXME put things below under ifdefs so we do not waste space for cases no codec will need
		//FIXME rounding ?
		 CONV(AV_SAMPLE_FMT_U8 , uint8_t, AV_SAMPLE_FMT_U8 ,  *(const uint8_t*)pi)
		 else CONV(AV_SAMPLE_FMT_S16, int16_t, AV_SAMPLE_FMT_U8 , (*(const uint8_t*)pi - 0x80)<<8)
		 else CONV(AV_SAMPLE_FMT_S32, int32_t, AV_SAMPLE_FMT_U8 , (*(const uint8_t*)pi - 0x80)<<24)
		 else CONV(AV_SAMPLE_FMT_FLT, float  , AV_SAMPLE_FMT_U8 , (*(const uint8_t*)pi - 0x80)*(1.0 / (1<<7)))
		 else CONV(AV_SAMPLE_FMT_DBL, double , AV_SAMPLE_FMT_U8 , (*(const uint8_t*)pi - 0x80)*(1.0 / (1<<7)))
		 else CONV(AV_SAMPLE_FMT_U8 , uint8_t, AV_SAMPLE_FMT_S16, (*(const int16_t*)pi>>8) + 0x80)
		 else CONV(AV_SAMPLE_FMT_S16, int16_t, AV_SAMPLE_FMT_S16,  *(const int16_t*)pi)
		 else CONV(AV_SAMPLE_FMT_S32, int32_t, AV_SAMPLE_FMT_S16,  *(const int16_t*)pi<<16)
		 else CONV(AV_SAMPLE_FMT_FLT, float  , AV_SAMPLE_FMT_S16,  *(const int16_t*)pi*(1.0 / (1<<15)))
		 else CONV(AV_SAMPLE_FMT_DBL, double , AV_SAMPLE_FMT_S16,  *(const int16_t*)pi*(1.0 / (1<<15)))
		 else CONV(AV_SAMPLE_FMT_U8 , uint8_t, AV_SAMPLE_FMT_S32, (*(const int32_t*)pi>>24) + 0x80)
		 else CONV(AV_SAMPLE_FMT_S16, int16_t, AV_SAMPLE_FMT_S32,  *(const int32_t*)pi>>16)
		 else CONV(AV_SAMPLE_FMT_S32, int32_t, AV_SAMPLE_FMT_S32,  *(const int32_t*)pi)
		 else CONV(AV_SAMPLE_FMT_FLT, float  , AV_SAMPLE_FMT_S32,  *(const int32_t*)pi*(1.0 / (1U<<31)))
		 else CONV(AV_SAMPLE_FMT_DBL, double , AV_SAMPLE_FMT_S32,  *(const int32_t*)pi*(1.0 / (1U<<31)))
		 else CONV(AV_SAMPLE_FMT_U8 , uint8_t, AV_SAMPLE_FMT_FLT, av_clip_uint8(  lrintf(*(const float*)pi * (1<<7)) + 0x80))
		 else CONV(AV_SAMPLE_FMT_S16, int16_t, AV_SAMPLE_FMT_FLT, av_clip_int16(  lrintf(*(const float*)pi * (1<<15))))
		 else CONV(AV_SAMPLE_FMT_S32, int32_t, AV_SAMPLE_FMT_FLT, av_clipl_int32(llrintf(*(const float*)pi * (1U<<31))))
		 else CONV(AV_SAMPLE_FMT_FLT, float  , AV_SAMPLE_FMT_FLT, *(const float*)pi)
		 else CONV(AV_SAMPLE_FMT_DBL, double , AV_SAMPLE_FMT_FLT, *(const float*)pi)
		 else CONV(AV_SAMPLE_FMT_U8 , uint8_t, AV_SAMPLE_FMT_DBL, av_clip_uint8(  lrint(*(const double*)pi * (1<<7)) + 0x80))
		 else CONV(AV_SAMPLE_FMT_S16, int16_t, AV_SAMPLE_FMT_DBL, av_clip_int16(  lrint(*(const double*)pi * (1<<15))))
		 else CONV(AV_SAMPLE_FMT_S32, int32_t, AV_SAMPLE_FMT_DBL, av_clipl_int32(llrint(*(const double*)pi * (1U<<31))))
		 else CONV(AV_SAMPLE_FMT_FLT, float  , AV_SAMPLE_FMT_DBL, *(const double*)pi)
		 else CONV(AV_SAMPLE_FMT_DBL, double , AV_SAMPLE_FMT_DBL, *(const double*)pi)
		 else return -1;
	}

	return 0;
}

namespace ffmpeg {
AudioOutput::AudioOutput(MediaPlayer* mediaPlayer)
{
	player = mediaPlayer;
	convert = 0;
	driverHandle = 0;
	mSampleRate = 0;
	mAudioFormat = 0;
	mAudioChannels = 0;
	mAudioLatency = 0;
	mAudioCurrentPTS = 0;
	mAudioCurrentPTSDrift = 0;
	mPausing = false;
}

AudioOutput::~AudioOutput(){}

int AudioOutput::open(uint32_t sampleRate, int format, int channels)
{
	int status;
	if(format != AV_SAMPLE_FMT_S16)
	{
		convert = av_audio_convert_alloc(AV_SAMPLE_FMT_S16, channels, (AVSampleFormat)format, channels, NULL, 0);
		if(!convert)
		{
			LOGE("Cannot convert %s sample format to %s sample format\n",
					av_get_sample_fmt_name((AVSampleFormat)format),
					av_get_sample_fmt_name(AV_SAMPLE_FMT_S16));
		}
	}

	// TODO: status = Output::AudioDriver_open(&mAudioDriverHandle, sampleRate, PCM_16_BIT, channels == 2 ? CHANNEL_OUT_STEREO : CHANNEL_OUT_MONO);
	mSampleRate = sampleRate;
	mAudioFormat = format;
	mAudioChannels = channels;
	//TODO: mAudioLatency = Output::AudioDriver_latency(mAudioDriverHandle) / 1000.0;
	LOGI("mAudioLatency : %f\n", mAudioLatency);
	//TODO: status |= Output::AudioDriver_start(mAudioDriverHandle);
	return status;
}

int AudioOutput::close()
{
	int status;
	//TODO: int status = Output::AudioDriver_stop(mAudioDriverHandle);
	//TODO: status |= Output::AudioDriver_close(mAudioDriverHandle);
	driverHandle = NULL;
	if (convert) {
		av_audio_convert_free(convert);
		convert = NULL;
	}
	return status;
}

int AudioOutput::queueAudioBuffer(uint8_t* buffer, int buffer_size, double pts)
{
#ifdef FPS_DEBUGGING
	{
		timeval ptime;
		static int frames;
		static double timeOne = -1;
		static double timeTwo = -1;
		gettimeofday(&ptime, NULL);

		timeTwo = ptime.tv_sec + (ptime.tv_usec / 1000000.0);
		if (timeOne == -1 || timeTwo > timeOne + 1) {
			LOGI("Audio fps:%i", frames);
			//sPlayer->notify(MEDIA_INFO_FRAMERATE_AUDIO, frames, -1);
			timeOne = timeTwo;
			frames = 0;
		}
		frames++;
	}
#endif
	mAudioCurrentPTS = pts;
	mAudioCurrentPTSDrift = mAudioCurrentPTS - av_gettime() / 1000000.0;
	if(convert)
	{
		const void *ibuf[6]= {buffer};
		void *obuf[6]= {buffer};
		int istride[6]= {av_get_bytes_per_sample((AVSampleFormat)mAudioFormat)};
		int ostride[6]= {2};
		int len = buffer_size / istride[0];
		if (av_audio_convert(convert, obuf, ostride, ibuf, istride, len)<0)
		{
			LOGE("av_audio_convert() failed\n");
			return ANDROID_AUDIOTRACK_RESULT_ERRNO;
		}
		buffer = sampleBuffer;
		buffer_size = len * 2;
	}
	do
	{
		int toWrite;
		//TODO: int toWrite = Output::AudioDriver_write(mAudioDriverHandle, buffer, buffer_size);
		if(toWrite <= 0) {
			LOGI("Couldn't write samples to audio track");
			break;
		}
		mAudioCurrentPTS += (double)toWrite / (mAudioChannels * mSampleRate * av_get_bytes_per_sample((AVSampleFormat)AV_SAMPLE_FMT_S16));
		double time = av_gettime() / 1000000.0;
		mAudioCurrentPTSDrift = mAudioCurrentPTS - time;
		buffer += toWrite;
		buffer_size -= toWrite;
	} while(buffer_size > 0);

	return NO_ERROR;
}

int AudioOutput::pause()
{
	mPausing = true;
	return NO_ERROR;
}

int AudioOutput::resume()
{
	mPausing = false;
	return NO_ERROR;
}

int AudioOutput::flush()
{
	return NO_ERROR;
}

double AudioOutput::getRealTime()
{
	double clock;
	if(mPausing)
	{
		clock = mAudioCurrentPTS - mAudioLatency;
	}

	else
	{
		clock = mAudioCurrentPTSDrift + av_gettime() / 1000000.0 - mAudioLatency;
	}

	return FFMAX(0, clock);
}
}
