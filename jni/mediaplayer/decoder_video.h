#ifndef FFMPEG_DECODER_VIDEO_H
#define FFMPEG_DECODER_VIDEO_H

#include "decoder.h"

class DecoderVideo : public IVideoDecoder
{
public:
    DecoderVideo(MediaPlayer* mediaPlayer, AVStream* stream);
    ~DecoderVideo();

	void						stop();	
		
private:
	AVFrame*					mFrame;
	double						mVideoClock;
    int64_t                     mFaultyPTS;
    int64_t                     mLastPTSForFaultDetection;
    int64_t                     mFaultyDTS;
    int64_t                     mLastDTSForFaultDetection;
	
	int64_t			 mFrameDropCount;
	int64_t			 mFrameDropCountFilter;
	int				 mCodecSkipLevel;
	
	void			 setCodecSkipLevel();
    bool                        prepare();
    bool                        decode(void* ptr);
    bool                        process(AVPacket *pkt);    
};

#endif //FFMPEG_DECODER_AUDIO_H
