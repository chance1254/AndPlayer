/*
 * mediaplayer.cpp
 */

//#define LOG_NDEBUG 0
#define TAG "FFMpegMediaPlayer"
#ifndef INT64_MIN
#define INT64_MIN       (-0x7fffffffffffffffLL - 1)
#endif
#ifndef INT64_MAX
#define INT64_MAX INT64_C(9223372036854775807)
#endif

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef ENABLE_LIBRARY_PROFILER
#include "../android-ndk-profiler-3.1/prof.h"
#endif

#define TRACE_LINE_PRINT() //LOGI("%s, %d\n", __FUNCTION__, __LINE__)

//#define TEST_ENCODER_AUDIO_AAC 1

//#define USE_INVALID_DATA_WRITE_CLOSED_AUDIO_CHANNEL_BUF

extern "C" {
	
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/avstring.h"
#include "libavutil/log.h"
#include "../yuv2rgb/include/yuv2rgb.h"
	
} // end of extern C

#include "mediaplayer.h"
#include "output.h"
#include "../jni/include/jniUtils.h"



#define LOGI(...) //__android_log_print(ANDROID_LOG_DEBUG, "mediaplayer", __VA_ARGS__)

static ff_create_omx_video_decoder p_ff_create_omx_video_decoder = NULL;
void ffmpeg_vd_register(ff_create_omx_video_decoder p)
{
    p_ff_create_omx_video_decoder = p;        
}

MediaPlayer::MediaPlayer()
{
    mVideoStreamIndex = mAudioStreamIndex = mAudioStreamIndexRequest = -1;
    mLastAudioPTS = mLastVideoPTS = 0;
    mDecoderVideo = NULL;
    mDecoderAudio = NULL;
    mVideoOutput = NULL;
    mAudioOutput = NULL;
    mVideoSurface = NULL;

	mAbortRequest = 0;
	
    mLeftChannelOn = mRightChannelOn = true;
    mListener = NULL;
    mCookie = NULL;
    mDuration = 0;
    mStreamType = MUSIC;
    mSeekingPosition = mSeekPosition = -1;
    mCurrentState = MEDIA_PLAYER_IDLE;
    mLoop = false;
    mLeftVolume = mRightVolume = 1.0;
    mVideoWidth = mVideoHeight = 0;
	mVideoRotate = 0;
    mUseIOMX = false;	
	
	mURI = NULL;
	mMovieFile = NULL;
	mAVSyncType = AV_SYNC_AUDIO_MASTER;
	mVideoFrameDrop = AV_DROP_LEVEL_SKIP_DECODE;
	mEncoderAudio = NULL;
	
#ifdef ENABLE_AUDIO_REVERB_PROC
    m_pReverb = NULL;
    m_bEnalbeAudioReverbEffect = true;
    m_nRecordAudioChannelsNum = DFT_RECORD_AUDIO_CHANNEL_NUM;
    m_nRecordAudioSampleRate = DFT_RECORD_AUDIO_SAMPLE_RATE;
#endif

}

MediaPlayer::~MediaPlayer()
{
	reset_i();
}
	
status_t MediaPlayer::prepareAudio()
{
	LOGI("prepareAudio");

    mAudioStreamIndex = -1;
	if (mAudioStreamIndexRequest > 0 
	    && mAudioStreamIndexRequest < mMovieFile->nb_streams
	    && mMovieFile->streams[mAudioStreamIndexRequest]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
	    mAudioStreamIndex = mAudioStreamIndexRequest;	    
	} else {
    	for (int i = 0; i < mMovieFile->nb_streams; i++) {
    		if (mMovieFile->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
    			mAudioStreamIndex = i;
    			break;
    		}
    	}
    }
	
	if (mAudioStreamIndex == -1) {
		return INVALID_OPERATION;
	}

	AVStream* stream = mMovieFile->streams[mAudioStreamIndex];
	// Get a pointer to the codec context for the video stream
	AVCodecContext* codec_ctx = stream->codec;
	AVCodec* codec = avcodec_find_decoder(codec_ctx->codec_id);
	if (codec == NULL) {
		return INVALID_OPERATION;
	}
	
	// Open codec
	if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
		return INVALID_OPERATION;
	}

	mSampleRate = stream->codec->sample_rate;

	return NO_ERROR;
}

