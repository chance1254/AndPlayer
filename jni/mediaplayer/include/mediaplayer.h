#ifndef FFMPEG_MEDIAPLAYER_H
#define FFMPEG_MEDIAPLAYER_H

#include <pthread.h>

#include <jni.h>
#include "errors.h"

#include "decoder_audio.h"
#include "decoder_video.h"

#define FFMPEG_PLAYER_MAX_QUEUE_SIZE 10

using namespace android;

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
	MEDIA_PLAYER_DECODED            = 1 << 4,
    MEDIA_PLAYER_STARTED            = 1 << 5,
    MEDIA_PLAYER_PAUSED             = 1 << 6,
    MEDIA_PLAYER_STOPPED            = 1 << 7,
    MEDIA_PLAYER_PLAYBACK_COMPLETE  = 1 << 8
};

class IMediaPlayerListener
{
public:
	IMediaPlayerListener();
	virtual ~IMediaPlayerListener();
	virtual void notify(int msg, int ext1, int ext2) = 0;
};

class MediaPlayer
{
public:
	MediaPlayer();
	~MediaPlayer();
	//Set media player data source
	status_t 		     setDataSource(const char* url);
	//set video surface
	status_t              setVideoSurface(JNIEnv* env, jobject jsurface);
	status_t              setListener(IMediaPlayerListener *listener);
	status_t              start();
	status_t              stop();
	status_t              pause();
	status_t              playAtSpeed(float* speed);
	bool                  isPlaying();
	status_t              getVideoWidth(int *w);
	status_t              getVideoHeight(int *h);
	status_t              seekTo(int msec);
	status_t              getCurrentPosition(int *msec);
	status_t              getDuration(int *msec);
	status_t              reset();
	status_t              setAudioStreamType(int type);
	status_t		      prepare();
	void                  notify(int msg, int ext1, int ext2);
	status_t 		      suspend();
	status_t        	  resume();
	status_t              setVideoFrameDrop(int level);
	inline int         	  videoFrameDrop() { return _videoFrameDrop; }
private:
	status_t 		      prepareAudio();
	status_t		      prepareVideo();
	int 				  _videoFrameDrop;
	bool			      shouldCancel(PacketQueue* queue);
	static void		      ffmpegNotify(void* ptr, int level, const char* fmt, va_list vl);
	static void*	      startPlayer(void* ptr);
	static void 	      decode(AVFrame* frame, double pts);
	static void 	      decode(int16_t* buffer, int buffer_size);
	void			      decodeMovie(void* ptr);
	double 				  time;
	pthread_mutex_t		  lock;
	pthread_t			  playerThread;
	PacketQueue*		  videoQueue;
	IMediaPlayerListener* mediaListener;
	AVFormatContext*      movieFile;
	int					  audioStreamIndex;
	int 				  videoStreamIndex;
	DecoderAudio* 		  audioDecoder;
	DecoderVideo*         videoDecoder;
	AVFrame*			  frame;
	struct SwsContext*	  convertContext;
	void*				  cookie;
	media_player_states   currentState;
	int					  duration;
	int 				  currentPosition;
	int					  seekPosition;
	bool				  prepareSync;
	status_t			  prepareStatus;
	int					  streamType;
	bool 				  isLoop;
	float				  leftVolume;
	float				  rightVolume;
	float				  playRate;
	int 				  videoWidth;
	int 				  videoHeight;
};
#endif //FFMPEG_MEDIAPLAYER_H
