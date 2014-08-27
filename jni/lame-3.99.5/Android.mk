LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH) \
	$(LOCAL_PATH)/libmp3lame \
    $(LOCAL_PATH)/include \
	
LOCAL_SRC_FILES 	:= \
		./libmp3lame/bitstream.c \
		./libmp3lame/encoder.c \
		./libmp3lame/fft.c \
		./libmp3lame/gain_analysis.c \
		./libmp3lame/id3tag.c \
		./libmp3lame/lame.c \
		./libmp3lame/mpglib_interface.c \
		./libmp3lame/newmdct.c \
		./libmp3lame/presets.c \
		./libmp3lame/psymodel.c \
		./libmp3lame/quantize.c \
		./libmp3lame/quantize_pvt.c \
		./libmp3lame/reservoir.c \
		./libmp3lame/set_get.c \
		./libmp3lame/tables.c \
		./libmp3lame/takehiro.c \
		./libmp3lame/util.c \
		./libmp3lame/vbrquantize.c \
		./libmp3lame/VbrTag.c \
		./libmp3lame/version.c
		
LOCAL_LDLIBS += -llog -lz -lm
LOCAL_CFLAGS += -DHAVE_CONFIG_H -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DANDROID
LOCAL_ARM_MODE := $(APP_ARM_MODE)
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_ARM_NEON := $(ENABLE_NEON)
endif
LOCAL_MODULE   := libmp3lame

include $(LOCAL_PATH)/../android-ndk-profiler-3.1/android-ndk-profiler.mk
include $(BUILD_STATIC_LIBRARY)
