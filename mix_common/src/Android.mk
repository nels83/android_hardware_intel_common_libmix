LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES :=		\
	mixlog.cpp		\
	mixparams.cpp		\
	mixdrmparams.cpp

LOCAL_C_INCLUDES :=		\
	$(LOCAL_PATH)		\
	$(GLIB_TOP)			\
	$(GLIB_TOP)/android	\
	$(GLIB_TOP)/glib

LOCAL_CFLAGS := -DANDROID

LOCAL_SHARED_LIBRARIES := 	\
	libglib-2.0
#	libgmodule-2.0

LOCAL_COPY_HEADERS_TO := libmixcommon

LOCAL_COPY_HEADERS :=		\
	mixlog.h		\
	mixresult.h		\
	mixparams.h		\
	mixdrmparams.h

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libmixcommon

include $(BUILD_SHARED_LIBRARY)
