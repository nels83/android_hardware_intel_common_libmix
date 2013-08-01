LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    VideoVPPBase.cpp


LOCAL_C_INCLUDES += \
    $(TARGET_OUT_HEADERS)/libwsbm       \
    $(TARGET_OUT_HEADERS)/libpsb_drm \
    $(TARGET_OUT_HEADERS)/libva \

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    libui \
    liblog \
    libhardware \
    libdrm \
    libdrm_intel \
    libwsbm \
    libva \
    libva-android \
    libva-tpi

LOCAL_COPY_HEADERS_TO  := libmix_videovpp

LOCAL_COPY_HEADERS := \
    VideoVPPBase.h

LOCAL_MODULE := libmix_videovpp

LOCAL_MODULE_TAGS := eng

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    test/main.cpp

LOCAL_C_INCLUDES += \
    $(TARGET_OUT_HEADERS)/libva \
    $(TARGET_OUT_HEADERS)/libmix_videovpp

LOCAL_SHARED_LIBRARIES := \
    libhardware \
    libmix_videovpp

LOCAL_MODULE := csc_vpp

LOCAL_MODULE_TAGS := eng


include $(BUILD_EXECUTABLE)