status_t MediaPlayer::prepareVideo()
{
	LOGI("prepareVideo");
	// Find the first video stream
	mVideoStreamIndex = -1;
	for (int i = 0; i < mMovieFile->nb_streams; i++) {
	    LOGI("prepareVideo[1] -i:%d, mMovieFile->streams[i]->codec->codec_type:%d ",i,mMovieFile->streams[i]->codec->codec_type);
		if (mMovieFile->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			mVideoStreamIndex = i;
			break;
		}
	}
	
	LOGI("prepareVideo[2] -mVideoStreamIndex:%d ",mVideoStreamIndex);
	if (mVideoStreamIndex == -1) {
		return INVALID_OPERATION;
	}
	
	AVStream* stream = mMovieFile->streams[mVideoStreamIndex];
	// Get a pointer to the codec context for the video stream
	AVCodecContext* codec_ctx = stream->codec;
	AVCodec* codec = avcodec_find_decoder(codec_ctx->codec_id);
	LOGI("prepareVideo[3] -codec:%d ",codec);
	if (codec == NULL) {
		return INVALID_OPERATION;
	}
	
	LOGI("prepareVideo[4] -codec:%d ",codec);
	// Open codec
	if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
	    LOGI("prepareVideo[4-1] -codec:%d ",codec);
		return INVALID_OPERATION;
	}
	
	LOGI("prepareVideo[5]");
	
	AVDictionaryEntry * entry = av_dict_get(stream->metadata, "rotate", NULL, 0);
	int rotate;
	if (entry && entry->value)
		rotate = atoi(entry->value);
	else
		rotate = 0;
	mVideoFormat = codec_ctx->pix_fmt;	
	av_log(NULL, AV_LOG_INFO, "mVideoRotate: %d, rotate: %d, format: %d\n", mVideoRotate, rotate, mVideoFormat);
	if (mVideoFormat != PIX_FMT_YUV420P && mVideoRotate != 0) {
		av_log(NULL, AV_LOG_ERROR, "don't support rotate: %d on format %d\n", mVideoRotate, mVideoFormat);		
	}
	if (mVideoFormat == PIX_FMT_YUV420P && (mVideoRotate == 90 || mVideoRotate == 180 || mVideoRotate == 270)) {
		switch(mVideoRotate) {
		case 90:
		case 270:
			mVideoHeight = codec_ctx->width;
			mVideoWidth = codec_ctx->height;
			break;		
		case 180:
		default:
			mVideoWidth = codec_ctx->width;
			mVideoHeight = codec_ctx->height;
			break;
		}
	} else {
		mVideoWidth = codec_ctx->width;
		mVideoHeight = codec_ctx->height;
	}
	
	notify_i(MEDIA_SET_VIDEO_SIZE, mVideoWidth, mVideoHeight);	

	return NO_ERROR;
}

status_t MediaPlayer::prepare_i()
{	
	LOGI("[Native]--prepare_i()[1]\n");
    if ((mCurrentState & (MEDIA_PLAYER_INITIALIZED | MEDIA_PLAYER_STOPPED))) {
    	LOGI("[Native]--prepare_i()[2] -mURI:%s \n", mURI?mURI:"(NULL)");
        AVFormatContext *ic;
        int ret;        
        mCurrentState = MEDIA_PLAYER_PREPARING;
        // Open video file
        if (mURI == NULL) {
        	LOGI("[Native]--prepare_i()[2-1]\n");
            mCurrentState = MEDIA_PLAYER_STATE_ERROR;
            notify_i(MEDIA_ERROR, 0, 0);
		    return INVALID_OPERATION;
        }
        LOGI("[Native]--prepare_i()[3]\n");
		ic = avformat_alloc_context();
		ic->interrupt_callback.callback = ffmpegDecodeInterruptCB;
		ic->interrupt_callback.opaque = this;
	    if((ret = avformat_open_input(&ic, mURI, NULL, NULL)) != 0) {
	        mCurrentState = MEDIA_PLAYER_STATE_ERROR;
	        notify_i(MEDIA_ERROR, ret, ret);
	        LOGI("[Native]--prepare_i()[3-1]\n");
		    return INVALID_OPERATION;
	    }
	    LOGI("[Native]--prepare_i()[4]\n");
		if (   av_strstart(mURI, "http:", NULL) 
			|| av_strstart(mURI, "https:", NULL)
			|| av_strstart(mURI, "mms:", NULL)
			|| av_strstart(mURI, "rtsp:", NULL)
			)
		{
			LOGI("[Native]--prepare_i()[4-1]\n");
			ic->flags |= AVFMT_FLAG_IGNIDX;
		}

		LOGI("[Native]--prepare_i()[5]\n");
	    // Retrieve stream information
	    if((ret = av_find_stream_info(ic)) < 0) {
	    	LOGI("[Native]--prepare_i()[5-1]\n");
	        av_close_input_file(ic);
	        LOGI("[Native]--prepare_i()[5-2]\n");
	        mMovieFile = NULL;
	        mCurrentState = MEDIA_PLAYER_STATE_ERROR;
	        notify_i(MEDIA_ERROR, ret, ret);
	        LOGI("[Native]--prepare_i()[5-3]\n");
		    return INVALID_OPERATION;
	    }
		
		mMovieFile = ic;
	    av_dump_format(mMovieFile, 0, mURI, 0);

		LOGI("[Native]--prepare_i()[6]\n");
		mDuration =  (double)mMovieFile->duration;
		LOGI("[Native]--prepare_i()[7]-mDuration:%f, mMovieFile->duration%lld \n",mDuration,mMovieFile->duration);
	    prepareAudio();
	    LOGI("[Native]--prepare_i()[8]\n");
	    prepareVideo();
	    LOGI("[Native]--prepare_i()[9]\n");
	    mCurrentState = MEDIA_PLAYER_PREPARED;
	    notify_i(MEDIA_PREPARED, 0, 0);
	    LOGI("[Native]--prepare_i()[10]\n");
	    return OK;
    }
    LOGI("[Native]--prepare_i()[11] mCurrentState:%d \n",mCurrentState);
    LOGE("prepare_i called in state %d\n", mCurrentState);
    return INVALID_OPERATION;
}

