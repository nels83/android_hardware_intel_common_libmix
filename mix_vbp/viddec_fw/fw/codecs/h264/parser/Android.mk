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
	$(GLIB_TOP)							   \
	$(GLIB_TOP)/android						   \
	$(GLIB_TOP)/glib						   \
	$(VENDORS_INTEL_MRST_MIXVBP_ROOT)/viddec_fw/include		   \
	$(VENDORS_INTEL_MRST_MIXVBP_ROOT)/viddec_fw/fw/include		   \
	$(VENDORS_INTEL_MRST_MIXVBP_ROOT)/viddec_fw/fw/parser/include	   \
	$(VENDORS_INTEL_MRST_MIXVBP_ROOT)/viddec_fw/fw/codecs/h264/include

LOCAL_MODULE := libmixvbp_h264
LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES :=		\
	libglib-2.0			\
	libmixvbp

include $(BUILD_SHARED_LIBRARY)
