#ifndef FFMPEG_MEDIAPLAYER_H
#define FFMPEG_MEDIAPLAYER_H

#include <pthread.h>

#include <jni.h>
#include "Errors.h"

#include "decoder_audio.h"
#include "decoder_video.h"
#include "encoder_audio_aac.h"
#include "encoder_audio_mp3.h"
#include "video_output.h"
#include "audio_output.h"
#include "time_source.h"
#include "RefBase.h"


typedef IVideoDecoder* (*ff_create_omx_video_decoder)(MediaPlayer* mediaPlayer, AVStream* stream);
void ffmpeg_vd_register(ff_create_omx_video_decoder p);

#define FFMPEG_PLAYER_MAX_QUEUE_SIZE 16
#define VIDEO_FRAME_QUEUE_SIZE 4
/* no AV sync correction is done if below the AV sync threshold */
#define AV_SYNC_THRESHOLD 0.01
/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD 30.0
#define AV_VIDEO_CODEC_SKIP_LEVEL_FILTER 128
#define AV_VIDEO_FRAME_DROP_COUNT	5
enum {
	AV_DROP_LEVEL_NONE						= 0x00,
	AV_DROP_LEVEL_SKIP_DECODE 				= 0x01,
	AV_DROP_LEVEL_DROP_FRAME 				= 0x02,
};

#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#define LOG_ASSERT(condition, ...) if(condition)__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__);

using namespace android;
using namespace ffmpeg;

enum media_event_type {
    MEDIA_NOP               = 0, // interface test message
    MEDIA_PREPARED          = 1,
    MEDIA_PLAYBACK_COMPLETE = 2,
    MEDIA_BUFFERING_UPDATE  = 3,
    MEDIA_SEEK_COMPLETE     = 4,
    MEDIA_SET_VIDEO_SIZE    = 5,
    MEDIA_ERROR             = 100,
    MEDIA_INFO              = 200,
};

// Generic error codes for the media player framework.  Errors are fatal, the
// playback must abort.
//
// Errors are communicated back to the client using the
// MediaPlayerListener::notify method defined below.
// In this situation, 'notify' is invoked with the following:
//   'msg' is set to MEDIA_ERROR.
//   'ext1' should be a value from the enum media_error_type.
//   'ext2' contains an implementation dependant error code to provide
//          more details. Should default to 0 when not used.
//
// The codes are distributed as follow:
//   0xx: Reserved
//   1xx: Android Player errors. Something went wrong inside the MediaPlayer.
//   2xx: Media errors (e.g Codec not supported). There is a problem with the
//        media itself.
//   3xx: Runtime errors. Some extraordinary condition arose making the playback
//        impossible.
//
enum media_error_type {
    // 0xx
    MEDIA_ERROR_UNKNOWN = 1,
    // 1xx
    MEDIA_ERROR_SERVER_DIED = 100,
    // 2xx
    MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK = 200,
    // 3xx
};


// Info and warning codes for the media player framework.  These are non fatal,
// the playback is going on but there might be some user visible issues.
//
// Info and warning messages are communicated back to the client using the
// MediaPlayerListener::notify method defined below.  In this situation,
// 'notify' is invoked with the following:
//   'msg' is set to MEDIA_INFO.
//   'ext1' should be a value from the enum media_info_type.
//   'ext2' contains an implementation dependant info code to provide
//          more details. Should default to 0 when not used.
//
// The codes are distributed as follow:
//   0xx: Reserved
//   7xx: Android Player info/warning (e.g player lagging behind.)
//   8xx: Media info/warning (e.g media badly interleaved.)
//
enum media_info_type {
    // 0xx
    MEDIA_INFO_UNKNOWN = 1,
    // 7xx
    // The video is too complex for the decoder: it can't decode frames fast
    // enough. Possibly only the audio plays fine at this stage.
    MEDIA_INFO_VIDEO_TRACK_LAGGING = 700,
    // 8xx
    // Bad interleaving means that a media has been improperly interleaved or not
    // interleaved at all, e.g has all the video samples first then all the audio
    // ones. Video is playing but a lot of disk seek may be happening.
    MEDIA_INFO_BAD_INTERLEAVING = 800,
    // The media is not seekable (e.g live stream).
    MEDIA_INFO_NOT_SEEKABLE = 801,
    // New media metadata is available.
    MEDIA_INFO_METADATA_UPDATE = 802,

    MEDIA_INFO_FRAMERATE_VIDEO = 900,
    MEDIA_INFO_FRAMERATE_AUDIO,
};



enum media_player_states {
    MEDIA_PLAYER_STATE_ERROR        = 0,
    MEDIA_PLAYER_IDLE               = 1 << 0,
    MEDIA_PLAYER_INITIALIZED        = 1 << 1,
    MEDIA_PLAYER_PREPARING          = 1 << 2,
    MEDIA_PLAYER_PREPARED           = 1 << 3,
    MEDIA_PLAYER_STARTED            = 1 << 4,
    MEDIA_PLAYER_PAUSED             = 1 << 5,
    MEDIA_PLAYER_STOPPED            = 1 << 6,
    MEDIA_PLAYER_PLAYBACK_COMPLETE  = 1 << 7
};

enum {
    AV_SYNC_AUDIO_MASTER, /* default choice */
    AV_SYNC_VIDEO_MASTER,
    AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
};

