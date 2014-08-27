#include <android/log.h>
#include "decoder_audio.h"
#include "mediaplayer.h"

#define TAG "FFMpegAudioDecoder"

DecoderAudio::DecoderAudio(MediaPlayer* mediaPlayer, AVStream* stream) : IDecoder(mediaPlayer, stream)
{
    mAudioClock = 0;
    mSamplesSize = AVCODEC_MAX_AUDIO_FRAME_SIZE;
    mSamples = (int16_t *) av_malloc(mSamplesSize);
}

DecoderAudio::~DecoderAudio()
{
    // Free audio samples buffer
	if (mSamples) {
		av_free(mSamples);
	}
}

bool DecoderAudio::prepare()
{
    //set_sched_policy(gettid(), SP_FOREGROUND);
    return true;
}

bool DecoderAudio::process(AVPacket *pkt)
{
	AVPacket tmp_pkt;

    if (pkt->pts != AV_NOPTS_VALUE) {
        mAudioClock = av_q2d(mStream->time_base) * pkt->pts;
    }
	tmp_pkt = *pkt;
	while(tmp_pkt.size > 0) {
		int size = mSamplesSize;
		int len = avcodec_decode_audio3(mStream->codec, mSamples, &size, &tmp_pkt);
		if (len < 0) {
			/* if error, skip frame */		
			break;
		}
		tmp_pkt.data += len;
		tmp_pkt.size -= len;
		if(size <= 0) {
			/* No data yet, get more frames */
			continue;
		}		
		//call handler for posting buffer to os audio driver
		mMediaPlayer->queueAudioBuffer(mSamples, size, mAudioClock);
        mAudioClock += (double)size / (mStream->codec->channels * mStream->codec->sample_rate * av_get_bytes_per_sample(mStream->codec->sample_fmt));
	}

    return true;
}

bool DecoderAudio::decode(void* ptr)
{
    AVPacket        pkt;

    __android_log_print(ANDROID_LOG_INFO, TAG, "decoding audio");

    while(mRunning) {
		if (mPausing) {
			delay(10);
			continue;
		}
        if(mQueue->get(&pkt, true) < 0) {
            break;
        }
		if (mQueue->isFlushPkt(&pkt)) {
		    avcodec_flush_buffers(mStream->codec);
		    mMediaPlayer->flushAudioFrame();
            continue;
        }
		process(&pkt);
        // Free the packet that was allocated by av_read_frame
        av_free_packet(&pkt);
    }

    __android_log_print(ANDROID_LOG_INFO, TAG, "decoding audio ended");

    return true;
}
