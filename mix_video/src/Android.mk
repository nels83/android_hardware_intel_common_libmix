LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

MIXVIDEO_LOG_ENABLE := false

LOCAL_SRC_FILES := 			\
	mixvideothread.cpp			\
	mixbuffer.cpp			\
	mixbufferpool.cpp			\
	mixdisplay.cpp			\
	mixdisplayandroid.cpp		\
	mixframemanager.cpp		\
	mixsurfacepool.cpp		\
	mixvideocaps.cpp			\
	mixvideoconfigparams.cpp		\
	mixvideoconfigparamsdec.cpp	\
	mixvideoconfigparamsdec_h264.cpp	\
	mixvideoconfigparamsdec_mp42.cpp	\
	mixvideoconfigparamsdec_vc1.cpp	\
	mixvideoconfigparamsenc.cpp	\
	mixvideoconfigparamsenc_mpeg4.cpp	\
	mixvideoconfigparamsenc_h264.cpp	\
	mixvideoconfigparamsenc_h263.cpp	\
	mixvideoconfigparamsenc_preview.cpp \
	mixvideodecodeparams.cpp		\
	mixvideoencodeparams.cpp		\
	mixvideoformat.cpp		\
	mixvideoformat_h264.cpp		\
	mixvideoformat_mp42.cpp		\
	mixvideoformat_vc1.cpp		\
	mixvideoformatenc.cpp		\
	mixvideoformatenc_h264.cpp	\
	mixvideoformatenc_h263.cpp	\
	mixvideoformatenc_mpeg4.cpp	\
	mixvideoformatenc_preview.cpp	\
	mixvideoframe.cpp			\
	mixvideoinitparams.cpp		\
	mixvideorenderparams.cpp	\
	mixvideo.cpp

LOCAL_CFLAGS :=			\
	-DMIXVIDEO_AGE=1	\
	-DMIXVIDEO_CURRENT=1	\
	-DMIXVIDEO_MAJOR=0	\
	-DMIXVIDEO_MINOR=1	\
	-DMIXVIDEO_REVISION=8

LOCAL_C_INCLUDES :=				\
	$(LOCAL_PATH)				\
	$(TARGET_OUT_HEADERS)/libmixcommon	\
	$(TARGET_OUT_HEADERS)/libmixvbp		\
	$(TARGET_OUT_HEADERS)/libva

LOCAL_LDLIBS += -lpthread

LOCAL_SHARED_LIBRARIES :=	\
	libcutils			\
	libmixcommon		\
	libmixvbp			\
	libva				\
	libva-android		\
	libva-tpi


LOCAL_CFLAGS += -DANDROID	\
				-DMIXVIDEO_ENCODE_ENABLE=0

ifeq ($(strip $(MIXVIDEO_LOG_ENABLE)),true)
LOCAL_CFLAGS += -DMIX_LOG_ENABLE
LOCAL_SHARED_LIBRARIES += liblog
endif

LOCAL_COPY_HEADERS_TO := libmixvideo

LOCAL_COPY_HEADERS :=			\
	mixvideothread.h			\
	mixbuffer.h			\
	mixbuffer_private.h		\
	mixbufferpool.h			\
	mixdisplay.h			\
	mixdisplayandroid.h		\
	mixframemanager.h		\
	mixsurfacepool.h		\
	mixvideo.h			\
	mixvideodef.h			\
	mixvideo_private.h		\
	mixvideocaps.h			\
	mixvideoconfigparams.h		\
	mixvideoconfigparamsdec.h	\
	mixvideoconfigparamsdec_h264.h	\
	mixvideoconfigparamsdec_mp42.h	\
	mixvideoconfigparamsdec_vc1.h	\
	mixvideoconfigparamsenc.h	\
	mixvideoconfigparamsenc_h264.h	\
	mixvideoconfigparamsenc_h263.h  \
	mixvideoconfigparamsenc_mpeg4.h	\
	mixvideoconfigparamsenc_preview.h \
	mixvideodecodeparams.h		\
	mixvideoencodeparams.h		\
	mixvideoformat.h		\
	mixvideoformat_h264.h		\
	mixvideoformat_mp42.h		\
	mixvideoformat_vc1.h		\
	mixvideoformatenc.h		\
	mixvideoformatenc_h264.h	\
	mixvideoformatenc_h263.h        \
	mixvideoformatenc_mpeg4.h	\
	mixvideoformatenc_preview.h	\
	mixvideoformatqueue.h		\
	mixvideoframe.h			\
	mixvideoframe_private.h		\
	mixvideoinitparams.h		\
	mixvideorenderparams.h		\
	mixvideorenderparams_internal.h

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libmixvideo

include $(BUILD_SHARED_LIBRARY)
