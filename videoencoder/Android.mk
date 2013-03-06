LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

#VIDEO_ENC_LOG_ENABLE := true

LOCAL_SRC_FILES :=              \
    VideoEncoderBase.cpp        \
    VideoEncoderAVC.cpp         \
    VideoEncoderH263.cpp        \
    VideoEncoderMP4.cpp         \
    VideoEncoderVP8.cpp         \
    VideoEncoderHost.cpp

# LOCAL_CFLAGS :=

LOCAL_C_INCLUDES :=             \
    $(LOCAL_PATH)               \
    $(TARGET_OUT_HEADERS)/libva \
    $(TOPDIR)/frameworks/native/include \
    $(TARGET_OUT_HEADERS)/pvr

#LOCAL_LDLIBS += -lpthread

LOCAL_SHARED_LIBRARIES :=       \
        libcutils               \
        libutils               \
        libva                   \
        libva-android           \
        libva-tpi		\
	libintelmetadatabuffer

#LOCAL_CFLAGS += -DANDROID

LOCAL_COPY_HEADERS_TO  := libmix_videoencoder

LOCAL_COPY_HEADERS := \
    VideoEncoderHost.h \
    VideoEncoderInterface.h \
    VideoEncoderDef.h

ifeq ($(VIDEO_ENC_LOG_ENABLE),true)
LOCAL_CPPFLAGS += -DVIDEO_ENC_LOG_ENABLE
endif

ifeq ($(VIDEO_ENC_STATISTICS_ENABLE),true)
LOCAL_CPPFLAGS += -DVIDEO_ENC_STATISTICS_ENABLE
endif

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libva_videoencoder

include $(BUILD_SHARED_LIBRARY)

# For libintelmetadatabuffer
# =====================================================

include $(CLEAR_VARS)

VIDEO_ENC_LOG_ENABLE := true

LOCAL_SRC_FILES :=              \
    IntelMetadataBuffer.cpp

LOCAL_C_INCLUDES :=             \
    $(LOCAL_PATH)               

LOCAL_COPY_HEADERS_TO  := libmix_videoencoder

LOCAL_COPY_HEADERS := \
    IntelMetadataBuffer.h

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libintelmetadatabuffer

include $(BUILD_SHARED_LIBRARY)
