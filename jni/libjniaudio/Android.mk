LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CFLAGS := -D__STDC_CONSTANT_MACROS

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../include

LOCAL_SRC_FILES += \
    jniaudio.cpp \

LOCAL_LDLIBS := -llog

#LOCAL_SHARED_LIBRARIES := libffmpeg_jni

LOCAL_ARM_MODE := $(APP_ARM_MODE)
LOCAL_MODULE := libjniaudio

include $(BUILD_SHARED_LIBRARY)

