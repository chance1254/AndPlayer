LOCAL_PATH := $(call my-dir)

#the yuv2rgb library
include $(CLEAR_VARS)
LOCAL_ALLOW_UNDEFINED_SYMBOLS=false

LOCAL_CFLAGS += -I$(LOCAL_PATH)/include -D__STDC_CONSTANT_MACROS 
LOCAL_CFLAGS += $(CC_OPTIMIZE_FLAG) 

LOCAL_ARM_MODE := $(APP_ARM_MODE)

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
	LOCAL_ARM_NEON := $(ENABLE_NEON)
	ifeq ($(ENABLE_NEON),true)		
		LOCAL_SRC_FILES := src/yuv2rgb16tab.c src/yuv420rgb888c.c src/yuv420rgb8888neon.c src/i420_rgb8888.S src/yuv420rgb565neon.c src/i420_rgb565.S
		#LOCAL_SRC_FILES := src/yuv2rgb16tab.c src/yuv420rgb888c.c src/yuv420rgb8888neon.c src/i420_rgb.S src/yuv420rgb565.s
		#LOCAL_SRC_FILES := src/yuv2rgb16tab.c src/yuv420rgb888c.c src/yuv420rgb8888c.c src/yuv420rgb565.s
	else
		LOCAL_SRC_FILES := src/yuv2rgb16tab.c src/yuv420rgb888c.c src/yuv420rgb8888c.c src/yuv420rgb565.s
	endif
else
		LOCAL_SRC_FILES := src/yuv2rgb16tab.c src/yuv420rgb888c.c src/yuv420rgb8888c.c src/yuv420rgb565.s
endif

#LOCAL_SRC_FILES := src/yuv2rgb16tab.c src/yuv420rgb8888.s src/yuv420rgb565.s 
#LOCAL_SRC_FILES := src/yuv2rgb16tab.c src/yuv420rgb888c.c src/yuv420rgb8888c.c src/yuv420rgb565.s
#LOCAL_SRC_FILES := src/yuv2rgb16tab.c src/yuv420rgb8888.s src/yuv420rgb565.s src/yuv422rgb565.s src/yuv2rgb555.s src/yuv2rgbX.s src/yuv420rgb888.s src/yuv422rgb565.s src/yuv422rgb888.s src/yuv422rgb8888.s src/yuv444rgb565.s src/yuv444rgb888.s src/yuv444rgb8888.s

ifeq ($(TARGET_ARCH_ABI),x86)
   LOCAL_SRC_FILES := src/yuv2rgb16tab.c src/yuv420rgb8888c.c src/yuv420rgb565c.c
endif

LOCAL_MODULE := yuv2rgb

LOCAL_SHARED_LIBRARIES := 
LOCAL_STATIC_LIBRARIES := 

include $(LOCAL_PATH)/../android-ndk-profiler-3.1/android-ndk-profiler.mk
include $(BUILD_STATIC_LIBRARY)

