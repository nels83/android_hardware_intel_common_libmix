
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
LOCAL_CFLAGS += -Wno-multichar -DLOG_TAG=\"ImageDecoder\"
LOCAL_CFLAGS += -DLOG_NDEBUG=0

ifeq ($(TARGET_BOARD_PLATFORM),baytrail)
GPGPU_OBJ_NAME := libjpeg_cm_genx.isa
GPGPU_OBJS += $(PRODUCT_OUT)/system/lib/$(GPGPU_OBJ_NAME)
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/ufo
LOCAL_SRC_FILES += JPEGBlitter_gen.cpp
LOCAL_SRC_FILES += JPEGDecoder_gen.cpp
LOCAL_C_INCLUDES += $(TOP)/vendor/intel/hardware/PRIVATE/ufo/inc
LOCAL_CFLAGS += -Wno-non-virtual-dtor -DGFXGEN
LOCAL_LDFLAGS += -L$(INTEL_CM_RUNTIME)/lib/x86/ -l:igfxcmrt32.so
$(GPGPU_OBJS):
	cp $(LOCAL_PATH)/$(GPGPU_OBJ_NAME) $@
else
LOCAL_SRC_FILES += JPEGBlitter_img.cpp
LOCAL_SRC_FILES += JPEGDecoder_img.cpp
endif
LOCAL_MODULE:= libjpegdec
LOCAL_MODULE_TAGS := optional

$(LOCAL_MODULE): $(GPGPU_OBJS)

include $(BUILD_SHARED_LIBRARY)

ifeq ($(TARGET_BOARD_PLATFORM),baytrail)
include $(CLEAR_VARS)

LOCAL_SRC_FILES += \
    test/testdecode.cpp

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH) \
    $(TARGET_OUT_HEADERS)/libva \

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
LOCAL_CFLAGS += -DLOG_NDEBUG=0

LOCAL_MODULE:= testjpegdec
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
endif

include $(CLEAR_VARS)

LOCAL_SRC_FILES += \
    JPEGDecoder_libjpeg_wrapper.cpp

ifeq ($(TARGET_BOARD_PLATFORM),baytrail)
LOCAL_CFLAGS += -DGFXGEN
endif

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH) \
    $(call include-path-for, jpeg) \
    $(TARGET_OUT_HEADERS)/libva \
    $(TARGET_OUT_HEADERS)/libjpegdec \

LOCAL_COPY_HEADERS_TO  := libjpeg_hw

LOCAL_COPY_HEADERS := \
    JPEGDecoder_libjpeg_wrapper.h

LOCAL_SHARED_LIBRARIES += \
    libcutils \
    libutils \
    liblog  \
    libva \
    libva-android \
    libjpegdec \
    libhardware

LOCAL_LDLIBS += -lpthread
LOCAL_CFLAGS += -Wno-multichar -DLOG_TAG=\"ImageDecoder\"
LOCAL_CFLAGS += -DUSE_INTEL_JPEGDEC
LOCAL_CFLAGS += -DLOG_NDEBUG=0

LOCAL_MODULE:= libjpeg_hw
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