status_t MediaPlayer::reset_i()
{
    mCurrentState = MEDIA_PLAYER_IDLE;
    
    {
        Mutex::Autounlock autoUnlock(mLock);
		mAbortRequest = 1;
        stopPlayerLoop();
        stopAudioRecord();

		mAbortRequest = 0;
    }
	closeVideoComponent_i();
    closeAudioComponent_i();
    if (mMovieFile) {
        av_close_input_file(mMovieFile);
        mMovieFile = NULL;
    }
    if (mURI) {
        free(mURI);
        mURI = NULL;
    }
	
    return NO_ERROR;
}

status_t MediaPlayer::start_i()
{
    if (mCurrentState != MEDIA_PLAYER_PREPARED
        && mCurrentState != MEDIA_PLAYER_STOPPED
        && mCurrentState != MEDIA_PLAYER_PLAYBACK_COMPLETE) {
        LOGE("start_i called at state %d\n", mCurrentState);
        return INVALID_OPERATION;
    }
    //closeVideoComponent_i();	
    //closeAudioComponent_i();
    if (mMovieFile) {
        //avformat_seek_file(mMovieFile, -1, 0, 0, 0, 0);
    }       
    startPlayerLoop();
    openAudioComponent_i();
    openVideoComponent_i();
    mCurrentState = MEDIA_PLAYER_STARTED;
    return NO_ERROR;
}

status_t MediaPlayer::stop_i()
{
    {
        Mutex::Autounlock autoUnlock(mLock);
        stopPlayerLoop();
        stopAudioRecord();
    }
    closeVideoComponent_i();
    closeAudioComponent_i();
    mCurrentState = MEDIA_PLAYER_STOPPED;
    if (mMovieFile) {
        //avformat_seek_file(mMovieFile, -1, 0, 0, 0, 0);
    }
    return NO_ERROR;
}

status_t MediaPlayer::play_i()
{
    if (mCurrentState != MEDIA_PLAYER_PAUSED) {
        LOGI("play_i called at state %d\n", mCurrentState);
        return INVALID_OPERATION;
    }
    mCurrentState = MEDIA_PLAYER_STARTED;
    if (mDecoderAudio) {
        mDecoderAudio->resume();
    }
    if (mDecoderVideo) {
        mDecoderVideo->resume();
    }
    if (mVideoOutput) {
        mVideoOutput->resume();
    }
    return NO_ERROR;       
}

status_t MediaPlayer::pause_i()
{
    if (mCurrentState != MEDIA_PLAYER_STARTED) {
        LOGI("pause_i called at state %d\n", mCurrentState);
        return INVALID_OPERATION;
    }
    mCurrentState = MEDIA_PLAYER_PAUSED;
    if (mDecoderAudio) {
        mDecoderAudio->pause();
    }
    if (mDecoderVideo) {
        mDecoderVideo->pause();
    }
    if (mVideoOutput) {
        mVideoOutput->pause();
    }
    return NO_ERROR;   
}
    
status_t MediaPlayer::prepare()
{
    Mutex::Autolock autoLock(mLock);
    LOGI("[Native] come here!!!\n");
    LOGI("prepare");
	return prepare_i();
}

status_t MediaPlayer::prepareAsync()
{    
    Mutex::Autolock autoLock(mLock);
    LOGI("prepareAsync");
    return NO_ERROR;    
}

status_t MediaPlayer::setListener(MediaPlayerListener* listener)
{
    Mutex::Autolock autoLock(mLock);

    LOGI("setListener");
    mListener = listener;
    return NO_ERROR;
}

status_t MediaPlayer::setDataSource(const char *url)
{
    Mutex::Autolock autoLock(mLock);
                
    LOGI("setDataSource(%s)", url);
    
    if (mCurrentState != MEDIA_PLAYER_STATE_ERROR &&
        mCurrentState != MEDIA_PLAYER_IDLE) {
        LOGI("setDataSource called in state %d\n", mCurrentState);
        return INVALID_OPERATION;
    }
    reset_i();
    mURI = strdup(url);        
	mCurrentState = MEDIA_PLAYER_INITIALIZED;

    return NO_ERROR;
}

status_t MediaPlayer::suspend()
{
    Mutex::Autolock autoLock(mLock);
	LOGI("suspend");
    
    return pause_i();
}

status_t MediaPlayer::resume()
{
	LOGI("resume");	
    return start();
}

status_t MediaPlayer::setVideoSurface(JNIEnv* env, jobject surf)
{
    Mutex::Autolock autoLock(mLock);

    if (surf == NULL) {
    	mVideoSurface = NULL;
    	LOGI("setSurface: mVideoSurface = %p\n", mVideoSurface);    	
    } else {
        mVideoSurface = surf;
    	LOGI("setSurface: mVideoSurface = %p\n", mVideoSurface);
    }
    if (mVideoOutput) {
        mVideoOutput->setSurface(env, mVideoSurface);
    }
    return NO_ERROR;
}

status_t MediaPlayer::selectChannel(bool left, bool right)
{
    Mutex::Autolock autoLock(mLock);
    mLeftChannelOn = left;
    mRightChannelOn = right;
    //LOGI("[Native]====MediaPlayer::selectChannel(%s,%s)  --mLeftChannelOn:%s ,mRightChannelOn:%s \n",left?"true":"false",right?"true":"false",mLeftChannelOn?"true":"false",mRightChannelOn?"true":"false");
    return NO_ERROR;
}

