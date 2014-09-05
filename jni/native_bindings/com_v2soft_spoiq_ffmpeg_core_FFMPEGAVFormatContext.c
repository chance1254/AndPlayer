#define TAG "android_media_FFMpegAVFormatContext"

#include <android/log.h>
#include "jniUtils.h"
#include "methods.h"
#include "com_v2soft_spoiq_ffmpeg_core_FFMPEGAVFormatContext.h"

struct fields_t
{
    jmethodID constructor;
};
static struct fields_t fields;

jclass AVFormatContext_getClass(JNIEnv *env) {
	return (*env)->FindClass(env, "com/v2soft/spoiq/ffmpeg/core/FFMPEGAVFormatContext");
}

const char *AVFormatContext_getClassSignature() {
	return "Lcom/v2soft/spoiq/ffmpeg/core/FFMPEGAVFormatContext;";
}

jobject AVFormatContext_create(JNIEnv *env, AVFormatContext *fileContext) {
	jclass *clazz = AVFormatContext_getClass(env);
	jobject result = (*env)->NewObject(env, clazz, fields.constructor);

	// set native pointer to java class for later use
	(*env)->SetIntField(env, result, (*env)->GetFieldID(env, clazz, "pointer", "I"), (jint)fileContext);

	(*env)->SetIntField(env, result, (*env)->GetFieldID(env, clazz,
			"nb_streams", "I"), fileContext->nb_streams);
	(*env)->SetIntField(env, result, (*env)->GetFieldID(env, clazz, "bit_rate",
			"I"), fileContext->bit_rate);
	(*env)->SetIntField(env, result, (*env)->GetFieldID(env, clazz,
			"packet_size", "I"), fileContext->packet_size);
	(*env)->SetIntField(env, result, (*env)->GetFieldID(env, clazz,
			"max_delay", "I"), fileContext->max_delay);
	(*env)->SetIntField(env, result, (*env)->GetFieldID(env, clazz, "flags",
			"I"), fileContext->flags);

	(*env)->SetLongField(env, result, (*env)->GetFieldID(env, clazz,
			"start_time", "J"), fileContext->start_time);
	(*env)->SetLongField(env, result, (*env)->GetFieldID(env, clazz,
			"duration", "J"), fileContext->duration);
	(*env)->SetObjectField(env, result, (*env)->GetFieldID(env, clazz,
			"filename", "Ljava/lang/String;"), (*env)->NewStringUTF(env,
			fileContext->filename));
	return result;
}

JNIEXPORT void JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEGAVFormatContext_nativeRelease
  (JNIEnv *env, jobject thiz, jint pointer)
{
    AVFormatContext *fileContext = (AVFormatContext *) pointer;
	__android_log_print(ANDROID_LOG_INFO, TAG,  "releasing FFMpegAVFormatContext"),
	free(fileContext);
}