LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

VENDORS_INTEL_MRST_LIBMIX_ROOT := $(LOCAL_PATH)

GLIB_TOP := external/glib

include $(VENDORS_INTEL_MRST_LIBMIX_ROOT)/mix_common/src/Android.mk
include $(VENDORS_INTEL_MRST_LIBMIX_ROOT)/mix_audio/src/Android.mk
include $(VENDORS_INTEL_MRST_LIBMIX_ROOT)/mix_video/src/Android.mk
include $(VENDORS_INTEL_MRST_LIBMIX_ROOT)/mix_vbp/Android.mk
