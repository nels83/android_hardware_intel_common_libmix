LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    viddec_vc1_parse.c \
    vc1parse_bpic_adv.c \
    vc1parse_huffman.c \
    vc1parse_mv_com.c \
    vc1parse_ppic_adv.c \
    vc1parse_bpic.c \
    vc1parse_common_tables.c \
    vc1parse_ipic_adv.c \
    vc1parse_pic_com_adv.c \
    vc1parse_ppic.c \
    vc1parse_bitplane.c \
    vc1parse.c \
    vc1parse_ipic.c \
    vc1parse_pic_com.c \
    vc1parse_vopdq.c

LOCAL_C_INCLUDES := \
    $(MIXVBP_DIR)/include   \
    $(MIXVBP_DIR)/vbp_manager/include   \
    $(MIXVBP_DIR)/vbp_plugin/vc1/include

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libmixvbp_vc1

LOCAL_SHARED_LIBRARIES := \
    libmixvbp \
    liblog

include $(BUILD_SHARED_LIBRARY)
