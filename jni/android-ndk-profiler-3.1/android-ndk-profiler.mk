
ifeq ($(ENABLE_LIBRARY_PROFILER), true)
    TARGET_thumb_release_CFLAGS := $(filter-out -ffunction-sections,$(TARGET_thumb_release_CFLAGS))
    TARGET_thumb_release_CFLAGS := $(filter-out -fomit-frame-pointer,$(TARGET_thumb_release_CFLAGS))
    TARGET_arm_release_CFLAGS := $(filter-out -ffunction-sections,$(TARGET_arm_release_CFLAGS))
    TARGET_arm_release_CFLAGS := $(filter-out -fomit-frame-pointer,$(TARGET_arm_release_CFLAGS))
    TARGET_CFLAGS := $(filter-out -ffunction-sections,$(TARGET_CFLAGS))
    LOCAL_CFLAGS += -pg -DENABLE_LIBRARY_PROFILER
    LOCAL_STATIC_LIBRARIES += libandprof
endif
