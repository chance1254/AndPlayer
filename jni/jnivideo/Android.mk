LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CFLAGS := -D__STDC_CONSTANT_MACROS

LOCAL_SRC_FILES += \
    jniVideo.cpp

LOCAL_LDLIBS := -llog
        
LOCAL_SHARED_LIBRARIES := libffmpeg-jni libmediaplayer
        
LOCAL_MODULE := libjnivideo

include $(BUILD_SHARED_LIBRARY)