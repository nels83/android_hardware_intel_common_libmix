LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

#MIXVIDEO_LOG_ENABLE := true

LOCAL_SRC_FILES := 			\
	mixbuffer.c			\
	mixbufferpool.c			\
	mixdisplay.c			\
	mixdisplayandroid.c		\
	mixframemanager.c		\
	mixsurfacepool.c		\
	mixvideo.c			\
	mixvideocaps.c			\
	mixvideoconfigparams.c		\
	mixvideoconfigparamsdec.c	\
	mixvideoconfigparamsdec_h264.c	\
	mixvideoconfigparamsdec_mp42.c	\
	mixvideoconfigparamsdec_vc1.c	\
	mixvideoconfigparamsenc.c	\
	mixvideoconfigparamsenc_h264.c	\
	mixvideoconfigparamsenc_h263.c	\
	mixvideoconfigparamsenc_mpeg4.c	\
	mixvideoconfigparamsenc_preview.c \
	mixvideodecodeparams.c		\
	mixvideoencodeparams.c		\
	mixvideoformat.c		\
	mixvideoformat_h264.c		\
	mixvideoformat_mp42.c		\
	mixvideoformat_vc1.c		\
	mixvideoformatenc.c		\
	mixvideoformatenc_h264.c	\
	mixvideoformatenc_h263.c	\
	mixvideoformatenc_mpeg4.c	\
	mixvideoformatenc_preview.c	\
	mixvideoframe.c			\
	mixvideoinitparams.c		\
	mixvideorenderparams.c

LOCAL_CFLAGS :=			\
	-DMIXVIDEO_AGE=1	\
	-DMIXVIDEO_CURRENT=1	\
	-DMIXVIDEO_MAJOR=0	\
	-DMIXVIDEO_MINOR=1	\
	-DMIXVIDEO_REVISION=8

LOCAL_C_INCLUDES :=				\
	$(LOCAL_PATH)				\
	$(GLIB_TOP)				\
	$(GLIB_TOP)/android			\
	$(GLIB_TOP)/glib			\
	$(GLIB_TOP)/gobject			\
	$(TARGET_OUT_HEADERS)/libmixcommon	\
	$(TARGET_OUT_HEADERS)/libmixvbp		\
	$(TARGET_OUT_HEADERS)/libva

LOCAL_SHARED_LIBRARIES :=	\
	libcutils		\
	libglib-2.0		\
	libgobject-2.0		\
	libgthread-2.0		\
	libgmodule-2.0		\
	libmixcommon		\
	libmixvbp		\
        libva                   \
        libva-android	        \
	libva-tpi

LOCAL_CFLAGS += -DANDROID
ifeq ($(strip $(MIXVIDEO_LOG_ENABLE)),true)
LOCAL_CFLAGS += -DMIX_LOG_ENABLE
LOCAL_SHARED_LIBRARIES += liblog
endif

LOCAL_COPY_HEADERS_TO := libmixvideo

LOCAL_COPY_HEADERS :=			\
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
	mixvideoformatenc_mpeg4.h	\
	mixvideoformatenc_preview.h	\
	mixvideoformatqueue.h		\
	mixvideoframe.h			\
	mixvideoframe_private.h		\
	mixvideoinitparams.h		\
	mixvideorenderparams.h		\
	mixvideorenderparams_internal.h

LOCAL_MODULE := libmixvideo

include $(BUILD_SHARED_LIBRARY)
