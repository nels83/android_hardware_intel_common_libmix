LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    VideoDecoderHost.cpp \
    VideoDecoderBase.cpp \
    VideoDecoderWMV.cpp \
    VideoDecoderMPEG4.cpp \
    VideoDecoderAVC.cpp \
    VideoDecoderPAVC.cpp \
    VideoDecoderTrace.cpp \

# LOCAL_CFLAGS :=

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(TARGET_OUT_HEADERS)/libva \
    $(TARGET_OUT_HEADERS)/libmixvbp

ifeq ($(USE_INTEL_SECURE_AVC),true)
LOCAL_SRC_FILES += securevideo/$(TARGET_BOARD_PLATFORM)/VideoDecoderAVCSecure.cpp
LOCAL_C_INCLUDES += $(LOCAL_PATH)/securevideo/$(TARGET_BOARD_PLATFORM)
endif

ifeq ($(TARGET_BOARD_PLATFORM),baytrail)
LOCAL_CFLAGS += -DLOAD_PVR_DRIVER
endif

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

ifeq ($(USE_HW_VP8),true)
LOCAL_SRC_FILES += VideoDecoderVP8.cpp
LOCAL_CFLAGS += -DUSE_HW_VP8
endif

include $(BUILD_SHARED_LIBRARY)
