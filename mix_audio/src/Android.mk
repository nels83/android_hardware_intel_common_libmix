LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

#MIXAUDIO_LOG_ENABLE := true

LOCAL_SRC_FILES :=	\
	mixaip.c	\
	mixacp.c	\
	mixacpmp3.c	\
	mixacpwma.c	\
	mixacpaac.c	\
	mixaudio.c	\
	sst_proxy.c

LOCAL_C_INCLUDES :=		\
	$(LOCAL_PATH)		\
	$(GLIB_TOP)		\
	$(GLIB_TOP)/android	\
	$(GLIB_TOP)/glib	\
	$(GLIB_TOP)/gobject	\
	$(TARGET_OUT_HEADERS)/libmixcommon

LOCAL_SHARED_LIBRARIES :=	\
	libglib-2.0		\
	libgobject-2.0		\
	libgthread-2.0		\
	libgmodule-2.0		\
	libmixcommon

ifeq ($(strip $(MIXAUDIO_LOG_ENABLE)),true)
LOCAL_CFLAGS += -DMIX_LOG_ENABLE
LOCAL_SHARED_LIBRARIES += liblog
endif

LOCAL_COPY_HEADERS_TO := libmixaudio

LOCAL_COPY_HEADERS :=		\
	amhelper.h		\
	intel_sst_ioctl.h	\
	mixacp.h		\
	mixacpaac.h		\
	mixacpmp3.h		\
	mixacpwma.h		\
	mixaip.h		\
	mixaudio.h		\
	mixaudiotypes.h		\
	sst_proxy.h
#	pvt.h			\

LOCAL_MODULE := libmixaudio
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
