#include "encoder_audio_mp3.h"
#include "mediaplayer.h"

#define TAG "FFMpegEncoderAudioMP3"

namespace ffmpeg {

EncoderAudioMP3::EncoderAudioMP3(MediaPlayer* mediaPlayer, char* output, bool bEnableReverb)
	: EncoderAudio(mediaPlayer, output, bEnableReverb), mHandle(0)
{
	mHandle = lame_init();

	LOGI("Init parameters:");
	lame_set_num_channels(mHandle, 2);
	lame_set_in_samplerate(mHandle, mMediaPlayer->getAudioSampleRate());
	lame_set_brate(mHandle, 128000);
	lame_set_mode(mHandle, JOINT_STEREO);
	lame_set_quality(mHandle, 5);

	int res = lame_init_params(mHandle);
	LOGI("Init returned: %d", res);	
	LOGI("initialized handle: %x", mHandle);
}

EncoderAudioMP3::~EncoderAudioMP3()
{
	if (mHandle)
	{
		lame_close(mHandle);
		mHandle = NULL;
	}
}

void EncoderAudioMP3::doEncode()
{	
	AVPacket tmp_pkt1;
	AVPacket tmp_pkt2;
	AVPacket pkt1;
	AVPacket pkt2;
	int sampleRate = mMediaPlayer->getAudioSampleRate();	
	
	tmp_pkt1.size = tmp_pkt2.size = 0;
	tmp_pkt1.data = tmp_pkt2.data = 0;
	
    while(mRunning) 
	{
		if(mPausing)
		{
			delay(10);
			continue;
		}
		if (tmp_pkt1.size <= 0)
		{
			if (mAudioBuffer1->get(&pkt1, true) < 0)
			{
			    LOGI("failed to get buffer1");
				break;
			}
			tmp_pkt1 = pkt1;
		}
		if (tmp_pkt2.size <= 0)
		{
			if (mAudioBuffer2->get(&pkt2, true) < 0)
			{
			    LOGI("failed to get buffer2");
				break;
			}
			tmp_pkt2 = pkt2;
		}
		int inputSize = FFMIN(tmp_pkt1.size, tmp_pkt2.size);
		int16_t* buffer1 = (int16_t*)tmp_pkt1.data;
		int16_t* buffer2 = (int16_t*)tmp_pkt2.data;
		for(int i = 0; i < inputSize / 2; i+=2)
		{
			int32_t l = buffer1[i] + buffer2[i];
			int32_t r = buffer1[i + 1] + buffer2[i+1];
			mMixedSamples[i] = clamp16(l);
			mMixedSamples[i+1] = clamp16(r);
			//mMixedSamples[i] = buffer1[i];
			//mMixedSamples[i+1] = buffer2[i+1];
		}
		if(mReverb)
		{    		
            //Do audio data reverb processing
            //OSStatus iRetVal = simpleDelay(mReverb, NULL, /*No use*/ inputSize / 2, mMixedSamples, sampleRate, 2);
			OSStatus iRetVal = simpleDelay1(mReverb, NULL, /*No use*/ inputSize / 2, mMixedSamples, sampleRate, 2);
			if (iRetVal != noErr)
			{
				LOGI("Unable to reverb audio: %x", noErr);
			}
		}
		
		tmp_pkt1.size -= inputSize;
		tmp_pkt1.data += inputSize;
		tmp_pkt2.size -= inputSize;
		tmp_pkt2.data += inputSize;
		if (tmp_pkt1.size <= 0)
		{
			av_free_packet(&pkt1);
		}
		if (tmp_pkt2.size <= 0)
		{
			av_free_packet(&pkt2);
		}

		int status = lame_encode_buffer_interleaved(mHandle, mMixedSamples,
                                              inputSize / 4,
                                              (unsigned char*)mOutputBuffer,
                                              AVCODEC_MAX_AUDIO_FRAME_SIZE * 2);
		if (status < 0) {
			if (status == -1) {
				LOGE("lame: output buffer too small");
			}
			LOGI("Unable to encode frame: %x", status);
			break;
		} else if (status > 0) {
			fwrite(mOutputBuffer, 1, status, mOutFile);	
		}
    }
	int status = lame_encode_flush(mHandle, (unsigned char*)mOutputBuffer, AVCODEC_MAX_AUDIO_FRAME_SIZE * 2);
	if (status > 0) {
		LOGI("Flushed %d bytes", status);
		fwrite(mOutputBuffer, 1, status, mOutFile);		
	}
}

}