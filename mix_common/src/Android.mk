LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES :=		\
	mixlog.c		\
	mixparams.c		\
	mixdrmparams.c		\

LOCAL_C_INCLUDES :=		\
	$(LOCAL_PATH)		\
	$(GLIB_TOP)		\
	$(GLIB_TOP)/android	\
	$(GLIB_TOP)/glib 	\
	$(GLIB_TOP)/gobject

LOCAL_SHARED_LIBRARIES := 	\
	libglib-2.0		\
	libgobject-2.0		\
	libgthread-2.0		\
	libgmodule-2.0

LOCAL_COPY_HEADERS_TO := libmixcommon

LOCAL_COPY_HEADERS :=		\
	mixlog.h		\
	mixresult.h		\
	mixparams.h		\
	mixdrmparams.h

LOCAL_MODULE := libmixcommon
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
