#include "com_v2soft_spoiq_ffmpeg_core_FFMPEGPlayer.h"

#include <android/log.h>
#include "jniUtils.h"
#include "methods.h"
#define TAG "FFMPEGPlayer"

#include <mediaplayer.h>

struct fields_t {
    jfieldID    context;
    jmethodID   post_event;
};
static fields_t fields;

static const char* const kClassPathName = "com/v2soft/spoiq/ffmpeg/core/FFMPEGPlayer";

class JNIFFmpegMediaPlayerListener: public MediaPlayerListener
{
public:
    JNIFFmpegMediaPlayerListener(JNIEnv* env, jobject thiz, jobject weak_thiz);
    ~JNIFFmpegMediaPlayerListener();
    void notify(int msg, int ext1, int ext2);
private:
    JNIFFmpegMediaPlayerListener();
    jclass      mClass;     // Reference to MediaPlayer class
    jobject     mObject;    // Weak ref to MediaPlayer Java object to call on
};

JNIFFmpegMediaPlayerListener::JNIFFmpegMediaPlayerListener(JNIEnv* env, jobject thiz, jobject weak_thiz)
{
    // Hold onto the MediaPlayer class for use in calling the static method
    // that posts events to the application thread.
    jclass clazz = env->GetObjectClass(thiz);
    if (clazz == NULL) {
        jniThrowException(env, "java/lang/Exception", kClassPathName);
        return;
    }
    mClass = (jclass)env->NewGlobalRef(clazz);

    // We use a weak reference so the MediaPlayer object can be garbage collected.
    // The reference is only used as a proxy for callbacks.
    mObject  = env->NewGlobalRef(weak_thiz);
}

JNIFFmpegMediaPlayerListener::~JNIFFmpegMediaPlayerListener()
{
    // remove global references
    JNIEnv *env = getJNIEnv();
    env->DeleteGlobalRef(mObject);
    env->DeleteGlobalRef(mClass);
}

void JNIFFmpegMediaPlayerListener::notify(int msg, int ext1, int ext2)
{
    JNIEnv *env = getJNIEnv();
    env->CallStaticVoidMethod(mClass, fields.post_event, mObject, msg, ext1, ext2, 0);
}

static MediaPlayer* getMediaPlayer(JNIEnv* env, jobject thiz)
{
    return (MediaPlayer*)env->GetIntField(thiz, fields.context);
}

static MediaPlayer* setMediaPlayer(JNIEnv* env, jobject thiz, MediaPlayer* player)
{
    MediaPlayer* old = (MediaPlayer*)env->GetIntField(thiz, fields.context);
    if (old != NULL) {
		__android_log_print(ANDROID_LOG_INFO, TAG, "freeing old mediaplayer object");
		free(old);
	}
    env->SetIntField(thiz, fields.context, (int)player);
    return old;
}

static void process_media_player_call(JNIEnv *env, jobject thiz, status_t opStatus, const char* exception, const char *message)
{
    if (exception == NULL) {  // Don't throw exception. Instead, send an event.
		/*
        if (opStatus != (status_t) OK) {
            sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
            if (mp != 0) mp->notify(MEDIA_ERROR, opStatus, 0);
        }
		*/
    } else {  // Throw exception!
        if ( opStatus == (status_t) INVALID_OPERATION ) {
            jniThrowException(env, "java/lang/IllegalStateException", NULL);
        } else if ( opStatus != (status_t) OK ) {
            if (strlen(message) > 230) {
               // if the message is too long, don't bother displaying the status code
               jniThrowException( env, exception, message);
            } else {
               char msg[256];
                // append the status code to the message
               sprintf(msg, "%s: status=0x%X", message, opStatus);
               jniThrowException( env, exception, msg);
            }
        }
    }
}

static void setDataSourceAndHeaders(JNIEnv *env, jobject thiz, jstring path, jobject headers)
{
    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }

    if (path == NULL) {
        jniThrowException(env, "java/lang/IllegalArgumentException", NULL);
        return;
    }

    const char *pathStr = env->GetStringUTFChars(path, NULL);
    if (pathStr == NULL) {  // Out of memory
        jniThrowException(env, "java/lang/RuntimeException", "Out of memory");
        return;
    }

    __android_log_print(ANDROID_LOG_INFO, TAG, "setDataSource: path %s", pathStr);
    status_t opStatus = mp->setDataSource(pathStr);

    // Make sure that local ref is released before a potential exception
    env->ReleaseStringUTFChars(path, pathStr);

    process_media_player_call(
            env, thiz, opStatus, "java/io/IOException",
            "setDataSource failed." );
}

JNIEXPORT void JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEGPlayer_setDataSource
  (JNIEnv *env, jobject thiz, jstring path)
{
    setDataSourceAndHeaders(env, thiz, path, 0);
}

