/*
 * mediaplayer.cpp
 */

//#define LOG_NDEBUG 0
#define TAG "FFMpegMediaPlayer"

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

extern "C" {

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/log.h"

} // end of extern C

#include <android/log.h>

#include "../include/mediaplayer.h"
#include "../include/audiotrack.h"

#define FPS_DEBUGGING false

static MediaPlayer* sPlayer;

MediaPlayer::MediaPlayer()
{
    mediaListener = NULL;
    cookie = NULL;
    duration = -1;
    streamType = MUSIC;
    currentPosition = -1;
    seekPosition = -1;
    currentState = MEDIA_PLAYER_IDLE;
    prepareSync = false;
    prepareStatus = NO_ERROR;
    isLoop = false;
    pthread_mutex_init(&lock, NULL);
    leftVolume = rightVolume = 1.0;
    videoWidth = videoHeight = 0;
    sPlayer = this;
}

MediaPlayer::~MediaPlayer()
{
	if(mediaListener != NULL) {
		free(mediaListener);
	}
}

status_t MediaPlayer::prepareAudio()
{
	__android_log_print(ANDROID_LOG_INFO, TAG, "prepareAudio");
	audioStreamIndex = -1;
	for (int i = 0; i < movieFile->nb_streams; i++) {
		if (movieFile->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			audioStreamIndex = i;
			break;
		}
	}

	if (audioStreamIndex == -1) {
		return INVALID_OPERATION;
	}

	AVStream* stream = movieFile->streams[audioStreamIndex];
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

	// prepare os output
	//TODO prepare os output
	/*if (Output::AudioDriver_set(MUSIC,
								stream->codec->sample_rate,
								PCM_16_BIT,
								(stream->codec->channels == 2) ? CHANNEL_OUT_STEREO
										: CHANNEL_OUT_MONO) != ANDROID_AUDIOTRACK_RESULT_SUCCESS) {
		return INVALID_OPERATION;
	}

	if (Output::AudioDriver_start() != ANDROID_AUDIOTRACK_RESULT_SUCCESS) {
		return INVALID_OPERATION;
	}*/

	return NO_ERROR;
}

status_t MediaPlayer::prepareVideo()
{
	__android_log_print(ANDROID_LOG_INFO, TAG, "prepareVideo");
	// Find the first video stream
	videoStreamIndex = -1;
	for (int i = 0; i < movieFile->nb_streams; i++) {
		if (movieFile->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStreamIndex = i;
			break;
		}
	}

	if (videoStreamIndex == -1) {
		return INVALID_OPERATION;
	}

	AVStream* stream = movieFile->streams[videoStreamIndex];
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

	videoWidth = codec_ctx->width;
	videoHeight = codec_ctx->height;
	duration =  movieFile->duration;

	convertContext = sws_getContext(stream->codec->width,
								 stream->codec->height,
								 stream->codec->pix_fmt,
								 stream->codec->width,
								 stream->codec->height,
								 PIX_FMT_RGB565,
								 SWS_POINT,
								 NULL,
								 NULL,
								 NULL);

	if (convertContext == NULL) {
		return INVALID_OPERATION;
	}

	void*		pixels;
	/*if (Output::VideoDriver_getPixels(stream->codec->width,
									  stream->codec->height,
									  &pixels) != ANDROID_SURFACE_RESULT_SUCCESS) {
		return INVALID_OPERATION;
	}*/

	frame = avcodec_alloc_frame();
	if (frame == NULL) {
		return INVALID_OPERATION;
	}
	// Assign appropriate parts of buffer to image planes in pFrameRGB
	// Note that pFrameRGB is an AVFrame, but AVFrame is a superset
	// of AVPicture
	avpicture_fill((AVPicture *) frame,
				   (uint8_t *) pixels,
				   PIX_FMT_RGB565,
				   stream->codec->width,
				   stream->codec->height);

	return NO_ERROR;
}

