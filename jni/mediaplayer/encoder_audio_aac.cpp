#include "encoder_audio_aac.h"
#include "mediaplayer.h"

#define TAG "FFMpegEncoderAudioAAC"

namespace ffmpeg {

EncoderAudioAAC::EncoderAudioAAC(MediaPlayer* mediaPlayer, char* output, bool bEnableReverb)
	: EncoderAudio(mediaPlayer, output, bEnableReverb), mHandle(0)
{	
	memset(&mCodecAPI, 0, sizeof(VO_AUDIO_CODECAPI));
	memset(&mMemOperator, 0, sizeof(VO_MEM_OPERATOR));
	memset(&mUserData, 0, sizeof(VO_CODEC_INIT_USERDATA));
	memset(&mParams, 0, sizeof(AACENC_PARAM));
	
	voGetAACEncAPI(&mCodecAPI);

	mMemOperator.Alloc = cmnMemAlloc;
	mMemOperator.Copy = cmnMemCopy;
	mMemOperator.Free = cmnMemFree;
	mMemOperator.Set = cmnMemSet;
	mMemOperator.Check = cmnMemCheck;
	mUserData.memflag = VO_IMF_USERMEMOPERATOR;
	mUserData.memData = &mMemOperator;
	mCodecAPI.Init(&mHandle, VO_AUDIO_CodingAAC, &mUserData);

	mParams.sampleRate = mMediaPlayer->getAudioSampleRate();
	mParams.bitRate = 128000;
	mParams.nChannels = 2;
	mParams.adtsUsed = 1;

	if (mCodecAPI.SetParam(mHandle, VO_PID_AAC_ENCPARAM, &mParams) != VO_ERR_NONE) 
	{
        LOGI("Unable to set encoding parameters");
	}
	LOGI("initialized handle: %x", mHandle);
}

EncoderAudioAAC::~EncoderAudioAAC()
{
	if (mHandle)
	{
		mCodecAPI.Uninit(mHandle);
		mHandle = NULL;
	}
}

void EncoderAudioAAC::doEncode()
{	
	AVPacket tmp_pkt1;
	AVPacket tmp_pkt2;
	AVPacket pkt1;
	AVPacket pkt2;
	VO_CODECBUFFER input = { 0 }, output = { 0 };
	VO_AUDIO_OUTPUTINFO output_info = { 0 };
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
		
		unsigned char* inputBuffer = (unsigned char*)mMixedSamples;
		input.Buffer = inputBuffer;
		input.Length = inputSize;
		//LOGI("read input buffer %d", input.Length);
		mCodecAPI.SetInputData(mHandle, &input);
		
		while(inputSize > 0)	
		{
		    output.Buffer = (unsigned char*)mOutputBuffer;
		    output.Length = 2 * AVCODEC_MAX_AUDIO_FRAME_SIZE;

		    int status = mCodecAPI.GetOutputData(mHandle, &output, &output_info);
		    if (status == VO_ERR_NONE)
		    {
		        //LOGI("output_info %d", output_info.InputUsed);
		        //LOGI("write output buffer %d", output.Length);
		        inputSize -= output_info.InputUsed;
			    fwrite(mOutputBuffer, 1, output.Length, mOutFile);
		    } else if (status == VO_ERR_OUTPUT_BUFFER_SMALL)
		    {
			    LOGI("output buffer was too small, read %d", output_info.InputUsed);
			    break;
			} else if (status == VO_ERR_INPUT_BUFFER_SMALL)
			{
				//LOGI("input buffer was too small, read %d", output_info.InputUsed);
				break;
		    } else {
			    LOGI("Unable to encode frame: %x", status);
			    break;
		    }
	    }
    }
}

}