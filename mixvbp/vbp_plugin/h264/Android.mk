LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES :=			\
	h264parse.c			\
	h264parse_bsd.c			\
	h264parse_math.c		\
	h264parse_mem.c			\
	h264parse_sei.c			\
	h264parse_sh.c			\
	h264parse_pps.c			\
	h264parse_sps.c			\
	h264parse_dpb.c			\
	viddec_h264_parse.c		\
	mix_vbp_h264_stubs.c

LOCAL_CFLAGS := -DVBP -DHOST_ONLY

LOCAL_C_INCLUDES :=							   \
	$(LOCAL_PATH)/include		   \
	$(MIXVBP_DIR)/include		   \
	$(MIXVBP_DIR)/vbp_manager/include	   \
	$(MIXVBP_DIR)/vbp_manager/h264/include

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libmixvbp_h264

LOCAL_SHARED_LIBRARIES :=		\
	libmixvbp

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
PLATFORM_SUPPORT_AVC_SHORT_FORMAT := baytrail

ifneq ($(filter $(TARGET_BOARD_PLATFORM),$(PLATFORM_SUPPORT_AVC_SHORT_FORMAT)),)
LOCAL_SRC_FILES := \
        h264parse.c \
        h264parse_bsd.c \
        h264parse_math.c \
        h264parse_mem.c \
        h264parse_sei.c \
        h264parse_pps.c \
        h264parse_sps.c \
        h264parse_dpb.c \
        h264parse_sh.c \
        secvideo/baytrail/viddec_h264secure_parse.c \
        mix_vbp_h264_stubs.c

LOCAL_CFLAGS := -DVBP -DHOST_ONLY -DUSE_AVC_SHORT_FORMAT

LOCAL_C_INCLUDES :=   \
	$(LOCAL_PATH)/include   \
	$(MIXVBP_DIR)/include    \
	$(MIXVBP_DIR)/vbp_manager/include   \
	$(MIXVBP_DIR)/vbp_manager/h264/include


LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libmixvbp_h264secure
LOCAL_SHARED_LIBRARIES := libmixvbp

include $(BUILD_SHARED_LIBRARY)

endif
