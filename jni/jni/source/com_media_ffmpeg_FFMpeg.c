#include "../include/onLoad.h"

static JNINativeMethod methods[] = {
	{ "native_avcodec_register_all", "()V", (void*) avcodec_register_all },
	//{ "native_avdevice_register_all", "()V", (void*) avdevice_register_all },
	//{ "native_avfilter_register_all", "()V", (void*) avfilter_register_all },
	{ "native_av_register_all", "()V", (void*) av_register_all },
	{ "native_av_init", "()V", (void*) FFMpeg_init },
	{ "native_av_parse_options", "([Ljava/lang/String;)V", (void*) FFMpeg_parseOptions },
	{ "native_av_convert", "()V", (void*) FFMpeg_convert },
	{ "native_av_release", "(I)I", (void*) FFMpeg_release },
	{ "native_av_setInputFile", "(Ljava/lang/String;)Lcom/media/ffmpeg/FFMpegAVFormatContext;", (void*) FFMpeg_setInputFile},
	{ "native_av_setOutputFile", "(Ljava/lang/String;)Lcom/media/ffmpeg/FFMpegAVFormatContext;", (void*) FFMpeg_setOutputFile},
	{ "native_av_setBitrate", "(Ljava/lang/String;Ljava/lang/String;)I", (void*) FFMpeg_setBitrate },
	{ "native_av_newVideoStream", "(I)V", (void*) FFMpeg_newVideoStream },
	{ "native_av_setAudioRate", "(I)V", (void*) FFMpeg_setAudioRate },
	{ "native_av_setAudioChannels", "(I)V", (void*) FFMpeg_setAudioChannels },
	{ "native_av_setVideoChannel", "(I)V", (void*) FFMpeg_setVideoChannel },
	{ "native_av_setFrameRate", "(I)Lcom/media/ffmpeg/FFMpegAVRational;", (void*) FFMpeg_setFrameRate },
	{ "native_av_setFrameAspectRatio", "(II)V", (void*) FFMpeg_setFrameAspectRatio },
	{ "native_av_setFrameSize", "(II)V", (void*) FFMpeg_setFrameSize },
	{ "native_av_setVideoCodec", "(Ljava/lang/String;)V", (void*) FFMpeg_setVideoCodec },
};

int register_android_media_FFMpeg(JNIEnv *env) {
	return jniRegisterNativeMethods(env, "com/media/ffmpeg/FFMpeg", methods, sizeof(methods) / sizeof(methods[0]));
}
