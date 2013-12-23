
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES += \
    JPEGDecoder.cpp \
    JPEGBlitter.cpp \
    JPEGParser.cpp \
    ImageDecoderTrace.cpp

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH) \
    $(TARGET_OUT_HEADERS)/libva \
    $(TARGET_OUT_HEADERS)/libmix_videovpp

LOCAL_COPY_HEADERS_TO  := libjpegdec

LOCAL_COPY_HEADERS := \
    JPEGDecoder.h \
    JPEGCommon.h \
    ImageDecoderTrace.h

LOCAL_SHARED_LIBRARIES += \
    libcutils \
    libutils \
    libva-android     \
    libva             \
    libva-tpi		  \
    libhardware

LOCAL_LDLIBS += -lpthread
LOCAL_CFLAGS += -Wno-multichar

ifeq ($(TARGET_BOARD_PLATFORM),baytrail)
LOCAL_SRC_FILES += JPEGBlitter_gen.cpp
LOCAL_SRC_FILES += JPEGDecoder_gen.cpp
else
LOCAL_SRC_FILES += JPEGBlitter_img.cpp
LOCAL_SRC_FILES += JPEGDecoder_img.cpp
endif

LOCAL_MODULE:= libjpegdec
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

ifeq ($(TARGET_BOARD_PLATFORM),baytrail)
include $(CLEAR_VARS)

LOCAL_SRC_FILES += \
    test/testdecode.cpp

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH) \
    $(TARGET_OUT_HEADERS)/libva \
    $(TARGET_OUT_HEADERS)/libmix_videovpp

LOCAL_SHARED_LIBRARIES += \
    libcutils \
    libutils \
    libva-android     \
    libva             \
    libva-tpi         \
    libjpegdec        \
    libhardware

LOCAL_LDLIBS += -lpthread
LOCAL_CFLAGS += -Wno-multichar

LOCAL_MODULE:= testjpegdec
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
endif

include $(CLEAR_VARS)

LOCAL_SRC_FILES += \
    JPEGDecoder_libjpeg_wrapper.cpp

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH) \
    $(call include-path-for, jpeg) \
    $(TARGET_OUT_HEADERS)/libva \
    $(TARGET_OUT_HEADERS)/libjpegdec \
    $(TARGET_OUT_HEADERS)/libmix_videovpp

LOCAL_COPY_HEADERS_TO  := libjpeg_hw

LOCAL_COPY_HEADERS := \
    JPEGDecoder_libjpeg_wrapper.h

LOCAL_SHARED_LIBRARIES += \
    libcutils \
    libutils \
    liblog  \
    libjpegdec \
    libhardware

LOCAL_LDLIBS += -lpthread
LOCAL_CFLAGS += -Wno-multichar
LOCAL_CFLAGS += -DUSE_INTEL_JPEGDEC

LOCAL_MODULE:= libjpeg_hw
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