// ----------------------------------------------------------------------------
// ref-counted object for callbacks
class MediaPlayerListener : public RefBase
{
public:
    virtual void notify(int msg, int ext1, int ext2) = 0;
};

class MediaPlayer
{
public:
    MediaPlayer();
    ~MediaPlayer();
	status_t        setDataSource(const char *url);
	status_t        setVideoSurface(JNIEnv* env, jobject jsurface);
	status_t        setListener(MediaPlayerListener *listener);
    status_t        selectChannel(bool left, bool right);
    status_t        selectAudioTrack(int num);
    status_t        getAudioTrackNum(int* num);
	status_t        start();
	status_t        stop();
	status_t        pause();
	bool            isPlaying();
	status_t        getVideoWidth(int *w);
	status_t        getVideoHeight(int *h);
	status_t        seekTo(int msec);
	status_t        getCurrentPosition(int *msec);
	status_t        getDuration(int *msec);
	status_t        reset();
	status_t        setAudioStreamType(int type);
	status_t		prepare();
    status_t		prepareAsync();	
	
	status_t        startAudioRecord(char* outfile, bool bMP3, bool bEnableReverb, int nChannelsNum, int nSampleRate);
	status_t		setAudioRecordEffectType(int audioEffectType);
	status_t		writeAudioRecordBuffer(void* buffer, int size);
	status_t		stopAudioRecord();
	
	status_t        suspend();
	status_t        resume();

    inline IVideoDecoder*       getDecoderVideo() { return mDecoderVideo; }
    inline DecoderAudio*       getDecoderAudio() { return mDecoderAudio; }
    inline int                 getAudioSampleRate() { return mSampleRate; }
	
	status_t		setVideoFrameDrop(int level);
	inline void		setVideoRotate(int rotate) { mVideoRotate = rotate; }
	inline int		getVideoRotate() { return mVideoRotate; }

	double              getVideoClock();
    double              getMasterClock();
    inline int         videoFrameDrop() { return mVideoFrameDrop; }
    inline int          getAVSyncType() { return mAVSyncType; }
	inline  double getFrameLastPTS() 
	{ 
		if (mVideoOutput) 
			return mVideoOutput->getFrameLastPTS();
		return 0;
	}
    double              computeTargetDelay(double delay);
    
	void 				flushVideoFrame();
	void                flushAudioFrame();
	void 				queueVideoFrame(AVFrame* frame, double pts, int64_t pos, double duration);
	void 				queueAudioBuffer(int16_t* buffer, int buffer_size, double pts);
	
private:
    void                unlockDelay(int msec);
    void                delay(int msec);
    status_t            prepare_i();
    status_t            reset_i();
    status_t            start_i();
    status_t            stop_i();
    status_t            play_i();
    status_t            pause_i();
    status_t            openAudioComponent_i();
    status_t            closeAudioComponent_i();
    status_t            switchAudioComponent_i();
    status_t            openVideoComponent_i();
    status_t            closeVideoComponent_i();
    
    //AV Sync Code
	double              getAudioClock();
//    double              getVideoClock();
    double              getExternalClock();
    int                 mAVSyncType;
    
    int                 mSampleRate;
	
	double              mExternalClockTimeBase;
	int                	mVideoFrameDrop;
	//AV Sync Code
		
private:
	status_t					prepareAudio();
	status_t					prepareVideo();
	void                        notify_i(int msg, int ext1, int ext2);
	int                         seekTo_i(int msec);
		
	bool						shouldCancel(PacketQueue* queue);
	static int					ffmpegDecodeInterruptCB(void* ctx);

    status_t                    startPlayerLoop();
    status_t                    stopPlayerLoop();
	static void*				startPlayer(void* ptr);
	void						playerLoop();

    // player pipeline
    char*                       mURI;
	pthread_t					mPlayerThread;
	bool                        mPlayerThreadStarted;
	bool                        mPlayerThreadStopped;

    AVFormatContext*			mMovieFile;

    int                         mAudioStreamIndexRequest;
    int 						mAudioStreamIndex;
    DecoderAudio*				mDecoderAudio;
    AudioOutput*                mAudioOutput;

    int 						mVideoStreamIndex;
	IVideoDecoder*             	mDecoderVideo;
	VideoOutput*				mVideoOutput;
	void*                       mVideoSurface;
	
	SystemTimeSource            mSystemTime;
	Mutex						mLock;	
	int		 					mAbortRequest;

	// channel toggle	
    bool                        mLeftChannelOn;
    bool                        mRightChannelOn;
    int16_t                     mSamples[AVCODEC_MAX_AUDIO_FRAME_SIZE];
    
	//Encoder
	EncoderAudio				*mEncoderAudio;
	Mutex						mEncoderAudioLock;

    sp<MediaPlayerListener>		mListener;
    void*                       mCookie;

    int                         mStreamType;
    
    media_player_states         mCurrentState;
    double                      mDuration;

    int		                    mSeekPosition;
    int                         mSeekingPosition;
    double                      mLastVideoPTS;
    double                      mLastAudioPTS;

    bool                        mLoop;
    float                       mLeftVolume;
    float                       mRightVolume;
    int                         mVideoWidth;
    int                         mVideoHeight;
	PixelFormat					mVideoFormat;
	int							mVideoRotate;

    bool                        mUseIOMX;
};

#endif // FFMPEG_MEDIAPLAYER_H
