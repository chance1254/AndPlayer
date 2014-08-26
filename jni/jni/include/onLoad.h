#ifndef ONLOAD_H_
#define ONLOAD_H_
#include <stdlib.h>
#include <android/log.h>
#include "jniUtils.h"

static JavaVM *javaVM;
int jniThrowException(JNIEnv* env, const char* className, const char* msg);
JNIEnv* getJNIEnv();
int jniRegisterNativeMethods(JNIEnv* env,
                             const char* className,
                             const JNINativeMethod* gMethods,
                             int numMethods);
jint JNI_OnLoad(JavaVM* vm, void* reserved);
#endif /* ONLOAD_H_ */