status_t MediaPlayer::selectAudioTrack(int num)
{
    Mutex::Autolock autoLock(mLock);

	if (mCurrentState < MEDIA_PLAYER_PREPARED) {
		return INVALID_OPERATION;
	}
	mAudioStreamIndexRequest = -1;
	for (int i = 0; i < mMovieFile->nb_streams; i++) {
		if (mMovieFile->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO
		    && (num == -1 || num-- == 0)) {
			mAudioStreamIndexRequest = i;
			break;
		}
	}
	LOGI("selectAudioTrack : %d-->%d\n", mAudioStreamIndex, mAudioStreamIndexRequest);
	return mAudioStreamIndexRequest == -1 ? INVALID_OPERATION : NO_ERROR;
}

status_t MediaPlayer::getAudioTrackNum(int* num)
{
    Mutex::Autolock autoLock(mLock);

	if (mCurrentState < MEDIA_PLAYER_PREPARED) {
		return INVALID_OPERATION;
	}
	int numAudioTracks = 0;
	for (int i = 0; i < mMovieFile->nb_streams; i++) {
		if (mMovieFile->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			numAudioTracks++;
		}
	}
	LOGI("getAudioTrackNum : %d\n", numAudioTracks);
	if (num) {
	    *num = numAudioTracks;
	}
	return NO_ERROR;
}

    
bool MediaPlayer::shouldCancel(PacketQueue* queue)
{
	return (mCurrentState == MEDIA_PLAYER_STATE_ERROR || mCurrentState == MEDIA_PLAYER_STOPPED ||
			 ((mCurrentState == MEDIA_PLAYER_STARTED) 
			  && queue->size() == 0));
}

double MediaPlayer::getAudioClock()
{
    if (mAudioOutput)
        return mAudioOutput->getRealTime();
    return 0;
}

/* get the current video clock value */
double MediaPlayer::getVideoClock()
{
    if (mVideoOutput)
        return mVideoOutput->getRealTime();
    return 0;
}

double MediaPlayer::getExternalClock()
{    
    return mSystemTime.getRealTime();
}

double MediaPlayer::getMasterClock()
{
    double clock;
    if (mAVSyncType == AV_SYNC_VIDEO_MASTER && mVideoStreamIndex != -1) {
        clock = getVideoClock();
    } else if (mAVSyncType == AV_SYNC_AUDIO_MASTER && mAudioStreamIndex != -1) {
        clock = getAudioClock();
    } else {
        clock = getExternalClock();
    }
    //LOGI("getMasterLock: %f\n", clock);
    return clock;
}

status_t MediaPlayer::setVideoFrameDrop(int level)
{
	Mutex::Autolock autoLock(mLock);
	
	mVideoFrameDrop = level;
	if (mDecoderVideo)
		mDecoderVideo->setVideoFrameDrop(level);
	if (mVideoOutput)
		mVideoOutput->setVideoFrameDrop(level);

	return NO_ERROR;
}

double MediaPlayer::computeTargetDelay(double delay)
{
    double sync_threshold, diff;

    /* update delay to follow master synchronisation source */
    if (((mAVSyncType == AV_SYNC_AUDIO_MASTER && mAudioStreamIndex != -1) ||
         mAVSyncType == AV_SYNC_EXTERNAL_CLOCK)) {
        /* if video is slave, we try to correct big delays by
           duplicating or deleting a frame */
        diff = getVideoClock() - getMasterClock();

        /* skip or repeat frame. We take into account the
           delay to compute the threshold. I still don't know
           if it is the best guess */
        sync_threshold = FFMAX(AV_SYNC_THRESHOLD, delay);
        if (fabs(diff) < AV_NOSYNC_THRESHOLD) {
            if (diff <= -sync_threshold)
                delay = 0;
            else if (diff >= sync_threshold)
                delay = 2 * delay;
        }
    }

    //av_dlog(NULL, "video: delay=%0.3f A-V=%f\n",
    //        delay, -diff);

    return delay;
}

void MediaPlayer::flushVideoFrame()
{
	if (mVideoOutput)
		mVideoOutput->flush();
	LOGI("Seeking to %f finished\n", ((double)mSeekingPosition) * 1e-3);
	if (mSeekingPosition == mSeekPosition)
	    mSeekPosition = -1;
    mSeekingPosition = -1;
}

void MediaPlayer::flushAudioFrame()
{
    if (mVideoStreamIndex == -1) {
		LOGI("Seeking to %f finished\n", ((double)mSeekingPosition) * 1e-3);
        if (mSeekingPosition == mSeekPosition)
            mSeekPosition = -1;
        mSeekingPosition = -1;
    }
}

void MediaPlayer::queueVideoFrame(AVFrame* frame, double pts, int64_t pos, double duration)
{
    mLastVideoPTS = pts;
	if (mVideoOutput) {
		mVideoOutput->queueVideoFrame(frame, pts, pos, duration);
	}
}