status_t MediaPlayer::prepare()
{
	status_t ret;
	currentState = MEDIA_PLAYER_PREPARING;
	av_log_set_callback(ffmpegNotify);
	if ((ret = prepareVideo()) != NO_ERROR) {
		currentState = MEDIA_PLAYER_STATE_ERROR;
		return ret;
	}
	if ((ret = prepareAudio()) != NO_ERROR) {
		currentState = MEDIA_PLAYER_STATE_ERROR;
		return ret;
	}
	currentState = MEDIA_PLAYER_PREPARED;
	return NO_ERROR;
}

status_t MediaPlayer::setListener(IMediaPlayerListener* listener)
{
    __android_log_print(ANDROID_LOG_INFO, TAG, "setListener");
    mediaListener = listener;
    return NO_ERROR;
}

status_t MediaPlayer::setDataSource(const char *url)
{
    __android_log_print(ANDROID_LOG_INFO, TAG, "setDataSource(%s)", url);
    status_t err = BAD_VALUE;
	// Open video file
	if(avformat_open_input(&movieFile, url, NULL, NULL) != 0) {
		return INVALID_OPERATION;
	}
	// Retrieve stream information
	if(av_find_stream_info(movieFile) < 0) {
		return INVALID_OPERATION;
	}
	currentState = MEDIA_PLAYER_INITIALIZED;
    return NO_ERROR;
}

status_t MediaPlayer::suspend() {
	__android_log_print(ANDROID_LOG_INFO, TAG, "suspend");

	currentState = MEDIA_PLAYER_STOPPED;
	if(audioDecoder != NULL) {
		audioDecoder->stop();
	}
	if(videoDecoder != NULL) {
		videoDecoder->stop();
	}

	if(pthread_join(playerThread, NULL) != 0) {
		__android_log_print(ANDROID_LOG_ERROR, TAG, "Couldn't cancel player thread");
	}

	// Close the codec
	free(audioDecoder);
	free(videoDecoder);

	// Close the video file
	av_close_input_file(movieFile);

	//TODO close OS drivers
	/*Output::AudioDriver_unregister();
	Output::VideoDriver_unregister();
*/
	__android_log_print(ANDROID_LOG_ERROR, TAG, "suspended");

    return NO_ERROR;
}

status_t MediaPlayer::resume() {
	//pthread_mutex_lock(&mLock);
	currentState = MEDIA_PLAYER_STARTED;
	//pthread_mutex_unlock(&mLock);
    return NO_ERROR;
}

status_t MediaPlayer::setVideoSurface(JNIEnv* env, jobject jsurface)
{
	//TODO Set surface
	/*if(Output::VideoDriver_register(env, jsurface) != ANDROID_SURFACE_RESULT_SUCCESS) {
		return INVALID_OPERATION;
	}
	if(Output::AudioDriver_register() != ANDROID_AUDIOTRACK_RESULT_SUCCESS) {
		return INVALID_OPERATION;
	}*/
    return NO_ERROR;
}

bool MediaPlayer::shouldCancel(PacketQueue* queue)
{
	return (currentState == MEDIA_PLAYER_STATE_ERROR || currentState == MEDIA_PLAYER_STOPPED ||
			 ((currentState == MEDIA_PLAYER_DECODED || currentState == MEDIA_PLAYER_STARTED)
			  && queue->size() == 0));
}

void MediaPlayer::decode(AVFrame* frame, double pts)
{
	if(FPS_DEBUGGING) {
		timeval pTime;
		static int frames = 0;
		static double t1 = -1;
		static double t2 = -1;

		gettimeofday(&pTime, NULL);
		t2 = pTime.tv_sec + (pTime.tv_usec / 1000000.0);
		if (t1 == -1 || t2 > t1 + 1) {
			__android_log_print(ANDROID_LOG_INFO, TAG, "Video fps:%i", frames);
			//sPlayer->notify(MEDIA_INFO_FRAMERATE_VIDEO, frames, -1);
			t1 = t2;
			frames = 0;
		}
		frames++;
	}

	// Convert the image from its native format to RGB
	sws_scale(sPlayer->convertContext,
		      frame->data,
		      frame->linesize,
			  0,
			  sPlayer->videoHeight,
			  sPlayer->frame->data,
			  sPlayer->frame->linesize);

	//TODO: Update surface
	/*Output::VideoDriver_updateSurface();*/
}

