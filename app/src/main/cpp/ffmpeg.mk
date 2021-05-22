MY_DIR := $(call my-dir)

ifeq ($(TARGET_ARCH),arm)
    ARCH_DIR=armeabi-v7a
else ifeq ($(TARGET_ARCH),arm64)
    ARCH_DIR=arm64-v8a
else ifeq ($(TARGET_ARCH),x86)
    ARCH_DIR=x86
else ifeq ($(TARGET_ARCH),x86_64)
    ARCH_DIR=x86_64
endif

LOCAL_PATH=${MY_DIR}/../obj/local/${ARCH_DIR}
include $(CLEAR_VARS)

LOCAL_MODULE    := mp3lame
LOCAL_SRC_FILES := libmp3lame.a
include $(PREBUILT_STATIC_LIBRARY)

LOCAL_PATH=${MY_DIR}/ffmpeg-build/${ARCH_DIR}/lib
include $(CLEAR_VARS)

LOCAL_MODULE    := avcodec
LOCAL_SRC_FILES := \
libavcodec.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE    := avformat
LOCAL_SRC_FILES := \
libavformat.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE    := avfilter
LOCAL_SRC_FILES := \
libavfilter.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE    := avdevice
LOCAL_SRC_FILES := \
libavdevice.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE    := avutil
LOCAL_SRC_FILES := \
libavutil.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE    := postproc
LOCAL_SRC_FILES := \
libpostproc.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE    := swresample
LOCAL_SRC_FILES := \
libswresample.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE    := swscale
LOCAL_SRC_FILES := \
libswscale.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := app

NDK_APP_DST_DIR=${MY_DIR}/../jniLibs/${ARCH_DIR}

LOCAL_SRC_FILES := \


LOCAL_STATIC_LIBRARIES := mp3lame avdevice avcodec avformat avfilter avutil postproc swresample swscale
include $(BUILD_SHARED_LIBRARY)