void MediaPlayer::queueAudioBuffer(int16_t* buffer, int buffer_size, double pts)
{
    int16_t* _buffer = buffer;    
    //LOGI("[Native]====MediaPlayer::queueAudioBuffer(%d,%d,%f)[1] --mLeftChannelOn:%s, mRightChannelOn:%s\n",buffer,buffer_size,pts,mLeftChannelOn?"true":"false",mRightChannelOn?"true":"false");
    if (mLeftChannelOn && mRightChannelOn) {
    	 //LOGI("[Native]====MediaPlayer::queueAudioBuffer()[1-1]\n");
        _buffer = buffer;
    } else if (mLeftChannelOn) {
    	 //LOGI("[Native]====MediaPlayer::queueAudioBuffer()[1-2]\n");
        int size = buffer_size / 2;
        _buffer = mSamples;
        for(int i = 0; i < size; i += 2) {
#ifdef USE_INVALID_DATA_WRITE_CLOSED_AUDIO_CHANNEL_BUF
						//LOGI("[Native] Only Left Channel On!!!\n");
            mSamples[i] = buffer[i]; //L
            memset(&mSamples[i+1], 0, sizeof(int16_t));//R
#else
            mSamples[i] = mSamples[i+1] = buffer[i];
#endif
        }
    } else if (mRightChannelOn) {
    	 //LOGI("[Native]====MediaPlayer::queueAudioBuffer()[1-3]\n");
        int size = buffer_size / 2;
        _buffer = mSamples;
        for(int i = 0; i < size; i += 2) {
#ifdef USE_INVALID_DATA_WRITE_CLOSED_AUDIO_CHANNEL_BUF
						//LOGI("[Native] Only Right Channel On!!!\n");
            memset(&mSamples[i], 0, sizeof(int16_t));//l
            mSamples[i+1] = buffer[i+1]; //R
#else
            mSamples[i] = mSamples[i+1] = buffer[i+1];
#endif
        }
    } else {
    	 //LOGI("[Native]====MediaPlayer::queueAudioBuffer()[1-4]\n");
        _buffer = mSamples;
        memset(mSamples, 0, buffer_size);   
    }
    //LOGI("[Native]====MediaPlayer::queueAudioBuffer()[2]\n");
#ifdef TEST_ENCODER_AUDIO_AAC
    writeAudioRecordBuffer(_buffer, buffer_size);
#endif
	if (mEncoderAudio != NULL) {
		Mutex::Autolock autoLock(mEncoderAudioLock);
		if (mEncoderAudio != NULL) {
			mEncoderAudio->enqueueAudioBuffer2(_buffer, buffer_size);
		}
	}
	mLastAudioPTS = pts;
	if (mAudioOutput) {
	    mAudioOutput->queueAudioBuffer((uint8_t*)_buffer, buffer_size, pts);
    }
}

status_t MediaPlayer::startPlayerLoop()
{
    if (mPlayerThreadStarted) {
        return INVALID_OPERATION;
    }
    mPlayerThreadStarted = true;
    mPlayerThreadStopped = false;
    pthread_create(&mPlayerThread, NULL, startPlayer, this);

    return NO_ERROR;
}

status_t MediaPlayer::stopPlayerLoop()
{
    if (!mPlayerThreadStarted) {
        return INVALID_OPERATION;
    }
    mPlayerThreadStarted = false;
    mPlayerThreadStopped = true;
    int ret = pthread_join(mPlayerThread, NULL);
    if (ret == -1) {
        LOGE("stopPlayerLoop: failed to join player Thread\n");
    }
    return ret;
}

void MediaPlayer::unlockDelay(int msec)
{
    Mutex::Autounlock autoUnlock(mLock);
    if (msec > 0) {
		usleep(msec * 1000);
	}
}

void MediaPlayer::delay(int msec)
{
    if (msec > 0) {
		usleep(msec * 1000);
	}
}

void MediaPlayer::playerLoop()
{
    int ret;
    AVPacket pPacket;

#ifdef TEST_ENCODER_AUDIO_AAC
	startAudioRecord("/sdcard/test.aac", false, 0, 0);
#endif
    
    while(!mPlayerThreadStopped) {        

        if (mMovieFile == NULL || !(mCurrentState & (MEDIA_PLAYER_PAUSED | MEDIA_PLAYER_STARTED))) {
            delay(10);
            continue;
        }
		if (mSeekPosition != -1 && mSeekingPosition == -1) {
		    int64_t seek_target = (int64_t)mSeekPosition * 1000;
            int64_t seek_min    = INT64_MIN;
            int64_t seek_max    = INT64_MAX;
            mSeekingPosition = mSeekPosition;
            LOGI("Seeking to %f\n", (double)seek_target * 1e-6);
            int ret = avformat_seek_file(mMovieFile, -1, seek_min, seek_target, seek_max, 0);
            if (ret < 0) {
                LOGI("error when seeking %s\n", mMovieFile->filename);
            } else {
                if (mDecoderVideo) {
                    mDecoderVideo->flush();
                }
                if (mDecoderAudio) {
                    mDecoderAudio->flush();
                }
                notify_i(MEDIA_SEEK_COMPLETE, 0, 0);
            }
            continue;
		}
		if (mAudioStreamIndexRequest != -1 
		    && mAudioStreamIndex != -1
		    && mAudioStreamIndexRequest != mAudioStreamIndex
		    ) {
		    Mutex::Autolock autoLock(mLock);
		    switchAudioComponent_i();
		    continue;
		}
		
		if ((mDecoderVideo == NULL || mDecoderVideo->packets() > FFMPEG_PLAYER_MAX_QUEUE_SIZE) &&
			(mDecoderAudio == NULL || mDecoderAudio->packets() > FFMPEG_PLAYER_MAX_QUEUE_SIZE)) {
			delay(10);
			continue;
		}
		ret = av_read_frame(mMovieFile, &pPacket);		
		if(ret < 0) {
		    if (ret == AVERROR_EOF || url_feof(mMovieFile->pb)) {
		        if (!mLoop) {
			        mCurrentState = MEDIA_PLAYER_PLAYBACK_COMPLETE;
			        notify_i(MEDIA_PLAYBACK_COMPLETE, 0, 0);
			    } else {
			        seekTo_i(0);
			    }
		    } else {
		        mCurrentState = MEDIA_PLAYER_STATE_ERROR;
		        notify_i(MEDIA_ERROR, ret, ret);
		    }
			continue;
		}

		// Is this a packet from the video stream?
		if (pPacket.stream_index == mVideoStreamIndex && mDecoderVideo) {
			mDecoderVideo->enqueue(&pPacket);
		} 
		else if (pPacket.stream_index == mAudioStreamIndex && mDecoderAudio) {
			mDecoderAudio->enqueue(&pPacket);
		}
		else {
			// Free the packet that was allocated by av_read_frame
			av_free_packet(&pPacket);
		}
    }

#ifdef TEST_ENCODER_AUDIO_AAC
	stopAudioRecord();
#endif

}

