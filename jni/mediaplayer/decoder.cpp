#include <android/log.h>
#include "decoder.h"
#include "mediaplayer.h"

#define TAG "FFMpegIDecoder"

IDecoder::IDecoder(MediaPlayer* mediaPlayer, AVStream* stream)
{
	mQueue = new PacketQueue();
	mMediaPlayer = mediaPlayer;
	mStream = stream;
    //mStream->codec->debug_mv = 1;
    //mStream->codec->debug = 1;
    //mStream->codec->flags |= CODEC_FLAG_EMU_EDGE;
    //mStream->codec->idct_algo= FF_IDCT_AUTO;
	mStream->codec->workaround_bugs = 1;
	mStream->codec->flags2 |= (CODEC_FLAG2_FAST);
	//mStream->codec->skip_frame = AVDISCARD_BIDIR;
	//mStream->codec->skip_idct = AVDISCARD_BIDIR;
	//mStream->codec->skip_loop_filter = AVDISCARD_BIDIR;
	//mStream->codec->error_recognition = FF_ER_CAREFUL;
    //mStream->codec->error_concealment= FF_EC_GUESS_MVS;
    //avcodec_thread_init(mStream->codec, 2);
}

IDecoder::~IDecoder()
{
	if(mRunning)
    {
        stop();
    }
	delete mQueue;
	avcodec_close(mStream->codec);
}

void IDecoder::enqueue(AVPacket* packet)
{
	mQueue->put(packet);
}

int IDecoder::packets()
{
	return mQueue->size();
}

void IDecoder::flush()
{
	mQueue->putFlushPkt();
}

void IDecoder::stop()
{
    mQueue->abort();
	Thread::stop();
    LOGI("waiting on end of decoder thread");
    int ret = -1;
    if((ret = wait()) != 0) {
        LOGI("Couldn't cancel IDecoder: %i", ret);
        return;
    }
}

void IDecoder::handleRun(void* ptr)
{
	if(!prepare())
    {
		LOGI("Couldn't prepare decoder");
        return;
    }
	decode(ptr);
}

bool IDecoder::prepare()
{
    return false;
}

bool IDecoder::process(AVPacket *packet)
{
	return false;
}

bool IDecoder::decode(void* ptr)
{
    return false;
}

IVideoDecoder::IVideoDecoder(MediaPlayer* mediaPlayer, AVStream* stream)
    :IDecoder(mediaPlayer, stream), mFrameDrop(mediaPlayer->videoFrameDrop())        
{
}
        
IVideoDecoder::~IVideoDecoder()
{
}

    