void MediaPlayer::decode(int16_t* buffer, int buffer_size)
{
	if(FPS_DEBUGGING) {
		timeval pTime;
		static int frames = 0;
		static double t1 = -1;
		static double t2 = -1;

		gettimeofday(&pTime, NULL);
		t2 = pTime.tv_sec + (pTime.tv_usec / 1000000.0);
		if (t1 == -1 || t2 > t1 + 1) {
			__android_log_print(ANDROID_LOG_INFO, TAG, "Audio fps:%i", frames);
			//sPlayer->notify(MEDIA_INFO_FRAMERATE_AUDIO, frames, -1);
			t1 = t2;
			frames = 0;
		}
		frames++;
	}

	//TODO AUDIO DRIVER WRITE
	/*if(Output::AudioDriver_write(buffer, buffer_size) <= 0) {
		__android_log_print(ANDROID_LOG_ERROR, TAG, "Couldn't write samples to audio track");
	}*/
}

void MediaPlayer::decodeMovie(void* ptr)
{
	AVPacket pPacket;

	AVStream* stream_audio = movieFile->streams[audioStreamIndex];
	audioDecoder = new DecoderAudio(stream_audio);
	audioDecoder->onDecode = decode;
	audioDecoder->asyncStartThread();

	AVStream* stream_video = movieFile->streams[videoStreamIndex];
	videoDecoder = new DecoderVideo(stream_video);
	videoDecoder->onDecode = decode;
	videoDecoder->asyncStartThread();

	currentState = MEDIA_PLAYER_STARTED;
	__android_log_print(ANDROID_LOG_INFO, TAG, "playing %ix%i", videoWidth, videoHeight);
	while (currentState != MEDIA_PLAYER_DECODED && currentState != MEDIA_PLAYER_STOPPED &&
		   currentState != MEDIA_PLAYER_STATE_ERROR)
	{
		if (videoDecoder->packets() > FFMPEG_PLAYER_MAX_QUEUE_SIZE &&
				audioDecoder->packets() > FFMPEG_PLAYER_MAX_QUEUE_SIZE) {
			usleep(200);
			continue;
		}

		if(av_read_frame(movieFile, &pPacket) < 0) {
			currentState = MEDIA_PLAYER_DECODED;
			continue;
		}

		// Is this a packet from the video stream?
		if (pPacket.stream_index == videoStreamIndex) {
			videoDecoder->enqueue(&pPacket);
		}
		else if (pPacket.stream_index == audioStreamIndex) {
			audioDecoder->enqueue(&pPacket);
		}
		else {
			// Free the packet that was allocated by av_read_frame
			av_free_packet(&pPacket);
		}
	}

	//waits on end of video thread
	__android_log_print(ANDROID_LOG_ERROR, TAG, "waiting on video thread");
	int ret = -1;
	if((ret = videoDecoder->wait()) != 0) {
		__android_log_print(ANDROID_LOG_ERROR, TAG, "Couldn't cancel video thread: %i", ret);
	}

	__android_log_print(ANDROID_LOG_ERROR, TAG, "waiting on audio thread");
	if((ret = audioDecoder->wait()) != 0) {
		__android_log_print(ANDROID_LOG_ERROR, TAG, "Couldn't cancel audio thread: %i", ret);
	}

	if(currentState == MEDIA_PLAYER_STATE_ERROR) {
		__android_log_print(ANDROID_LOG_INFO, TAG, "playing err");
	}
	currentState = MEDIA_PLAYER_PLAYBACK_COMPLETE;
	__android_log_print(ANDROID_LOG_INFO, TAG, "end of playing");
}

void* MediaPlayer::startPlayer(void* ptr)
{
    __android_log_print(ANDROID_LOG_INFO, TAG, "starting main player thread");
    sPlayer->decodeMovie(ptr);
}

status_t MediaPlayer::start()
{
	if (currentState != MEDIA_PLAYER_PREPARED) {
		return INVALID_OPERATION;
	}
	pthread_create(&playerThread, NULL, startPlayer, NULL);
	return NO_ERROR;
}

status_t MediaPlayer::stop()
{
	//pthread_mutex_lock(&mLock);
	currentState = MEDIA_PLAYER_STOPPED;
	//pthread_mutex_unlock(&mLock);
    return NO_ERROR;
}

