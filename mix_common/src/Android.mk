LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES :=		\
	mixlog.cpp		\
	mixparams.cpp		\
	mixdrmparams.cpp		\
	j_slist.cpp		\
	j_queue.cpp		\
	j_hashtable.cpp


LOCAL_C_INCLUDES :=		\
	$(LOCAL_PATH)

LOCAL_SHARED_LIBRARIES :=	\
	libcutils

LOCAL_CFLAGS := -DANDROID


LOCAL_COPY_HEADERS_TO := libmixcommon

LOCAL_COPY_HEADERS :=		\
	mixtypes.h		\
	j_slist.h		\
	j_queue.h		\
	j_hashtable.h	\
	mixlog.h		\
	mixresult.h		\
	mixparams.h		\
	mixdrmparams.h

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libmixcommon

include $(BUILD_SHARED_LIBRARY)
