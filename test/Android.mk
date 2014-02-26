LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES :=  mix_decoder.cpp

LOCAL_C_INCLUDES :=             \
    $(TARGET_OUT_HEADERS)/libva_videodecoder \
    $(TARGET_OUT_HEADERS)/libva \
    $(TARGET_OUT_HEADERS)/libmixvbp \
    $(TARGET_OUT_HEADERS)/libmix_videodecoder

LOCAL_SHARED_LIBRARIES :=       \
        libva_videodecoder liblog libva

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := mix_decoder

include $(BUILD_EXECUTABLE)

# For intelmetadatabuffer test
# =====================================================

include $(CLEAR_VARS)

#VIDEO_ENC_LOG_ENABLE := true

LOCAL_SRC_FILES :=              \
    btest.cpp

LOCAL_C_INCLUDES :=             \
    $(TARGET_OUT_HEADERS)/libmix_videoencoder \

LOCAL_SHARED_LIBRARIES :=       \
        libintelmetadatabuffer  

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := btest 

include $(BUILD_EXECUTABLE)

# For intelmetadatabuffer cross-process buffersharing test
# =====================================================

include $(CLEAR_VARS)

#VIDEO_ENC_LOG_ENABLE := true

LOCAL_SRC_FILES :=              \
    BSServer.cpp

LOCAL_C_INCLUDES :=             \
    $(TARGET_OUT_HEADERS)/libmix_videoencoder \

LOCAL_SHARED_LIBRARIES :=       \
        libintelmetadatabuffer libutils libbinder

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := bserver

include $(BUILD_EXECUTABLE)

# For intelmetadatabuffer cross-process buffersharing test
# =====================================================

include $(CLEAR_VARS)

#VIDEO_ENC_LOG_ENABLE := true

LOCAL_SRC_FILES :=              \
    BSClient.cpp

LOCAL_C_INCLUDES :=             \
    $(TARGET_OUT_HEADERS)/libmix_videoencoder \

LOCAL_SHARED_LIBRARIES :=       \
        libintelmetadatabuffer libutils libbinder \
        libgui                  \
        libui                   \
        libutils                \
        libcutils               \
        libhardware             \

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := bclient

include $(BUILD_EXECUTABLE)

# For mix_encoder
# =====================================================

include $(CLEAR_VARS)

#VIDEO_ENC_LOG_ENABLE := true

LOCAL_SRC_FILES :=              \
    mix_encoder.cpp

LOCAL_C_INCLUDES :=             \
    $(TARGET_OUT_HEADERS)/libva \
    $(TARGET_OUT_HEADERS)/libmix_videoencoder \
    $(TARGET_OUT_HEADERS)/pvr \

LOCAL_SHARED_LIBRARIES := 	\
	libintelmetadatabuffer	\
	libva_videoencoder	\
        libva                   \
        libva-android           \
        libva-tpi		\
	libgui			\
	libui			\
	libutils		\
	libcutils		\
	libhardware		\
	libbinder

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := mix_encoder

include $(BUILD_EXECUTABLE)
# For mix_encoder2
# =====================================================

include $(CLEAR_VARS)

#VIDEO_ENC_LOG_ENABLE := true

LOCAL_SRC_FILES :=              \
    mix_encoder2.cpp

LOCAL_C_INCLUDES :=             \
    $(TARGET_OUT_HEADERS)/libva \
    $(TARGET_OUT_HEADERS)/libmix_videoencoder \
    $(TARGET_OUT_HEADERS)/pvr \
    $(call include-path-for, libstagefright) \
    $(call include-path-for, frameworks-openmax) \

LOCAL_SHARED_LIBRARIES :=       \
        libintelmetadatabuffer  \
        libva_videoencoder      \
        libva                   \
        libva-android           \
        libva-tpi               \
        libgui                  \
        libui                   \
        libutils                \
        libcutils               \
        libhardware             \
        libbinder		\
	libstagefright		\
	liblog			\
	libstagefright_foundation

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := mix_encoder2

include $(BUILD_EXECUTABLE)

# For gfx_test
# =====================================================

include $(CLEAR_VARS)

#VIDEO_ENC_LOG_ENABLE := true

LOCAL_SRC_FILES :=              \
    gfx_test.cpp

LOCAL_C_INCLUDES :=             \
    $(TARGET_OUT_HEADERS)/pvr \
    $(call include-path-for, libstagefright) \

LOCAL_SHARED_LIBRARIES :=       \
        libgui                  \
        libui                   \
        libutils                \
        libcutils               \
        libhardware             \
        libbinder               \
        libstagefright          \
        libstagefright_foundation

ifeq ($(ENABLE_MRFL_GRAPHICS),true)
    LOCAL_CFLAGS += -DMRFLD_GFX
endif

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := gfx_test

include $(BUILD_EXECUTABLE)
