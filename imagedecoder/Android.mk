#ifeq ($(strip $(USE_INTEL_JPEGDEC)),true)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES += \
    JPEGDecoder.c \
    JPEGParser.c \
    ImageDecoderTrace.c

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH) \
    $(TARGET_OUT_HEADERS)/libva

LOCAL_COPY_HEADERS_TO  := libjpeg_hw

LOCAL_COPY_HEADERS := \
    JPEGDecoder.h \
    JPEGParser.h \
    ImageDecoderTrace.h

LOCAL_SHARED_LIBRARIES += \
    libcutils \
    libva-android     \
    libva             \
    libva-tpi         \

LOCAL_LDLIBS += -lpthread
LOCAL_CFLAGS += -Wno-multichar
LOCAL_CFLAGS += -DUSE_INTEL_JPEGDEC
LOCAL_MODULE:= libjpeg_hw
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

#endif

