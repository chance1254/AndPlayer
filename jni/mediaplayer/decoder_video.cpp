#include <stdint.h>

#include <android/log.h>
#include "decoder_video.h"
#include "mediaplayer.h"

#define VIDEO_AV_FRAME_SIZE 10

#define TAG "FFMpegVideoDecoder"

DecoderVideo::DecoderVideo(MediaPlayer* mediaPlayer, AVStream* stream)
    : IVideoDecoder(mediaPlayer, stream)
	, mFrame(NULL), mVideoClock(0)
	, mFaultyPTS(0), mLastPTSForFaultDetection(0)
    , mFaultyDTS(0), mLastDTSForFaultDetection(0)
	, mFrameDropCount(0)
	, mFrameDropCountFilter(0), mCodecSkipLevel(0)
{
	setCodecSkipLevel();
	mFrame = avcodec_alloc_frame();
}

DecoderVideo::~DecoderVideo()
{
    if(mFrame)
		av_free(mFrame);
}

void DecoderVideo::setCodecSkipLevel()
{
	AVDiscard discard = AVDISCARD_DEFAULT;	    

	switch(mCodecSkipLevel)
	{
	case 0:
		discard = AVDISCARD_DEFAULT;
		break;
	case 1:
		discard = AVDISCARD_NONREF;
		break;
	case 2:
		discard = AVDISCARD_BIDIR;
		break;
	case 3:
	default:
		discard = AVDISCARD_NONKEY;
		break;	
	}
	LOGI("setCodecSkipLevel: (%d) --> (%d)\n", mStream->codec->skip_frame, discard);
	mStream->codec->skip_frame = discard;
	mStream->codec->skip_idct = discard;
	mStream->codec->skip_loop_filter = discard;
}

void DecoderVideo::stop()
{
	IDecoder::stop();
}

bool DecoderVideo::prepare()
{
	return true;
}

bool DecoderVideo::process(AVPacket *pkt)
{
    int	completed;
    int64_t pts_int;
    double pts;
    double frame_delay;

	if (!mFrame)
		return false;	

#if 0
    if (pkt->dts != AV_NOPTS_VALUE) {
        double time = mMediaPlayer->getMasterClock();
        double dts_time = pkt->dts * av_q2d(mStream->time_base);
        if (dts_time < time) {
            LOGI("%s: drop packet %f, clock %f\n", __FUNCTION__, dts_time, time);
            return false;
        }
    }
#endif    
    
    mStream->codec->reordered_opaque = pkt->pts;
	// Decode video frame
	avcodec_decode_video2(mStream->codec,
						 mFrame,
						 &completed,
						 pkt);
    
    if (completed) {
        if(pkt->dts != AV_NOPTS_VALUE){
            mFaultyDTS += pkt->dts <= mLastDTSForFaultDetection;
            mLastDTSForFaultDetection = pkt->dts;
        }
        if(mFrame->reordered_opaque != AV_NOPTS_VALUE){
            mFaultyPTS += mFrame->reordered_opaque <= mLastPTSForFaultDetection;
            mLastPTSForFaultDetection= mFrame->reordered_opaque;
        }
        if ((mFaultyPTS < mFaultyDTS || pkt->dts == AV_NOPTS_VALUE)
            && mFrame->reordered_opaque != AV_NOPTS_VALUE) {
            pts_int = mFrame->reordered_opaque;         
        } else if (pkt->dts != AV_NOPTS_VALUE) {
            pts_int = pkt->dts;
        } else {
            pts_int = 0;
        }
    
        pts = pts_int * av_q2d(mStream->time_base);
        if (pts != 0) {
            mVideoClock = pts;
        } else {
            pts = mVideoClock;
        }
        /* update video clock for next frame */
        frame_delay = av_q2d(mStream->codec->time_base);        
        /* for MPEG2, the frame can be repeated, so we update the clock accordingly */
        frame_delay += mFrame->repeat_pict * (frame_delay * 0.5);
        mVideoClock += frame_delay;
        
		if (mFrameDrop > AV_DROP_LEVEL_NONE && mMediaPlayer->getDecoderAudio()) {
			double frame_last_pts = mMediaPlayer->getFrameLastPTS();
			double video_clock = mMediaPlayer->getVideoClock();
			double master_clock = mMediaPlayer->getMasterClock();
            double clockdiff = video_clock - master_clock;
            double ptsdiff = pts - frame_last_pts;
			//LOGI("%s: clockdiff %f ptsdiff %f video_clock %f master_clock %f pts %f frame_last_pts %f (%lld)(%lld)(%d)\n", __FUNCTION__, clockdiff, ptsdiff, video_clock, master_clock, pts, frame_last_pts, mFrameDropCount, mFrameDropCountFilter, mCodecSkipLevel);
            if (fabs(clockdiff) < AV_NOSYNC_THRESHOLD 
				&& ptsdiff > 0 && ptsdiff < AV_NOSYNC_THRESHOLD
				&& clockdiff + ptsdiff < 0) {
				static int dropCount = 0;
                //is->frame_last_dropped_pos = pkt->pos;
                //is->frame_last_dropped_pts = dpts;
                //is->frame_drops_early++;
				if (mFrameDrop >= AV_DROP_LEVEL_DROP_FRAME) {
					dropCount++;
					if ((dropCount % AV_VIDEO_FRAME_DROP_COUNT) == 0) {
						mMediaPlayer->queueVideoFrame(mFrame, pts, pkt->pos, frame_delay);
					} else {
						mFrameDropCount++;					
						//LOGI("%s: clockdiff %f ptsdiff %f video_clock %f master_clock %f pts %f frame_last_pts %f (%lld)(%lld)(%d)\n", __FUNCTION__, clockdiff, ptsdiff, video_clock, master_clock, pts, frame_last_pts, mFrameDropCount, mFrameDropCountFilter, mCodecSkipLevel);
					}
				} else {
					mMediaPlayer->queueVideoFrame(mFrame, pts, pkt->pos, frame_delay);
				}
				if (mCodecSkipLevel < 3) {
					mFrameDropCountFilter++;
					if (mFrameDropCountFilter > AV_VIDEO_CODEC_SKIP_LEVEL_FILTER) {
						mFrameDropCountFilter = 0;
						mCodecSkipLevel++;
						setCodecSkipLevel();
					}
				}
            } else {				
				if (mCodecSkipLevel > 0) {
					mFrameDropCountFilter--;
					if (mFrameDropCountFilter < -AV_VIDEO_CODEC_SKIP_LEVEL_FILTER) {
						mFrameDropCountFilter = 0;
						mCodecSkipLevel--;
						setCodecSkipLevel();
					}
				}
				mMediaPlayer->queueVideoFrame(mFrame, pts, pkt->pos, frame_delay);
			}
        } else {
			mMediaPlayer->queueVideoFrame(mFrame, pts, pkt->pos, frame_delay);
		}
			
		return true;
    }	

	return false;
}

bool DecoderVideo::decode(void* ptr)
{
	AVPacket        pkt;
	
	LOGI("decoding video");
	
    while(mRunning) {        
		if (mPausing) {
			delay(10);
			continue;
		}
		
        if(mQueue->get(&pkt, true) < 0) {
            mRunning = false;
            break;
        }
		if (mQueue->isFlushPkt(&pkt)) {
		    avcodec_flush_buffers(mStream->codec);
			mMediaPlayer->flushVideoFrame();
            continue;
        }
		process(&pkt);		
        // Free the packet that was allocated by av_read_frame
        av_free_packet(&pkt);
    }
	
    LOGI("decoding video ended");    

    return true;
}
