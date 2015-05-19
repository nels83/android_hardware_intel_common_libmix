LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES :=                           \
    viddec_mp4_visualobject.c            \
    viddec_mp4_decodevideoobjectplane.c  \
    viddec_mp4_parse.c                   \
    viddec_mp4_videoobjectplane.c        \
    viddec_mp4_shortheader.c             \
    viddec_mp4_videoobjectlayer.c

LOCAL_CFLAGS := -DVBP -DHOST_ONLY

LOCAL_C_INCLUDES :=                          \
    $(MIXVBP_DIR)/include          \
    $(LOCAL_PATH)/include \
    $(MIXVBP_DIR)/vbp_manager/include

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libmixvbp_mpeg4

LOCAL_SHARED_LIBRARIES :=      \
    libmixvbp      \
    liblog

include $(BUILD_SHARED_LIBRARY)
