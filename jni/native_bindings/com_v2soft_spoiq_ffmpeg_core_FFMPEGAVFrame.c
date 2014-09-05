#include "com_v2soft_spoiq_ffmpeg_core_FFMPEGAVFrame.h"

#define TAG "android_media_FFMpegAVFrame"

#include <android/log.h>
#include "jniUtils.h"
#include "methods.h"

struct fields_t
{
    jmethodID constructor;
};
static struct fields_t fields;

jclass AVFrame_getClass(JNIEnv *env) {
	return (*env)->FindClass(env,"com/v2soft/spoiq/ffmpeg/core/FFMPEGAVFrame");
}

const char *AVFrame_getClassSignature() {
	return "Lcom/v2soft/spoiq/ffmpeg/core/FFMPEGAVFrame;";
}

jobject AVFrame_create(JNIEnv *env, AVFrame *frame) {
	jclass clazz = AVFrame_getClass(env);
	jobject result = (*env)->NewObject(env,clazz, fields.constructor);

	(*env)->SetIntField(env,result,
					 (*env)->GetFieldID(env,clazz, "mPointer", "I"),
					 (jint)frame);
	(*env)->SetIntField(env,result,
					 (*env)->GetFieldID(env,clazz, "mKeyFrame", "I"),
					 frame->key_frame);
	(*env)->SetIntField(env,result,
					 (*env)->GetFieldID(env,clazz, "mPictType", "I"),
					 frame->pict_type);
	(*env)->SetIntField(env,result,
					 (*env)->GetFieldID(env,clazz, "mCodedPictureNumber", "I"),
					 frame->coded_picture_number);
	(*env)->SetIntField(env,result,
					 (*env)->GetFieldID(env,clazz, "mDisplayPictureNumber", "I"),
					 frame->display_picture_number);
	(*env)->SetIntField(env,result,
					 (*env)->GetFieldID(env,clazz, "mQuality", "I"),
					 frame->quality);
	(*env)->SetIntField(env,result,
					 (*env)->GetFieldID(env,clazz, "mReference", "I"),
					 frame->reference);
	(*env)->SetIntField(env,result,
					 (*env)->GetFieldID(env,clazz, "mQstride", "I"),
					 frame->qstride);
	(*env)->SetIntField(env,result,
					 (*env)->GetFieldID(env,clazz, "mType", "I"),
					 frame->type);
	(*env)->SetIntField(env,result,
					 (*env)->GetFieldID(env,clazz, "mRepeatPict", "I"),
					 frame->repeat_pict);
	(*env)->SetIntField(env,result,
					 (*env)->GetFieldID(env,clazz, "mQscaleType", "I"),
					 frame->qscale_type);
	(*env)->SetIntField(env,result,
					 (*env)->GetFieldID(env,clazz, "mInterlacedFrame", "I"),
					 frame->interlaced_frame);
	(*env)->SetIntField(env,result,
					 (*env)->GetFieldID(env,clazz, "mTopFieldFirst", "I"),
					 frame->top_field_first);
	(*env)->SetIntField(env,result,
					 (*env)->GetFieldID(env,clazz, "mPaletteHasChanged", "I"),
					 frame->palette_has_changed);
	(*env)->SetIntField(env,result,
					 (*env)->GetFieldID(env,clazz, "mBufferHints", "I"),
					 frame->buffer_hints);

	return result;
}

JNIEXPORT void JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEGAVFrame_nativeRelease
  (JNIEnv *env, jobject thiz, jint pointer)
{
    AVFormatContext *fileContext = (AVFormatContext *) pointer;
	__android_log_print(ANDROID_LOG_INFO, TAG,  "releasing FFMpegAVFrame"),
	free(fileContext);
}