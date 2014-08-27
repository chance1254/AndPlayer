LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

#LOCAL_CFLAGS := -D__STDC_CONSTANT_MACROS
LOCAL_CPPFLAGS := -fpermissive

#LOCAL_C_INCLUDES += $(LOCAL_PATH)/../include

LOCAL_SRC_FILES += \
    api.c \
    freeverb.c \
    reverb.c

#LOCAL_SHARED_LIBRARIES := libReverbEffect_jni

LOCAL_ARM_MODE := $(APP_ARM_MODE)
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_ARM_NEON := $(ENABLE_NEON)
endif
LOCAL_MODULE := libreverb

include $(LOCAL_PATH)/../android-ndk-profiler-3.1/android-ndk-profiler.mk
include $(BUILD_STATIC_LIBRARY)

