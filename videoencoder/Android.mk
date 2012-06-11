LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

VIDEO_ENC_LOG_ENABLE := true

LOCAL_SRC_FILES :=              \
    VideoEncoderBase.cpp        \
    VideoEncoderAVC.cpp         \
    VideoEncoderH263.cpp        \
    VideoEncoderMP4.cpp         \
    VideoEncoderHost.cpp

# LOCAL_CFLAGS :=

LOCAL_C_INCLUDES :=             \
    $(LOCAL_PATH)               \
    $(TARGET_OUT_HEADERS)/libva \

#LOCAL_LDLIBS += -lpthread

LOCAL_SHARED_LIBRARIES :=       \
        libcutils               \
        libva                   \
        libva-android           \
        libva-tpi

#LOCAL_CFLAGS += -DANDROID

LOCAL_COPY_HEADERS_TO  := libmix_videoencoder

LOCAL_COPY_HEADERS := \
    VideoEncoderHost.h \
    VideoEncoderInterface.h \
    VideoEncoderDef.h

ifeq ($(VIDEO_ENC_LOG_ENABLE),true)
LOCAL_CPPFLAGS += -DVIDEO_ENC_LOG_ENABLE
endif

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libva_videoencoder

include $(BUILD_SHARED_LIBRARY)
