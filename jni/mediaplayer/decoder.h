#ifndef FFMPEG_DECODER_H
#define FFMPEG_DECODER_H

extern "C" {
	
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

}

#include "thread.h"
#include "packetqueue.h"

class MediaPlayer;

class IDecoder : public Thread
{
public:
	IDecoder(MediaPlayer* mediaPlayer, AVStream* stream);
	virtual ~IDecoder();
	
    void						stop();
	void						enqueue(AVPacket* packet);
	int							packets();
	void                        flush();

protected:
    PacketQueue*                mQueue;
    AVStream*             		mStream;
	MediaPlayer*				mMediaPlayer;

    virtual bool                prepare();
    virtual bool                decode(void* ptr);
    virtual bool                process(AVPacket *packet);
	void						handleRun(void* ptr);
};

class IVideoDecoder : public IDecoder
{
public:
    IVideoDecoder(MediaPlayer* mediaPlayer, AVStream* stream);
    virtual ~IVideoDecoder();

    virtual void setVideoFrameDrop(int level) { mFrameDrop = level; }
    
protected:
    int		     	 mFrameDrop;    
};

#endif //FFMPEG_DECODER_H
