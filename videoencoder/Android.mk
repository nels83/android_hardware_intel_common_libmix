LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

#VIDEO_ENC_LOG_ENABLE := true
#NO_BUFFER_SHARE := true

ifeq ($(ENABLE_IMG_GRAPHICS),)
LOCAL_CFLAGS += -DBX_RC \
                -DOSCL_IMPORT_REF= -DOSCL_UNUSED_ARG= -DOSCL_EXPORT_REF=

LOCAL_STATIC_LIBRARIES := \
                          libstagefright_m4vh263enc
endif

LOCAL_SRC_FILES :=              \
    VideoEncoderBase.cpp        \
    VideoEncoderAVC.cpp         \
    VideoEncoderH263.cpp        \
    VideoEncoderMP4.cpp         \
    VideoEncoderVP8.cpp         \
    VideoEncoderHost.cpp

ifeq ($(ENABLE_IMG_GRAPHICS),)
    LOCAL_SRC_FILES += PVSoftMPEG4Encoder.cpp
endif

# LOCAL_CFLAGS :=

LOCAL_C_INCLUDES :=             \
    $(LOCAL_PATH)               \
    $(TARGET_OUT_HEADERS)/libva \
    $(TOPDIR)/frameworks/native/include \
    $(TARGET_OUT_HEADERS)/pvr

ifeq ($(ENABLE_IMG_GRAPHICS),)
LOCAL_C_INCLUDES +=             \
    frameworks/av/media/libstagefright/codecs/m4v_h263/enc/include \
    frameworks/av/media/libstagefright/codecs/m4v_h263/enc/src \
    frameworks/av/media/libstagefright/codecs/common/include \
    frameworks/native/include/media/openmax \
    frameworks/native/include/media/hardware \
    frameworks/av/media/libstagefright/include
endif

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

ifeq ($(NO_BUFFER_SHARE),true)
LOCAL_CPPFLAGS += -DNO_BUFFER_SHARE
endif

ifeq ($(VIDEO_ENC_STATISTICS_ENABLE),true)
LOCAL_CPPFLAGS += -DVIDEO_ENC_STATISTICS_ENABLE
endif

ifeq ($(ENABLE_IMG_GRAPHICS),true)
    LOCAL_CFLAGS += -DIMG_GFX
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
