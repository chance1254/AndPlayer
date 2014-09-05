LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

IN_NDK := true

LOCAL_CFLAGS := -D__STDC_CONSTANT_MACROS

WITH_CONVERTOR := true
WITH_PLAYER := true

ifeq ($(WITH_PLAYER),true)
LOCAL_CFLAGS += -DBUILD_WITH_PLAYER
endif

ifeq ($(WITH_CONVERTOR),true)
LOCAL_CFLAGS += -DBUILD_WITH_CONVERTOR
endif

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../libffmpeg \
    $(LOCAL_PATH)/../libmediaplayer \
    $(LOCAL_PATH)/../include

LOCAL_SRC_FILES := \
    jniUtils.c \
    com_v2soft_spoiq_ffmpeg_core_FFMPEGAVFormatContext.c \
    com_v2soft_spoiq_ffmpeg_core_FFMPEGUtils.c \
    com_v2soft_spoiq_ffmpeg_core_FFMPEGAVFrame.c \
    com_v2soft_spoiq_ffmpeg_core_FFMPEGAVInputFormat.c \
    com_v2soft_spoiq_ffmpeg_core_FFMPEGAVRational.c \
    com_v2soft_spoiq_ffmpeg_core_FFMPEGPlayer.cpp \
    com_v2soft_spoiq_ffmpeg_core_FFMPEGAVCodecContext.c \
    ffmpegHelpUtils.c \
    com_v2soft_spoiq_ffmpeg_core_FFMPEG.c

ifeq ($(IN_NDK),true)
LOCAL_LDLIBS := -llog
else
LOCAL_PRELINK_MODULE := false
LOCAL_SHARED_LIBRARIES := liblog
endif

LOCAL_SHARED_LIBRARIES := libjniaudio libjnivideo
LOCAL_STATIC_LIBRARIES := libavcodec libavformat libavutil libswscale libmediaplayer

LOCAL_MODULE := libffmpeg_jni

include $(BUILD_SHARED_LIBRARY)