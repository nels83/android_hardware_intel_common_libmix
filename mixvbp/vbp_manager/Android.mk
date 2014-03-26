LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

ifeq (true,$(strip $(PRODUCT_PACKAGE_DEBUG)))
MIXVBP_LOG_ENABLE := true
endif

LOCAL_SRC_FILES :=                  \
    vbp_h264_parser.c               \
    vbp_vc1_parser.c                \
    vbp_loader.c                    \
    vbp_mp42_parser.c               \
    vbp_utils.c                     \
    viddec_parse_sc.c               \
    viddec_pm_parser_ops.c          \
    viddec_pm_utils_bstream.c       \
    vbp_thread.c

LOCAL_CFLAGS := -DVBP -DHOST_ONLY
LOCAL_CFLAGS += -DUSE_MULTI_THREADING

LOCAL_C_INCLUDES +=			\
	$(LOCAL_PATH)/include		\
	$(MIXVBP_DIR)/include		      \
	$(MIXVBP_DIR)/vbp_plugin/h264/include \
	$(MIXVBP_DIR)/vbp_plugin/mp2/include  \
	$(MIXVBP_DIR)/vbp_plugin/mp4/include  \
	$(MIXVBP_DIR)/vbp_plugin/vc1/include  \
	$(MIXVBP_DIR)/vbp_plugin/vc1/         \
	$(MIXVBP_DIR)/vbp_plugin/mp4/         \
	$(TARGET_OUT_HEADERS)/libva

LOCAL_COPY_HEADERS_TO := libmixvbp

LOCAL_COPY_HEADERS :=	\
	vbp_loader.h

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libmixvbp

LOCAL_SHARED_LIBRARIES :=		\
	libdl				\
	libcutils

ifeq ($(strip $(MIXVBP_LOG_ENABLE)),true)
LOCAL_CFLAGS += -DVBP_TRACE
LOCAL_SHARED_LIBRARIES += liblog
endif

ifeq ($(USE_HW_VP8),true)
LOCAL_SRC_FILES += vbp_vp8_parser.c
LOCAL_C_INCLUDES += $(MIXVBP_DIR)/vbp_plugin/vp8/include
LOCAL_CFLAGS += -DUSE_HW_VP8
endif

PLATFORM_SUPPORT_AVC_SHORT_FORMAT := \
    baytrail \
    cherrytrail

ifneq ($(filter $(TARGET_BOARD_PLATFORM),$(PLATFORM_SUPPORT_AVC_SHORT_FORMAT)),)
LOCAL_CFLAGS += -DUSE_AVC_SHORT_FORMAT
LOCAL_C_INCLUDES += $(LOCAL_PATH)/secvideo/baytrail/
LOCAL_SRC_FILES += secvideo/baytrail/vbp_h264secure_parser.c
endif

PLATFORM_SUPPORT_USE_SLICE_HEADER_PARSING := merrifield moorefield

ifneq ($(filter $(TARGET_BOARD_PLATFORM),$(PLATFORM_SUPPORT_USE_SLICE_HEADER_PARSING)),)
LOCAL_CFLAGS += -DUSE_SLICE_HEADER_PARSING
LOCAL_C_INCLUDES += $(LOCAL_PATH)/secvideo/merrifield/
LOCAL_SRC_FILES += secvideo/merrifield/vbp_h264secure_parser.c
endif

include $(BUILD_SHARED_LIBRARY)
