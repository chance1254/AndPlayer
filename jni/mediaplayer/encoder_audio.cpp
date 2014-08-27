#include "encoder_audio.h"
#include "mediaplayer.h"

#define TAG "FFMpegEncoderAudio"

#define FFMPEG_ENCODER_MAX_QUEUE_SIZE (512*8) /*2M encoder buffer*/

namespace ffmpeg {

EncoderAudio::EncoderAudio(MediaPlayer* mediaPlayer, char* output, bool bEnableReverb)
	: mMediaPlayer(mediaPlayer), mOutPath(strdup(output)), mOutFile(NULL)	  
{
	mAudioBuffer1 = new PacketQueue();
	mAudioBuffer2 = new PacketQueue();

	if (bEnableReverb)
	{
		mReverb = createReverb();
		LOGI("mReverb %p", mReverb);
	}
	
	mOutFile = fopen(mOutPath, "wb");
	LOGI("writing to %s", mOutPath);
}

EncoderAudio::~EncoderAudio()
{
	if (mReverb)
	{
		LOGI("mReverb %p", mReverb);
		deleteReverb(mReverb);
		mReverb = NULL;
	}

	if (mOutFile)
	{
	  fclose(mOutFile);
	  mOutFile = NULL;
	}
	if (mOutPath)
	{
		free(mOutPath);
		mOutPath = NULL;
	}
	delete mAudioBuffer1;
	mAudioBuffer1 = NULL;
	delete mAudioBuffer2;
	mAudioBuffer2 = NULL;
}
	
void EncoderAudio::stop()
{
	Thread::stop();	
	mAudioBuffer1->abort();
	mAudioBuffer2->abort();
	
    LOGI("waiting on end of encoder thread");
    int ret = -1;
    if((ret = wait()) != 0) 
	{
        LOGI("Couldn't cancel EncoderAudio: %i", ret);
        return;
    }
}

void EncoderAudio::enqueueAudioBuffer1(void* buffer, int size)
{
    if (mAudioBuffer1->size() > FFMPEG_ENCODER_MAX_QUEUE_SIZE)
    {
        LOGI("encoder too slowly for buffer 1, %d\n", mAudioBuffer1->size());
        return;
    }
    
	AVPacket pkt;	
	av_new_packet(&pkt, size);
	memcpy(pkt.data, buffer, pkt.size);
	mAudioBuffer1->put(&pkt);
}

void EncoderAudio::enqueueAudioBuffer2(void* buffer, int size)
{
    if (mAudioBuffer2->size() > FFMPEG_ENCODER_MAX_QUEUE_SIZE)
    {
        LOGI("encoder too slowly for buffer 2, %d\n", mAudioBuffer2->size());
        return;
    }
        
	AVPacket pkt;
	av_new_packet(&pkt, size);
	memcpy(pkt.data, buffer, pkt.size);
	mAudioBuffer2->put(&pkt);
}

void EncoderAudio::setAudioRecordEffectType(int audioEffectType)
{
	LOGI("setAudioRecordEffectType: mReverb %p, effectType, %d", mReverb, audioEffectType);
	if (mReverb)
	{
		switch(audioEffectType)
		{
		case 0:
			//setReverbParem(mReverb, 0, 0, 0, 0, 0);
			setReverbParem(mReverb, 0, 0, 0, 0, 0);
			break;
		case 2:
			//setReverbParem(mReverb, 50, 30, 45, 30, 30);
			setReverbParem(mReverb, .50, .30, .45, .30, .30);
			break;
		case 3:
			//setReverbParem(mReverb, 50, 40, 45, 50, 50);
			setReverbParem(mReverb, .50, .40, .45, .50, .50);
			break;
		case 4:
			//setReverbParem(mReverb, 50, 40, 45, 70, 70);
			setReverbParem(mReverb, .50, .40, .45, .70, .70);
			break;
		}
	}
}

void EncoderAudio::handleRun(void* ptr)
{	
	LOGI("encoding audio start");
	doEncode();
    LOGI("encoding audio ended");
}

}