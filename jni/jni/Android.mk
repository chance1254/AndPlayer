LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CFLAGS := -D__STDC_CONSTANT_MACROS
LOCAL_LDLIBS := -llog

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/include

LOCAL_SRC_FILES += \
	source/onLoad.cpp \
    source/com_media_ffmpeg_FFMpegAVFrame.cpp \
    source/com_media_ffmpeg_FFMpegAVInputFormat.c \
    source/com_media_ffmpeg_FFMpegAVRational.c \
    source/com_media_ffmpeg_FFMpegAVFormatContext.c \
    source/com_media_ffmpeg_FFMpegAVCodecContext.cpp \
    source/com_media_ffmpeg_FFMpegUtils.cpp

LOCAL_STATIC_LIBRARIES := libavcodec libavfilter libavformat libavutil libswresample libswscale

LOCAL_MODULE := libffmpeg-jni

include $(BUILD_SHARED_LIBRARY)