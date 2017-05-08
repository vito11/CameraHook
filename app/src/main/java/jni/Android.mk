LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    inject.c \
    ptrace.c \
    elf.c


LOCAL_C_INCLUDES := $(LOCAL_PATH)
LOCAL_MODULE:= injector
LOCAL_LDLIBS :=-llog
TARGET_OUT =./src/main/assets
LOCAL_CFLAGS := -DANDROID

include $(BUILD_EXECUTABLE)


include $(CLEAR_VARS)


sources := \
    source/compare.cc           \
    source/compare_common.cc    \
    source/convert.cc           \
    source/convert_argb.cc      \
    source/convert_from.cc      \
    source/convert_from_argb.cc \
    source/convert_to_argb.cc   \
    source/convert_to_i420.cc   \
    source/cpu_id.cc            \
    source/planar_functions.cc  \
    source/rotate.cc            \
    source/rotate_any.cc        \
    source/rotate_argb.cc       \
    source/rotate_common.cc     \
    source/row_any.cc           \
    source/row_common.cc        \
    source/scale.cc             \
    source/scale_any.cc         \
    source/scale_argb.cc        \
    source/scale_common.cc      \
    source/video_common.cc      \
    hook.c


LOCAL_SRC_FILES := $(sources)

LOCAL_C_INCLUDES := $(LOCAL_PATH)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include

LOCAL_MODULE:= hook
LOCAL_LDLIBS :=-llog
TARGET_OUT =./src/main/assets

include $(BUILD_SHARED_LIBRARY)