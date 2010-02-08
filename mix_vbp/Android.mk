LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

VENDORS_INTEL_MRST_MIXVBP_ROOT := $(LOCAL_PATH)

include $(VENDORS_INTEL_MRST_MIXVBP_ROOT)/viddec_fw/fw/parser/Android.mk
include $(VENDORS_INTEL_MRST_MIXVBP_ROOT)/viddec_fw/fw/codecs/h264/parser/Android.mk
