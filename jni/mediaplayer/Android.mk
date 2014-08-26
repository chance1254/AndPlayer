LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CFLAGS := -D__STDC_CONSTANT_MACROS
LOCAL_LDLIBS := -llog

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../ffmpeg 

LOCAL_SRC_FILES += \
	source/thread.cpp \
	source/packetqueue.cpp \
	source/decoder.cpp \
	source/decoder_audio.cpp \
	source/decoder_video.cpp \
	source/mediaplayer.cpp \
	source/android_surface.cpp \
	source/time_source.cpp \
	source/video_output.cpp

LOCAL_STATIC_LIBRARIES := libavcodec libavfilter libavformat libavutil libswresample libswscale

LOCAL_MODULE := libmediaplayer

include $(BUILD_SHARED_LIBRARY)