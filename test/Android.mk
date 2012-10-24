LOCAL_PATH := $(call my-dir)

# For intelmetadatabuffer test
# =====================================================

include $(CLEAR_VARS)

#VIDEO_ENC_LOG_ENABLE := true

LOCAL_SRC_FILES :=              \
    btest.cpp

LOCAL_C_INCLUDES :=             \
    $(LOCAL_PATH)               \
    $(TARGET_OUT_HEADERS)/libmix_videoencoder \

LOCAL_SHARED_LIBRARIES :=       \
        libintelmetadatabuffer  

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := btest 

include $(BUILD_EXECUTABLE)

# For mix_encoder
# =====================================================

include $(CLEAR_VARS)

#VIDEO_ENC_LOG_ENABLE := true

LOCAL_SRC_FILES :=              \
    mix_encoder.cpp

LOCAL_C_INCLUDES :=             \
    $(TARGET_OUT_HEADERS)/libva \
    $(TARGET_OUT_HEADERS)/libmix_videoencoder \
    $(TOP)/frameworks/base/include/display \
    $(LOCAL_PATH)

LOCAL_SHARED_LIBRARIES := 	\
	libintelmetadatabuffer	\
	libva_videoencoder	\
        libva                   \
        libva-android           \
        libva-tpi		\
	libgui			\
	libui			\
	libutils		\
	libcutils		\
	libhardware		\
	libbinder

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := mix_encoder

include $(BUILD_EXECUTABLE)
