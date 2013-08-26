LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
        vp8parse.c \
        bool_coder.c \
        viddec_vp8_parse.c

LOCAL_CFLAGS := -DVBP -DHOST_ONLY

LOCAL_C_INCLUDES := \
        $(MIXVBP_DIR)/include \
        $(MIXVBP_DIR)/vbp_manager/include \
        $(LOCAL_PATH)/include

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libmixvbp_vp8

LOCAL_SHARED_LIBRARIES := \
        libmixvbp \
        liblog

include $(BUILD_SHARED_LIBRARY)
