#include "com_v2soft_spoiq_ffmpeg_core_FFMPEG.h"
#include "ffmpegHelpUtils.h"
#include "libswscale/swscale.h"
static int nb_input_files = 2;
static int nb_output_files = 2;
static int video_stream_copy = 0;
static int frame_width  = 0;
static int frame_height = 0;
static int frame_aspect_ratio = 0;
static char *video_codec_name = NULL;
static AVRational frame_rate;
static int video_channel = 0;
static int audio_channels = 1;
static int audio_sample_rate = 44100;
AVCodecContext *avcodec_opts[AVMEDIA_TYPE_NB];
AVFormatContext *avformat_opts;
struct SwsContext *sws_opts;
static unsigned int sws_flags = SWS_BICUBIC;

static const OptionDef options[];

struct fields_t
{
    jmethodID clb_onReport;
};
static struct fields_t fields;
static jobject sObject;

JNIEXPORT jint JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEG_native_1av_1release
  (JNIEnv *env, jobject thiz, jint result)
{
    if (sObject != NULL) {
        (*env)->DeleteGlobalRef(env, sObject);
        sObject = NULL;
    }

    return av_exit(result);
}

static const OptionDef options[] = {};

JNIEXPORT void JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEG_native_1av_1convert
  (JNIEnv *env, jobject thiz)
{
}

JNIEXPORT void JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEG_native_1av_1parse_1options
  (JNIEnv *env, jobject obj, jobjectArray args)
{
    int i = 0;
	int argc = 0;
	char **argv = NULL;

	if (args != NULL) {
		argc = (*env)->GetArrayLength(env, args);
		argv = (char **) malloc(sizeof(char *) * argc);

		for(i=0;i<argc;i++)
		{
			jstring str = (jstring)(*env)->GetObjectArrayElement(env, args, i);
			argv[i] = (char *)(*env)->GetStringUTFChars(env, str, NULL);
		}
	}

	parse_options(argc, argv, options, opt_output_file);

    if(nb_output_files <= 0 && nb_input_files == 0) {
        jniThrowException(env,
                          "java/lang/RuntimeException",
                          "Use -h to get full help or, even better, run 'man ffmpeg");
    }

    /* file converter / grab */
    if (nb_output_files <= 0) {
        jniThrowException(env,
                          "java/lang/RuntimeException",
                          "At least one output file must be specified");
    }

    if (nb_input_files == 0) {
        jniThrowException(env,
                          "java/lang/RuntimeException",
                          "At least one input file must be specified");
    }
}

JNIEXPORT void JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEG_native_1av_1setVideoCodec
  (JNIEnv *env, jobject thiz, jstring codec)
{
    const char *_codec = (*env)->GetStringUTFChars(env, codec, NULL);
	opt_codec(&video_stream_copy, &video_codec_name, AVMEDIA_TYPE_VIDEO, _codec);
}

JNIEXPORT void JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEG_native_1av_1newVideoStream
  (JNIEnv *env, jobject thiz, jint pointer)
{
    AVFormatContext *context = (AVFormatContext *)pointer;
	new_video_stream(context);
}

JNIEXPORT jobject JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEG_native_1av_1setOutputFile
  (JNIEnv *env, jobject thiz, jstring filePath)
{
    const char *_filePath = (*env)->GetStringUTFChars(env, filePath, NULL);
	AVFormatContext *fileContext = opt_output_file(_filePath);
	if(fileContext == NULL) {
		jniThrowException(env,
						  "java/io/IOException",
		                  "Can't create output file");
	}
	return AVFormatContext_create(env, fileContext);
}

JNIEXPORT jobject JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEG_native_1av_1setInputFile
  (JNIEnv *env, jobject thiz, jstring filePath)
{
    const char *_filePath = (*env)->GetStringUTFChars(env, filePath, NULL);
	AVFormatContext *fileContext = opt_input_file(_filePath);
	if(fileContext == NULL) {
			jniThrowException(env,
							  "java/io/IOException",
			                  "Can't create input file");
		}
	jobject *file = AVFormatContext_create(env, fileContext);
	jobject *inFormat = AVInputFormat_create(env, fileContext->iformat);
	jfieldID f = (*env)->GetFieldID(env,
									AVFormatContext_getClass(env),
									"mInFormat",
									AVInputFormat_getClassSignature());
	(*env)->SetObjectField(env, file, f, inFormat);
	return file;
}

JNIEXPORT jint JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEG_native_1av_1setBitrate
  (JNIEnv *env, jobject obj, jstring opt, jstring arg)
{
    const char *_opt = (*env)->GetStringUTFChars(env, opt, NULL);
	const char *_arg = (*env)->GetStringUTFChars(env, arg, NULL);
    return opt_bitrate(_opt, _arg);
}

JNIEXPORT void JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEG_native_1av_1setFrameSize
  (JNIEnv *env, jobject thiz, jint width, jint height)
{
    frame_width = width;
	frame_height = height;
}

JNIEXPORT void JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEG_native_1av_1setFrameAspectRatio
  (JNIEnv *env, jobject thiz, jint x, jint y)
{
    frame_aspect_ratio = (double)x / (double)y;
}

JNIEXPORT jobject JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEG_native_1av_1setFrameRate
  (JNIEnv *env, jobject thiz, jint rate)
{
    frame_rate.num = rate;
	frame_rate.den = 1;
	return AVRational_create(env, &frame_rate);
}

JNIEXPORT void JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEG_native_1av_1setVideoChannel
  (JNIEnv *env, jobject thiz, jint channel)
{
    video_channel = channel;
}

JNIEXPORT void JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEG_native_1av_1setAudioChannels
  (JNIEnv *env, jobject thiz, jint count)
{
    audio_channels = count;
}

JNIEXPORT void JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEG_native_1av_1setAudioRate
  (JNIEnv *env, jobject thiz, jint rate)
{
    audio_sample_rate = rate;
}

JNIEXPORT void JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEG_native_1av_1init
  (JNIEnv *env, jobject obj)
{
    sObject = (*env)->NewGlobalRef(env, obj);
    jclass clazz = (*env)->GetObjectClass(env, obj);
    fields.clb_onReport = (*env)->GetMethodID(env, clazz, "onReport", "(DDD)V");
    if (fields.clb_onReport == NULL) {
        jniThrowException(env,
                          "java/lang/RuntimeException",
                          "can't load clb_onReport callback");
    }

#if HAVE_ISATTY
    if(isatty(STDIN_FILENO))
        url_set_interrupt_cb(decode_interrupt_cb);
#endif

    int i=0;
    for(i=0; i<AVMEDIA_TYPE_NB; i++){
        avcodec_opts[i]= avcodec_alloc_context3(i);
    }
    avformat_opts = avformat_alloc_context();
    sws_opts = sws_getContext(16, 16, 0, 16, 16, 0, sws_flags, NULL, NULL, NULL);
}

void handleReport(double total_size, double time, double bitrate) {
         JNIEnv *env = getJNIEnv();
         if (env == NULL) {
             return;
         }
         (*env)->CallVoidMethod(env,
                                sObject,
                                fields.clb_onReport,
                                total_size,
                                time,
                                bitrate);
}

JNIEXPORT void JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEG_native_1av_1register_1all
  (JNIEnv *env, jobject thiz)
{
    av_register_all();
}

JNIEXPORT void JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEG_native_1avcodec_1register_1all
  (JNIEnv *env, jobject thiz)
{
    avcodec_register_all();
}

