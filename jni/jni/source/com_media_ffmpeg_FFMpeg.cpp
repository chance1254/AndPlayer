extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
}
#include "jni.h"
#include "../include/com_media_ffmpeg_FFMpeg.h"
#include "../include/com_media_ffmpeg_FFMpeg_IFFMpegListener.h"

JNIEXPORT void JNICALL Java_com_media_ffmpeg_FFMpeg_native_1avcodec_1register_1all
  (JNIEnv *env, jobject thiz)
{
	avcodec_register_all();
}

JNIEXPORT void JNICALL Java_com_media_ffmpeg_FFMpeg_native_1av_1register_1all
  (JNIEnv *env, jobject thiz)
{
	av_register_all();
}
