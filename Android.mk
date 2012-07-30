LOCAL_PATH := $(call my-dir)

ifeq ($(INTEL_VA),true)

include $(CLEAR_VARS)
VENDORS_INTEL_MRST_LIBMIX_ROOT := $(LOCAL_PATH)

include $(VENDORS_INTEL_MRST_LIBMIX_ROOT)/mix_common/src/Android.mk
#include $(VENDORS_INTEL_MRST_LIBMIX_ROOT)/mix_audio/src/Android.mk
include $(VENDORS_INTEL_MRST_LIBMIX_ROOT)/mix_video/src/Android.mk
include $(VENDORS_INTEL_MRST_LIBMIX_ROOT)/mix_vbp/Android.mk
include $(VENDORS_INTEL_MRST_LIBMIX_ROOT)/asfparser/Android.mk
include $(VENDORS_INTEL_MRST_LIBMIX_ROOT)/videodecoder/Android.mk
include $(VENDORS_INTEL_MRST_LIBMIX_ROOT)/videoencoder/Android.mk
include $(VENDORS_INTEL_MRST_LIBMIX_ROOT)/frameworks/asf_extractor/Android.mk
include $(VENDORS_INTEL_MRST_LIBMIX_ROOT)/frameworks/vavideodecoder/Android.mk
include $(VENDORS_INTEL_MRST_LIBMIX_ROOT)/frameworks/libI420colorconvert/Android.mk
include $(VENDORS_INTEL_MRST_LIBMIX_ROOT)/frameworks/videoedit/Android.mk
endif
