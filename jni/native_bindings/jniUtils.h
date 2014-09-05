#ifndef jniUtils_h
#define jniUtils_h

#include <stdlib.h>
#include <jni.h>

#ifdef __cplusplus

extern "C" {
#endif

int jniThrowException(JNIEnv* env, const char* className, const char* msg);

JNIEnv* getJNIEnv();

int jniRegisterNativeMethods(JNIEnv* env,
                             const char* className,
                             const JNINativeMethod* gMethods,
                             int numMethods);

#ifdef __cplusplus
}
#endif
#endif