JNIEXPORT void JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEGPlayer__1setVideoSurface
  (JNIEnv *env, jobject thiz, jobject jsurface)
{
    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
	if (jsurface == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
	process_media_player_call( env, thiz, mp->setVideoSurface(env, jsurface),
							  "java/io/IOException", "Set video surface failed.");
}

JNIEXPORT void JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEGPlayer_prepare
  (JNIEnv *env, jobject thiz)
{
    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call( env, thiz, mp->prepare(), "java/io/IOException", "Prepare failed." );
}

JNIEXPORT void JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEGPlayer__1start
  (JNIEnv *env, jobject thiz)
{
    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call( env, thiz, mp->start(), NULL, NULL );
}

JNIEXPORT void JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEGPlayer__1stop
  (JNIEnv *env, jobject thiz)
{
    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call( env, thiz, mp->stop(), NULL, NULL );
}

JNIEXPORT void JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEGPlayer__1pause
  (JNIEnv *env, jobject thiz)
{
    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call( env, thiz, mp->pause(), NULL, NULL );
}

JNIEXPORT jboolean JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEGPlayer_isPlaying
  (JNIEnv *env, jobject thiz)
{
    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return false;
    }
    const jboolean is_playing = mp->isPlaying();
    return is_playing;
}

JNIEXPORT void JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEGPlayer_seekTo
  (JNIEnv *env, jobject thiz, jint msec)
{
    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call( env, thiz, mp->seekTo(msec), NULL, NULL );
}

JNIEXPORT jint JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEGPlayer_getVideoWidth
  (JNIEnv *env, jobject thiz)
{
    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return 0;
    }
    int w;
    if (0 != mp->getVideoWidth(&w)) {
        w = 0;
    }
    return w;
}

JNIEXPORT jint JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEGPlayer_getVideoHeight
  (JNIEnv *env, jobject thiz)
{
    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return 0;
    }
    int h;
    if (0 != mp->getVideoHeight(&h)) {
        h = 0;
    }
    return h;
}

JNIEXPORT jint JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEGPlayer_getCurrentPosition
  (JNIEnv *env, jobject thiz)
{
    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return 0;
    }
    int msec;
    process_media_player_call( env, thiz, mp->getCurrentPosition(&msec), NULL, NULL );
    return msec;
}

JNIEXPORT jint JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEGPlayer_getDuration
  (JNIEnv *env, jobject thiz)
{
    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return 0;
    }
    int msec;
    process_media_player_call( env, thiz, mp->getDuration(&msec), NULL, NULL );
    return msec;
}

JNIEXPORT void JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEGPlayer__1reset
  (JNIEnv *env, jobject thiz)
{
    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call( env, thiz, mp->reset(), NULL, NULL );
}

JNIEXPORT void JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEGPlayer_setAudioStreamType
  (JNIEnv *env, jobject thiz, jint streamtype)
{
    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call( env, thiz, mp->setAudioStreamType(streamtype) , NULL, NULL );
}

JNIEXPORT void JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEGPlayer_native_1init
  (JNIEnv *env, jclass c)
{
    __android_log_print(ANDROID_LOG_INFO, TAG, "native_init");
    jclass clazz;
    clazz = env->FindClass("com/v2soft/spoiq/ffmpeg/core/FFMPEGPlayer");
    if (clazz == NULL) {
        jniThrowException(env, "java/lang/RuntimeException", "Can't find android/media/MediaPlayer");
        return;
    }

    fields.context = env->GetFieldID(clazz, "mNativeContext", "I");
    if (fields.context == NULL) {
        jniThrowException(env, "java/lang/RuntimeException", "Can't find MediaPlayer.mNativeContext");
        return;
    }

    fields.post_event = env->GetStaticMethodID(clazz, "postEventFromNative",
                                                   "(Ljava/lang/Object;IIILjava/lang/Object;)V");
    if (fields.post_event == NULL) {
        jniThrowException(env, "java/lang/RuntimeException", "Can't find FFMpegMediaPlayer.postEventFromNative");
        return;
    }
}

JNIEXPORT void JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEGPlayer_native_1setup
  (JNIEnv *env, jobject thiz, jobject weak_thiz)
{
	__android_log_print(ANDROID_LOG_INFO, TAG, "native_setup");
    MediaPlayer* mp = new MediaPlayer();
    if (mp == NULL) {
        jniThrowException(env, "java/lang/RuntimeException", "Out of memory");
        return;
    }
    // create new listener and give it to MediaPlayer
    JNIFFmpegMediaPlayerListener* listener = new JNIFFmpegMediaPlayerListener(env, thiz, weak_thiz);
    mp->setListener(listener);

    // Stow our new C++ MediaPlayer in an opaque field in the Java object.
    setMediaPlayer(env, thiz, mp);
}

JNIEXPORT void JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEGPlayer__1release
  (JNIEnv *env, jobject thiz)
{
}

JNIEXPORT void JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEGPlayer_native_1finalize
  (JNIEnv *env, jobject thiz)
{
    __android_log_print(ANDROID_LOG_INFO, TAG, "native_finalize");
    Java_com_v2soft_spoiq_ffmpeg_core_FFMPEGPlayer__1release(env, thiz);
}

JNIEXPORT jint JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEGPlayer_native_1suspend_1resume
  (JNIEnv *env, jobject thiz, jboolean isSuspend)
{
    MediaPlayer* mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return UNKNOWN_ERROR;
    }

    return isSuspend ? mp->suspend() : mp->resume();
}