void* MediaPlayer::startPlayer(void* ptr)
{
    JavaVM *vm = getJavaVM();
    JNIEnv *env;
    vm->AttachCurrentThread(&env, NULL);
    //androidSetThreadPriority(0, ANDROID_PRIORITY_FOREGROUND);
    
#ifdef ENABLE_LIBRARY_PROFILER
    monstartup("libffmpeg_jni.so");
#endif
    
	MediaPlayer* thiz = (MediaPlayer*)ptr;
    LOGI("starting main player thread");
    thiz->playerLoop();
	LOGI("end of main player thread");	

#ifdef ENABLE_LIBRARY_PROFILER	
	moncleanup();
#endif
    vm->DetachCurrentThread();

    return NULL;
}

int MediaPlayer::openAudioComponent_i()
{
    LOGI("openAudioComponent_i");
    
    if (mMovieFile == NULL) {
        return INVALID_OPERATION;
    }
    
	mAudioStreamIndex = -1;
	if (mAudioStreamIndexRequest > 0 
	    && mAudioStreamIndexRequest < mMovieFile->nb_streams
	    && mMovieFile->streams[mAudioStreamIndexRequest]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
	    mAudioStreamIndex = mAudioStreamIndexRequest;	    
	} else {
    	for (int i = 0; i < mMovieFile->nb_streams; i++) {
    		if (mMovieFile->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
    			mAudioStreamIndex = i;
    			break;
    		}
    	}
    }
	if (mAudioStreamIndex == -1) {
		return INVALID_OPERATION;
	}

	AVStream* stream = mMovieFile->streams[mAudioStreamIndex];
	// Get a pointer to the codec context for the video stream
	AVCodecContext* codec_ctx = stream->codec;
	AVCodec* codec = avcodec_find_decoder(codec_ctx->codec_id);
	if (codec == NULL) {
		return INVALID_OPERATION;
	}
	
	// Open codec
	if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
		return INVALID_OPERATION;
	}
	mSampleRate = stream->codec->sample_rate;
	mAudioOutput = new AudioOutput(this);
	mAudioOutput->open(stream->codec->sample_rate,
					stream->codec->sample_fmt,
					stream->codec->channels);	
	mDecoderAudio = new DecoderAudio(this, stream);
	mDecoderAudio->startAsync();

	return NO_ERROR;
}
    
int MediaPlayer::closeAudioComponent_i()
{
    LOGI("closeAudioComponent_i");
    
    if (mDecoderAudio != NULL) {           
        mDecoderAudio->stop();
        mDecoderAudio->wait();
        delete mDecoderAudio;
        mDecoderAudio = NULL;
    }
    if (mAudioOutput != NULL) {
        mAudioOutput->close();
        delete mAudioOutput;
        mAudioOutput = NULL;
    }
    mAudioStreamIndex = -1;

	return NO_ERROR;
}

