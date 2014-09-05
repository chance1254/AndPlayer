LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CFLAGS := -D__STDC_CONSTANT_MACROS -g
LOCAL_LDLIBS := -llog -landroid

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../libffmpeg \
    $(LOCAL_PATH)/../include

LOCAL_SRC_FILES += \
    packetqueue.cpp \
    output.cpp \
    mediaplayer.cpp \
    decoder.cpp \
    decoder_audio.cpp \
    decoder_video.cpp \
    thread.cpp \
    surface.c \
    audiotrack.c

LOCAL_STATIC_LIBRARIES := libavcodec libavformat libavutil libswscale

LOCAL_MODULE := libmediaplayer

include $(BUILD_SHARED_LIBRARY)