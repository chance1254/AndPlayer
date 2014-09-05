#include "com_v2soft_spoiq_ffmpeg_core_FFMPEGAVCodecContext.h"

#define TAG "android_media_FFMpegAVCodecContext"

#include <android/log.h>
#include "jniUtils.h"
#include "methods.h"

struct fields_t
{
    jmethodID constructor;
};
static struct fields_t fields;

jclass AVCodecContext_getClass(JNIEnv *env) {
	return (*env)->FindClass(env,"com/v2soft/spoiq/ffmpeg/core/FFMPEGAVCodecContext");
}

const char *AVCodecContext_getClassSignature() {
	return "Lcom/v2soft/spoiq/ffmpeg/core/FFMPEGAVCodecContext;";
}

jobject AVCodecContext_create(JNIEnv *env, AVCodecContext *codecContext) {
	jclass clazz = AVCodecContext_getClass(env);
	jobject result = (*env)->NewObject(env, clazz, fields.constructor);

	//env->SetIntField(result,
	//				 env->GetFieldID(clazz, "mPointer", "I"),
	//				 (jint)frame);
	(*env)->SetIntField(env,result,
					 (*env)->GetFieldID(env,clazz, "mBitRate", "I"),
					 codecContext->bit_rate);
	(*env)->SetIntField(env,result,
					 (*env)->GetFieldID(env,clazz, "mBitRateTolerance", "I"),
					 codecContext->bit_rate_tolerance);
	(*env)->SetIntField(env,result,
					 (*env)->GetFieldID(env,clazz, "mFlags", "I"),
					 codecContext->flags);
	(*env)->SetIntField(env,result,
					 (*env)->GetFieldID(env,clazz, "mMeMethod", "I"),
					 codecContext->me_method);
	(*env)->SetIntField(env,result,
					 (*env)->GetFieldID(env,clazz, "mExtradataSize", "I"),
					 codecContext->extradata_size);
	(*env)->SetIntField(env,result,
					 (*env)->GetFieldID(env,clazz, "mWidth", "I"),
					 codecContext->width);
	(*env)->SetIntField(env,result,
					 (*env)->GetFieldID(env,clazz, "mHeight", "I"),
					 codecContext->height);
	(*env)->SetIntField(env,result,
					 (*env)->GetFieldID(env,clazz, "mGopSize", "I"),
					 codecContext->gop_size);
	(*env)->SetIntField(env,result,
					 (*env)->GetFieldID(env,clazz, "mSampleRate", "I"),
					 codecContext->sample_rate);
	(*env)->SetIntField(env,result,
					 (*env)->GetFieldID(env,clazz, "mChannels", "I"),
					 codecContext->channels);
	(*env)->SetIntField(env,result,
					 (*env)->GetFieldID(env,clazz, "mFrameSize", "I"),
					 codecContext->frame_size);
	(*env)->SetIntField(env,result,
					 (*env)->GetFieldID(env,clazz, "mFrameNumber", "I"),
					 codecContext->frame_number);

	return result;
}

JNIEXPORT void JNICALL Java_com_v2soft_spoiq_ffmpeg_core_FFMPEGAVCodecContext_release
  (JNIEnv *env, jobject pointer)
{
    AVFormatContext *fileContext = (AVFormatContext *) pointer;
	__android_log_print(ANDROID_LOG_INFO, TAG,  "releasing FFMpegAVCodecContext"),
	free(fileContext);
}
