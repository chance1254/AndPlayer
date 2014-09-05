#include "com_v2soft_spoiq_ffmpeg_core_FFMPEGAVRational.h"

#define TAG "android_media_FFMpegAVRational"

#include <android/log.h>
#include "jniUtils.h"
#include "methods.h"

struct fields_t
{
    jmethodID constructor;
};
static struct fields_t fields;

jclass *AVRational_getClass(JNIEnv *env) {
	return (*env)->FindClass(env, "com/v2soft/spoiq/ffmpeg/core/FFMPEGAVRational");
}

const char *AVRational_getClassSignature() {
	return "Lcom/v2soft/spoiq/ffmpeg/core/FFMPEGAVRational;";
}

jobject *AVRational_create(JNIEnv *env, AVRational *rational) {
	jclass *clazz = AVRational_getClass(env);
	jobject result = (*env)->NewObject(env, clazz, fields.constructor);

	// set native pointer to java class for later use
	(*env)->SetIntField(env, result, (*env)->GetFieldID(env, clazz, "mNum", "I"), (jint)rational->num);
	(*env)->SetIntField(env, result, (*env)->GetFieldID(env, clazz, "mDen", "I"), (jint)rational->den);
	return result;
}

JNIEXPORT void JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEGAVRational_nativeRelease
  (JNIEnv *env, jobject thiz, jint pointer)
{
    AVFormatContext *fileContext = (AVFormatContext *) pointer;
	__android_log_print(ANDROID_LOG_INFO, TAG,  "releasing FFMpegAVFormatContext"),
	free(fileContext);
}
