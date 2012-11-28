LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

VENDORS_INTEL_MRST_MIXVBP_ROOT := $(LOCAL_PATH)

include $(VENDORS_INTEL_MRST_MIXVBP_ROOT)/viddec_fw/fw/parser/Android.mk
include $(VENDORS_INTEL_MRST_MIXVBP_ROOT)/viddec_fw/fw/codecs/h264/parser/Android.mk
include $(VENDORS_INTEL_MRST_MIXVBP_ROOT)/viddec_fw/fw/codecs/mp4/parser/Android.mk
include $(VENDORS_INTEL_MRST_MIXVBP_ROOT)/viddec_fw/fw/codecs/vc1/parser/Android.mk

# Add source codes for Merrifield
MERRIFIELD_DEVICE := \
        mrfl_vp \
        mrfl_hvp \
        mrfl_sle
ifneq ($(filter $(REF_DEVICE_NAME),$(MERRIFIELD_DEVICE)),)
include $(VENDORS_INTEL_MRST_MIXVBP_ROOT)/viddec_fw/fw/codecs/vp8/parser/Android.mk
endif
