#include "com_v2soft_spoiq_ffmpeg_core_FFMPEGAVInputFormat.h"

#define TAG "android_media_FFMpegAVInputFormat"

#include <android/log.h>
#include "jniUtils.h"
#include "methods.h"

struct fields_t
{
    jmethodID constructor;
};
static struct fields_t fields;

jclass *AVInputFormat_getClass(JNIEnv *env) {
	return (*env)->FindClass(env, "com/v2soft/spoiq/ffmpeg/core/FFMPEGAVInputFormat");
}

const char *AVInputFormat_getClassSignature() {
	return "Lcom/v2soft/spoiq/ffmpeg/core/FFMPEGAVInputFormat;";
}

jobject *AVInputFormat_create(JNIEnv *env, AVInputFormat *format) {
	jclass *clazz = AVInputFormat_getClass(env);
	jobject result = (*env)->NewObject(env, clazz, fields.constructor);

	(*env)->SetIntField(env, result,
						(*env)->GetFieldID(env, clazz, "mPrivDataSize", "I"),
						format->priv_data_size);
	(*env)->SetIntField(env, result,
						(*env)->GetFieldID(env, clazz, "mFlags", "I"),
						format->flags);

	(*env)->SetObjectField(env, result,
						   (*env)->GetFieldID(env, clazz, "mName", "Ljava/lang/String;"),
						   (*env)->NewStringUTF(env, format->name));
	(*env)->SetObjectField(env, result,
						   (*env)->GetFieldID(env, clazz, "mName", "Ljava/lang/String;"),
						   (*env)->NewStringUTF(env, format->long_name));
	(*env)->SetObjectField(env, result,
						   (*env)->GetFieldID(env, clazz, "mExtensions","Ljava/lang/String;"),
						   (*env)->NewStringUTF(env, format->extensions));

	return result;
}

JNIEXPORT void JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEGAVInputFormat_nativeRelease
  (JNIEnv *env, jobject thiz, jint pointer)
{
    AVFormatContext *fileContext = (AVFormatContext *) pointer;
	__android_log_print(ANDROID_LOG_INFO, TAG,  "releasing FFMpegAVInputFormat"),
	free(fileContext);
}