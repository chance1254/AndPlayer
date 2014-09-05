LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE:= ics
LOCAL_SRC_FILES:= lib/libiomx-ics.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE:= jellybean
LOCAL_SRC_FILES:= lib/libiomx-jbmr2.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE:= kitkat
LOCAL_SRC_FILES:= lib/libiomx-kk.so
include $(PREBUILT_SHARED_LIBRARY)