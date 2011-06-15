LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)


LOCAL_SRC_FILES := \
    VideoDecoderHost.cpp \
    VideoDecoderBase.cpp \
    VideoDecoderWMV.cpp \
    VideoDecoderMPEG4.cpp \
    VideoDecoderAVC.cpp \
    VideoDecoderVP8.cpp \
    VideoDecoderTrace.cpp

# LOCAL_CFLAGS :=

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(TARGET_OUT_HEADERS)/libva \
    $(TARGET_OUT_HEADERS)/libmixvbp

#LOCAL_LDLIBS += -lpthread

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libmixvbp \
    libva \
    libva-android \
    libva-tpi


#LOCAL_CFLAGS += -DANDROID


#LOCAL_SHARED_LIBRARIES += liblog


LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libva_videodecoder

include $(BUILD_SHARED_LIBRARY)