status_t  MediaPlayer::switchAudioComponent_i()
{
    LOGI("switchAudioComponent_i");

#if 1
    mAudioStreamIndex = -1;
	if (mAudioStreamIndexRequest > 0 
	    && mAudioStreamIndexRequest < mMovieFile->nb_streams
	    && mMovieFile->streams[mAudioStreamIndexRequest]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
	    mAudioStreamIndex = mAudioStreamIndexRequest;	    
	} else {
    	for (int i = 0; i < mMovieFile->nb_streams; i++) {
    		if (mMovieFile->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
    			mAudioStreamIndex = i;
    			break;
    		}
    	}
    }
	if (mAudioStreamIndex == -1) {
		return INVALID_OPERATION;
	}
	return NO_ERROR;
#else
    if (mAudioOutput != NULL) {
        mAudioOutput->pause();
    }
    if (mDecoderAudio != NULL) {           
        mDecoderAudio->stop();
        mDecoderAudio->wait();
        delete mDecoderAudio;
        mDecoderAudio = NULL;
    }
    
    
    if (mMovieFile == NULL) {
        return INVALID_OPERATION;
    }
    
	mAudioStreamIndex = -1;
	if (mAudioStreamIndexRequest > 0 
	    && mAudioStreamIndexRequest < mMovieFile->nb_streams
	    && mMovieFile->streams[mAudioStreamIndexRequest]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
	    mAudioStreamIndex = mAudioStreamIndexRequest;	    
	} else {
    	for (int i = 0; i < mMovieFile->nb_streams; i++) {
    		if (mMovieFile->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
    			mAudioStreamIndex = i;
    			break;
    		}
    	}
    }
	if (mAudioStreamIndex == -1) {
		return INVALID_OPERATION;
	}

	AVStream* stream = mMovieFile->streams[mAudioStreamIndex];
	// Get a pointer to the codec context for the video stream
	AVCodecContext* codec_ctx = stream->codec;
	AVCodec* codec = avcodec_find_decoder(codec_ctx->codec_id);
	if (codec == NULL) {
		return INVALID_OPERATION;
	}
	
	// Open codec
	if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
		return INVALID_OPERATION;
	}
	
	mSampleRate = stream->codec->sample_rate;
	if (mAudioOutput != NULL) {
		mAudioOutput->close();
		mAudioOutput->open(stream->codec->sample_rate,
				stream->codec->sample_fmt,
				stream->codec->channels);
	}
	mDecoderAudio = new DecoderAudio(this, stream);
	mDecoderAudio->startAsync();
    if (mAudioOutput != NULL) {
        mAudioOutput->resume();
    }

	return NO_ERROR;
#endif	
}
  
int MediaPlayer::openVideoComponent_i()
{
    LOGI("*** openVideoComponent_i[1] -mMovieFile:%d, mVideoStreamIndex:%d, mMovieFile->nb_streams:%d \n", mMovieFile, mVideoStreamIndex, mMovieFile?mMovieFile->nb_streams:0);
    AVCodecContext *avctx;
    AVCodec *codec;
    AVStream* stream;

    if (mMovieFile == NULL || mVideoStreamIndex < 0 || mVideoStreamIndex >= mMovieFile->nb_streams)
    {
        LOGI("*** openVideoComponent_i[1-1] -mMovieFile:%d, mVideoStreamIndex:%d, mMovieFile->nb_streams:%d \n", mMovieFile, mVideoStreamIndex, mMovieFile?mMovieFile->nb_streams:0);
        return INVALID_OPERATION;
    }

    LOGI("*** openVideoComponent_i[2]\n");
    stream = mMovieFile->streams[mVideoStreamIndex];
    avctx =  stream->codec;
	if (
        mUseIOMX
	    && p_ff_create_omx_video_decoder
	    && (mDecoderVideo = p_ff_create_omx_video_decoder(this, stream))) 
	{
        LOGI("*** openVideoComponent_i[2-1]\n");
		LOGI("!!!using ffmpeg_omxil video decoder!!!");
	} else {
        LOGI("*** openVideoComponent_i[2-2]\n");
		codec =  avcodec_find_decoder(avctx->codec_id);
		if (!codec)
            return INVALID_OPERATION;
		
	    // Open codec
	    if (avcodec_open2(avctx, codec, NULL) < 0) {
		    return INVALID_OPERATION;
	    }

        LOGI("*** openVideoComponent_i[2-3]\n");
	    mDecoderVideo = new DecoderVideo(this, stream);
	}
    
    LOGI("*** openVideoComponent_i[3]\n");
	mVideoOutput = new VideoOutput(this);
    LOGI("*** openVideoComponent_i[4]\n");
	mVideoOutput->setSurface(getJNIEnv(), mVideoSurface);
	if (mVideoFormat == PIX_FMT_YUV420P && (mVideoRotate == 90 || mVideoRotate == 180 || mVideoRotate == 270)) {
		mVideoOutput->open(mVideoFormat, mVideoWidth, mVideoHeight, mVideoRotate);
	} else {
		mVideoOutput->open(mVideoFormat, mVideoWidth, mVideoHeight, 0);
	}
	mDecoderVideo->startAsync();
    LOGI("*** openVideoComponent_i[5]\n");

    return OK;
}

status_t MediaPlayer::closeVideoComponent_i()
{
	if (mVideoOutput != NULL) {
        mVideoOutput->close();
	}
    if (mDecoderVideo != NULL) {
        mDecoderVideo->stop();
        mDecoderVideo->wait();
    }
    if (mVideoOutput != NULL) {
        delete mVideoOutput;
        mVideoOutput = NULL;
    }
    if (mDecoderVideo != NULL) {
        delete mDecoderVideo;
        mDecoderVideo = NULL;
    }
    return OK;
}

		   
status_t MediaPlayer::start()
{
    Mutex::Autolock autoLock(mLock);
    int ret = INVALID_OPERATION;

	if (mCurrentState == MEDIA_PLAYER_PREPARED || mCurrentState == MEDIA_PLAYER_STOPPED) {
        ret = start_i();                	    
	} else if (mCurrentState == MEDIA_PLAYER_PAUSED) {
	    ret = play_i();
	} else if (mCurrentState == MEDIA_PLAYER_PLAYBACK_COMPLETE) {
	    seekTo_i(0);
	    ret = play_i();
	}

	return ret;
}

