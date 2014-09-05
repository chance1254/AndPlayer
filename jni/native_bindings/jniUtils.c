#include <android/log.h>
#include "jniUtils.h"

#define TAG "jniUtils"

static JavaVM *sVm;

int jniThrowException(JNIEnv* env, const char* className, const char* msg) {
    jclass exceptionClass = (*env)->FindClass(env,className);
    if (exceptionClass == NULL) {
        __android_log_print(ANDROID_LOG_ERROR,
			    TAG,
			    "Unable to find exception class %s",
	                    className);
        return -1;
    }

    if ((*env)->ThrowNew(env,exceptionClass, msg) != JNI_OK) {
        __android_log_print(ANDROID_LOG_ERROR,
			    TAG,
			    "Failed throwing '%s' '%s'",
			    className, msg);
    }
    return 0;
}

JNIEnv* getJNIEnv() {
    JNIEnv* env = NULL;
    if ((*sVm)->GetEnv(sVm, (void**) &env, JNI_VERSION_1_4) != JNI_OK) {
    	__android_log_print(ANDROID_LOG_ERROR,
							TAG,
							"Failed to obtain JNIEnv");
    	return NULL;
    }
    return env;
}

int jniRegisterNativeMethods(JNIEnv* env,
                             const char* className,
                             const JNINativeMethod* gMethods,
                             int numMethods)
{
    jclass clazz;

    __android_log_print(ANDROID_LOG_INFO, TAG, "Registering %s natives\n", className);
    clazz = (*env)->FindClass(env,className);
    if (clazz == NULL) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "Native registration unable to find class '%s'\n", className);
        return -1;
    }
    if ((*env)->RegisterNatives(env,clazz, gMethods, numMethods) < 0) {
        __android_log_print(ANDROID_LOG_ERROR, TAG, "RegisterNatives failed for '%s'\n", className);
        return -1;
    }
    return 0;
}



