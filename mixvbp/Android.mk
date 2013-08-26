LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

MIXVBP_DIR := $(LOCAL_PATH)

include $(MIXVBP_DIR)/vbp_manager/Android.mk
include $(MIXVBP_DIR)/vbp_plugin/h264/Android.mk
include $(MIXVBP_DIR)/vbp_plugin/mp4/Android.mk
include $(MIXVBP_DIR)/vbp_plugin/vc1/Android.mk

ifeq ($(USE_HW_VP8),true)
include $(MIXVBP_DIR)/vbp_plugin/vp8/Android.mk
endif