status_t MediaPlayer::pause()
{
	//pthread_mutex_lock(&mLock);
	currentState = MEDIA_PLAYER_PAUSED;
	//pthread_mutex_unlock(&mLock);
	return NO_ERROR;
}

bool MediaPlayer::isPlaying()
{
    return currentState == MEDIA_PLAYER_STARTED ||
		currentState == MEDIA_PLAYER_DECODED;
}

status_t MediaPlayer::getVideoWidth(int *w)
{
	if (currentState < MEDIA_PLAYER_PREPARED) {
		return INVALID_OPERATION;
	}
	*w = videoWidth;
    return NO_ERROR;
}

status_t MediaPlayer::getVideoHeight(int *h)
{
	if (currentState < MEDIA_PLAYER_PREPARED) {
		return INVALID_OPERATION;
	}
	*h = videoHeight;
    return NO_ERROR;
}

status_t MediaPlayer::getCurrentPosition(int *msec)
{
	if (currentState < MEDIA_PLAYER_PREPARED) {
		return INVALID_OPERATION;
	}
	*msec = 0/*av_gettime()*/;
	//__android_log_print(ANDROID_LOG_INFO, TAG, "position %i", *msec);
	return NO_ERROR;
}

status_t MediaPlayer::getDuration(int *msec)
{
	if (currentState < MEDIA_PLAYER_PREPARED) {
		return INVALID_OPERATION;
	}
	*msec = duration / 1000;
    return NO_ERROR;
}

status_t MediaPlayer::seekTo(int msec)
{
    return INVALID_OPERATION;
}

status_t MediaPlayer::reset()
{
    return INVALID_OPERATION;
}

status_t MediaPlayer::setAudioStreamType(int type)
{
	return NO_ERROR;
}

void MediaPlayer::ffmpegNotify(void* ptr, int level, const char* fmt, va_list vl) {

	switch(level) {
			/**
			 * Something went really wrong and we will crash now.
			 */
		case AV_LOG_PANIC:
			__android_log_print(ANDROID_LOG_ERROR, TAG, "AV_LOG_PANIC: %s", fmt);
			//sPlayer->currentState = MEDIA_PLAYER_STATE_ERROR;
			break;

			/**
			 * Something went wrong and recovery is not possible.
			 * For example, no header was found for a format which depends
			 * on headers or an illegal combination of parameters is used.
			 */
		case AV_LOG_FATAL:
			__android_log_print(ANDROID_LOG_ERROR, TAG, "AV_LOG_FATAL: %s", fmt);
			//sPlayer->currentState = MEDIA_PLAYER_STATE_ERROR;
			break;

			/**
			 * Something went wrong and cannot losslessly be recovered.
			 * However, not all future data is affected.
			 */
		case AV_LOG_ERROR:
			__android_log_print(ANDROID_LOG_ERROR, TAG, "AV_LOG_ERROR: %s", fmt);
			//sPlayer->currentState = MEDIA_PLAYER_STATE_ERROR;
			break;

			/**
			 * Something somehow does not look correct. This may or may not
			 * lead to problems. An example would be the use of '-vstrict -2'.
			 */
		case AV_LOG_WARNING:
			__android_log_print(ANDROID_LOG_ERROR, TAG, "AV_LOG_WARNING: %s", fmt);
			break;

		case AV_LOG_INFO:
			__android_log_print(ANDROID_LOG_INFO, TAG, "%s", fmt);
			break;

		case AV_LOG_DEBUG:
			__android_log_print(ANDROID_LOG_DEBUG, TAG, "%s", fmt);
			break;

	}
}

void MediaPlayer::notify(int msg, int ext1, int ext2)
{
    //__android_log_print(ANDROID_LOG_INFO, TAG, "message received msg=%d, ext1=%d, ext2=%d", msg, ext1, ext2);
    bool send = true;
    bool locked = false;

    if ((mediaListener != 0) && send) {
       //__android_log_print(ANDROID_LOG_INFO, TAG, "callback application");
       mediaListener->notify(msg, ext1, ext2);
       //__android_log_print(ANDROID_LOG_INFO, TAG, "back from callback");
	}
}
