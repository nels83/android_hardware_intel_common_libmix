LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)


LOCAL_SRC_FILES := \
    VideoDecoderHost.cpp \
    VideoDecoderBase.cpp \
    VideoDecoderWMV.cpp \
    VideoDecoderMPEG4.cpp \
    VideoDecoderAVC.cpp \
    VideoDecoderPAVC.cpp \
    VideoDecoderAVCSecure.cpp \
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

LOCAL_COPY_HEADERS_TO  := libmix_videodecoder

LOCAL_COPY_HEADERS := \
    VideoDecoderHost.h \
    VideoDecoderInterface.h \
    VideoDecoderDefs.h

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libva_videodecoder

# Add source codes for Merrifield
MERRIFIELD_PRODUCT := \
        mrfl_vp \
        mrfl_hvp \
        mrfl_sle
ifneq ($(filter $(TARGET_PRODUCT),$(MERRIFIELD_PRODUCT)),)
LOCAL_SRC_FILES += VideoDecoderVP8.cpp
LOCAL_CFLAGS += -DUSE_HW_VP8
endif

include $(BUILD_SHARED_LIBRARY)
