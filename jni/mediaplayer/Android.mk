LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CFLAGS := -D__STDC_CONSTANT_MACROS
LOCAL_LDLIBS := -llog

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../ffmpeg \
    $(LOCAL_PATH)/../jni \
	$(LOCAL_PATH)/../android-aac-enc \
	$(LOCAL_PATH)/../lame-3.99.5/include \
	$(LOCAL_PATH)/../yuv2rgb/include \
	$(LOCAL_PATH)/../LibReverbEffect

LOCAL_SRC_FILES += \
    utils.cpp \
    atomic.c \
	RefBase.cpp \
	time_source.cpp \
	TimedEventQueue.cpp\
    packetqueue.cpp \
	video_output.cpp \
	audio_output.cpp \
    output.cpp \
    mediaplayer.cpp \
    decoder.cpp \
    decoder_audio.cpp \
    decoder_video.cpp \
	encoder_audio.cpp \
	encoder_audio_aac.cpp \
	encoder_audio_mp3.cpp \
    thread.cpp
    
LOCAL_STATIC_LIBRARIES := libavcodec libavfilter libavformat libavutil libswresample libswscale libyuv2rgb libreverb libmp3lame

LOCAL_MODULE := libmediaplayer

include $(LOCAL_PATH)/../android-ndk-profiler-3.1/android-ndk-profiler.mk

include $(BUILD_STATIC_LIBRARY)