status_t MediaPlayer::stop()
{
    Mutex::Autolock autoLock(mLock);
	
	return stop_i();
}

status_t MediaPlayer::pause()
{
    Mutex::Autolock autoLock(mLock);
	
	return pause_i();
}

bool MediaPlayer::isPlaying()
{
    Mutex::Autolock autoLock(mLock);
        
    return mCurrentState == MEDIA_PLAYER_STARTED;
}

status_t MediaPlayer::getVideoWidth(int *w)
{
    Mutex::Autolock autoLock(mLock);
        
	if (mCurrentState < MEDIA_PLAYER_PREPARED) {
		return INVALID_OPERATION;
	}
	*w = mVideoWidth;
    return NO_ERROR;
}

status_t MediaPlayer::getVideoHeight(int *h)
{
    Mutex::Autolock autoLock(mLock);

	if (mCurrentState < MEDIA_PLAYER_PREPARED) {
		return INVALID_OPERATION;
	}
	*h = mVideoHeight;
    return NO_ERROR;
}

status_t MediaPlayer::getCurrentPosition(int *msec)
{
    Mutex::Autolock autoLock(mLock);

	*msec = 0;
	if (mCurrentState < MEDIA_PLAYER_PREPARED) {
		return INVALID_OPERATION;
	}
	if (mCurrentState & (MEDIA_PLAYER_STARTED | MEDIA_PLAYER_PAUSED)) {
    	if (mSeekPosition != -1) {
    	    *msec = mSeekPosition;
		/*
		} else if (mAudioStreamIndex != -1 && mLastAudioPTS >= 0) {
    	    *msec = (int) (mLastAudioPTS * 1000);			
    	} else if (mVideoStreamIndex != -1 && mLastVideoPTS >= 0) {
    	    *msec = (int) (mLastVideoPTS * 1000);    	
		*/
    	} else {
    	    *msec = (int)(getMasterClock() * 1000);
    	}
	} else {
	    *msec = 0;
	}

	//LOGI("%d\n", *msec);
	return NO_ERROR;
}

status_t MediaPlayer::getDuration(int *msec)
{
    Mutex::Autolock autoLock(mLock);

	//LOGI(" **getDuration()[1]\n");
	if (mCurrentState < MEDIA_PLAYER_PREPARED) {
		LOGI(" **getDuration()[1-1] (mCurrentState:0x%x)\n",mCurrentState);
		return INVALID_OPERATION;
	}
	LOGI(" **getDuration()[2] (mCurrentState:0x%x , mDuration:%f)\n",mCurrentState,mDuration);
	*msec = mDuration / 1000;
    return NO_ERROR;
}

status_t MediaPlayer::seekTo_i(int msec)
{
    mSeekPosition = msec;    
}
    
status_t MediaPlayer::seekTo(int msec)
{
    Mutex::Autolock autoLock(mLock);

    return seekTo_i(msec);
}

status_t MediaPlayer::reset()
{
    Mutex::Autolock autoLock(mLock);

    return reset_i();
}

status_t MediaPlayer::setAudioStreamType(int type)
{
    Mutex::Autolock autoLock(mLock);
        
	return NO_ERROR;
}

int	MediaPlayer::ffmpegDecodeInterruptCB(void* ctx)
{
	MediaPlayer* player = (MediaPlayer*)ctx;
	return player->mAbortRequest;
}

void MediaPlayer::notify_i(int msg, int ext1, int ext2)
{
    LOGI("message received msg=%d, ext1=%d, ext2=%d", msg, ext1, ext2);
    if (mListener != 0) {
       mListener->notify(msg, ext1, ext2);
	}
}

status_t MediaPlayer::startAudioRecord(char* outfile, bool bMP3, bool bEnableReverb, int nChannelsNum, int nSampleRate)
{
	Mutex::Autolock autoLock(mEncoderAudioLock);
	
	if (mEncoderAudio != NULL)
	{
		return INVALID_OPERATION;
	}
	if (bMP3) {
		mEncoderAudio = new EncoderAudioMP3(this, outfile, bEnableReverb);
	} else {
		mEncoderAudio = new EncoderAudioAAC(this, outfile, bEnableReverb);
	}
	mEncoderAudio->startAsync();
	return NO_ERROR;
}

status_t MediaPlayer::setAudioRecordEffectType(int audioEffectType)
{
	Mutex::Autolock autoLock(mEncoderAudioLock);
	if (mEncoderAudio == NULL)
	{
		return INVALID_OPERATION;
	}
	mEncoderAudio->setAudioRecordEffectType(audioEffectType);
	return NO_ERROR;
}

status_t MediaPlayer::writeAudioRecordBuffer(void* buffer, int size)
{
	Mutex::Autolock autoLock(mEncoderAudioLock);
	if (mEncoderAudio == NULL)
	{
		return INVALID_OPERATION;
	}	
	mEncoderAudio->enqueueAudioBuffer1(buffer, size);
	return NO_ERROR;
}

status_t MediaPlayer::stopAudioRecord()
{
	Mutex::Autolock autoLock(mEncoderAudioLock);
	if (mEncoderAudio == NULL)
	{
		return INVALID_OPERATION;
	}
	mEncoderAudio->stop();
	delete mEncoderAudio;
	mEncoderAudio = NULL;	
	
	return NO_ERROR;
}
