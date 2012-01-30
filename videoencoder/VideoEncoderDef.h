/*
 INTEL CONFIDENTIAL
 Copyright 2011 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

#ifndef __VIDEO_ENCODER_DEF_H__
#define __VIDEO_ENCODER_DEF_H__

#include <stdint.h>

#define STRING_TO_FOURCC(format) ((uint32_t)(((format)[0])|((format)[1]<<8)|((format)[2]<<16)|((format)[3]<<24)))
#define min(X,Y) (((X) < (Y)) ? (X) : (Y))
#define max(X,Y) (((X) > (Y)) ? (X) : (Y))

typedef int32_t Encode_Status;

// Video encode error code
enum {
    ENCODE_NO_REQUEST_DATA = -10,
    ENCODE_WRONG_STATE = -9,
    ENCODE_NOTIMPL = -8,
    ENCODE_NO_MEMORY = -7,
    ENCODE_NOT_INIT = -6,
    ENCODE_DRIVER_FAIL = -5,
    ENCODE_INVALID_PARAMS = -4,
    ENCODE_NOT_SUPPORTED = -3,
    ENCODE_NULL_PTR = -2,
    ENCODE_FAIL = -1,
    ENCODE_SUCCESS = 0,
    ENCODE_ALREADY_INIT = 1,
    ENCODE_SLICESIZE_OVERFLOW = 2,
    ENCODE_BUFFER_TOO_SMALL = 3 // The buffer passed to encode is too small to contain encoded data
};

typedef enum {
    OUTPUT_EVERYTHING = 0,  //Output whatever driver generates
    OUTPUT_CODEC_DATA = 1,
    OUTPUT_FRAME_DATA = 2, //Equal to OUTPUT_EVERYTHING when no header along with the frame data
    OUTPUT_ONE_NAL = 4,
    OUTPUT_ONE_NAL_WITHOUT_STARTCODE = 8,
    OUTPUT_LENGTH_PREFIXED = 16,
    OUTPUT_BUFFER_LAST
} VideoOutputFormat;

typedef enum {
    RAW_FORMAT_NONE = 0,
    RAW_FORMAT_YUV420 = 1,
    RAW_FORMAT_YUV422 = 2,
    RAW_FORMAT_YUV444 = 4,
    RAW_FORMAT_NV12 = 8,
    RAW_FORMAT_PROTECTED = 0x80000000,
    RAW_FORMAT_LAST
} VideoRawFormat;

typedef enum {
    RATE_CONTROL_NONE = 1,
    RATE_CONTROL_CBR = 2,
    RATE_CONTROL_VBR = 4,
    RATE_CONTROL_VCM = 8,
    RATE_CONTROL_LAST
} VideoRateControl;

typedef enum {
    PROFILE_MPEG2SIMPLE = 0,
    PROFILE_MPEG2MAIN,
    PROFILE_MPEG4SIMPLE,
    PROFILE_MPEG4ADVANCEDSIMPLE,
    PROFILE_MPEG4MAIN,
    PROFILE_H264BASELINE,
    PROFILE_H264MAIN,
    PROFILE_H264HIGH,
    PROFILE_VC1SIMPLE,
    PROFILE_VC1MAIN,
    PROFILE_VC1ADVANCED,
    PROFILE_H263BASELINE
} VideoProfile;

typedef enum {
    AVC_DELIMITER_LENGTHPREFIX = 0,
    AVC_DELIMITER_ANNEXB
} AVCDelimiterType;

typedef enum {
    VIDEO_ENC_NONIR,       // Non intra refresh
    VIDEO_ENC_CIR, 		// Cyclic intra refresh
    VIDEO_ENC_AIR, 		// Adaptive intra refresh
    VIDEO_ENC_BOTH,
    VIDEO_ENC_LAST
} VideoIntraRefreshType;

enum VideoBufferSharingMode {
    BUFFER_SHARING_NONE = 1, //Means non shared buffer mode
    BUFFER_SHARING_CI = 2,
    BUFFER_SHARING_V4L2 = 4,
    BUFFER_SHARING_SURFACE = 8,
    BUFFER_SHARING_USRPTR = 16,
    BUFFER_SHARING_GFXHANDLE = 32,
    BUFFER_LAST
};

// Output buffer flag
#define ENCODE_BUFFERFLAG_ENDOFFRAME       0x00000001
#define ENCODE_BUFFERFLAG_PARTIALFRAME     0x00000002
#define ENCODE_BUFFERFLAG_SYNCFRAME        0x00000004
#define ENCODE_BUFFERFLAG_CODECCONFIG      0x00000008
#define ENCODE_BUFFERFLAG_DATACORRUPT      0x00000010
#define ENCODE_BUFFERFLAG_DATAINVALID      0x00000020
#define ENCODE_BUFFERFLAG_SLICEOVERFOLOW   0x00000040

typedef struct {
    uint8_t *data;
    uint32_t bufferSize; //buffer size
    uint32_t dataSize; //actuall size
    uint32_t remainingSize;
    int flag; //Key frame, Codec Data etc
    VideoOutputFormat format; //output format
    uint64_t timeStamp; //reserved
} VideoEncOutputBuffer;

typedef struct {
    uint8_t *data;
    uint32_t size;
    bool bufAvailable; //To indicate whether this buffer can be reused
    uint64_t timeStamp; //reserved
} VideoEncRawBuffer;

struct VideoEncSurfaceBuffer {
    VASurfaceID surface;
    uint8_t *usrptr;
    uint32_t index;
    bool bufAvailable;
    VideoEncSurfaceBuffer *next;
};

struct AirParams {
    uint32_t airMBs;
    uint32_t airThreshold;
    uint32_t airAuto;

    AirParams &operator=(const AirParams &other) {
        if (this == &other) return *this;

        this->airMBs= other.airMBs;
        this->airThreshold= other.airThreshold;
        this->airAuto = other.airAuto;
        return *this;
    }
};

struct VideoFrameRate {
    uint32_t frameRateNum;
    uint32_t frameRateDenom;

    VideoFrameRate &operator=(const VideoFrameRate &other) {
        if (this == &other) return *this;

        this->frameRateNum = other.frameRateNum;
        this->frameRateDenom = other.frameRateDenom;
        return *this;
    }
};

struct VideoResolution {
    uint32_t width;
    uint32_t height;

    VideoResolution &operator=(const VideoResolution &other) {
        if (this == &other) return *this;

        this->width = other.width;
        this->height = other.height;
        return *this;
    }
};

struct VideoRateControlParams {
    uint32_t bitRate;
    uint32_t initQP;
    uint32_t minQP;
    uint32_t windowSize;
    uint32_t targetPercentage;
    uint32_t disableFrameSkip;
    uint32_t disableBitsStuffing;

    VideoRateControlParams &operator=(const VideoRateControlParams &other) {
        if (this == &other) return *this;

        this->bitRate = other.bitRate;
        this->initQP = other.initQP;
        this->minQP = other.minQP;
        this->windowSize = other.windowSize;
        this->targetPercentage = other.targetPercentage;
        this->disableFrameSkip = other.disableFrameSkip;
        this->disableBitsStuffing = other.disableBitsStuffing;
        return *this;
    }
};

struct SliceNum {
    uint32_t iSliceNum;
    uint32_t pSliceNum;

    SliceNum &operator=(const SliceNum &other) {
        if (this == &other) return *this;

        this->iSliceNum = other.iSliceNum;
        this->pSliceNum= other.pSliceNum;
        return *this;
    }
};

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t lumaStride;
    uint32_t chromStride;
    uint32_t format;
} ExternalBufferAttrib;

enum VideoParamConfigType {
    VideoParamsTypeStartUnused = 0x01000000,
    VideoParamsTypeCommon,
    VideoParamsTypeAVC,
    VideoParamsTypeH263,
    VideoParamsTypeMP4,
    VideoParamsTypeVC1,
    VideoParamsTypeUpSteamBuffer,
    VideoParamsTypeUsrptrBuffer,
    VideoParamsTypeHRD,

    VideoConfigTypeFrameRate,
    VideoConfigTypeBitRate,
    VideoConfigTypeResolution,
    VideoConfigTypeIntraRefreshType,
    VideoConfigTypeAIR,
    VideoConfigTypeCyclicFrameInterval,
    VideoConfigTypeAVCIntraPeriod,
    VideoConfigTypeNALSize,
    VideoConfigTypeIDRRequest,
    VideoConfigTypeSliceNum,

    VideoParamsConfigExtension
};

struct VideoParamConfigSet {
    VideoParamConfigType type;
    uint32_t size;

    VideoParamConfigSet &operator=(const VideoParamConfigSet &other) {
        if (this == &other) return *this;
        this->type = other.type;
        this->size = other.size;
        return *this;
    }
};

struct VideoParamsCommon : VideoParamConfigSet {

    VAProfile profile;
    uint8_t level;
    VideoRawFormat rawFormat;
    VideoResolution resolution;
    VideoFrameRate frameRate;
    int32_t intraPeriod;
    VideoRateControl rcMode;
    VideoRateControlParams rcParams;
    VideoIntraRefreshType refreshType;
    int32_t cyclicFrameInterval;
    AirParams airParams;
    uint32_t disableDeblocking;
    bool syncEncMode;

    VideoParamsCommon() {
        type = VideoParamsTypeCommon;
        size = sizeof(VideoParamsCommon);
    }

    VideoParamsCommon &operator=(const VideoParamsCommon &other) {
        if (this == &other) return *this;

        VideoParamConfigSet::operator=(other);
        this->profile = other.profile;
        this->level = other.level;
        this->rawFormat = other.rawFormat;
        this->resolution = other.resolution;
        this->frameRate = other.frameRate;
        this->intraPeriod = other.intraPeriod;
        this->rcMode = other.rcMode;
        this->rcParams = other.rcParams;
        this->refreshType = other.refreshType;
        this->cyclicFrameInterval = other.cyclicFrameInterval;
        this->airParams = other.airParams;
        this->disableDeblocking = other.disableDeblocking;
        this->syncEncMode = other.syncEncMode;
        return *this;
    }
};

struct VideoParamsAVC : VideoParamConfigSet {
    uint32_t basicUnitSize;  //for rate control
    uint8_t VUIFlag;
    int32_t maxSliceSize;
    uint32_t idrInterval;
    SliceNum sliceNum;
    AVCDelimiterType delimiterType;

    VideoParamsAVC() {
        type = VideoParamsTypeAVC;
        size = sizeof(VideoParamsAVC);
    }

    VideoParamsAVC &operator=(const VideoParamsAVC &other) {
        if (this == &other) return *this;

        VideoParamConfigSet::operator=(other);
        this->basicUnitSize = other.basicUnitSize;
        this->VUIFlag = other.VUIFlag;
        this->maxSliceSize = other.maxSliceSize;
        this->idrInterval = other.idrInterval;
        this->sliceNum = other.sliceNum;
        this->delimiterType = other.delimiterType;

        return *this;
    }
};

struct VideoParamsUpstreamBuffer : VideoParamConfigSet {

    VideoParamsUpstreamBuffer() {
        type = VideoParamsTypeUpSteamBuffer;
        size = sizeof(VideoParamsUpstreamBuffer);
    }

    VideoBufferSharingMode bufferMode;
    uint32_t *bufList;
    uint32_t bufCnt;
    ExternalBufferAttrib *bufAttrib;
    void *display;
};

struct VideoParamsUsrptrBuffer : VideoParamConfigSet {

    VideoParamsUsrptrBuffer() {
        type = VideoParamsTypeUsrptrBuffer;
        size = sizeof(VideoParamsUsrptrBuffer);
    }

    //input
    uint32_t width;
    uint32_t height;
    uint32_t format;
    uint32_t expectedSize;

    //output
    uint32_t actualSize;
    uint32_t stride;
    uint8_t *usrPtr;
};

struct VideoParamsHRD : VideoParamConfigSet {

    VideoParamsHRD() {
        type = VideoParamsTypeHRD;
        size = sizeof(VideoParamsHRD);
    }

    uint32_t bufferSize;
    uint32_t initBufferFullness;
};

struct VideoConfigFrameRate : VideoParamConfigSet {

    VideoConfigFrameRate() {
        type = VideoConfigTypeFrameRate;
        size = sizeof(VideoConfigFrameRate);
    }

    VideoFrameRate frameRate;
};

struct VideoConfigBitRate : VideoParamConfigSet {

    VideoConfigBitRate() {
        type = VideoConfigTypeBitRate;
        size = sizeof(VideoConfigBitRate);
    }

    VideoRateControlParams rcParams;
};

struct VideoConfigAVCIntraPeriod : VideoParamConfigSet {

    VideoConfigAVCIntraPeriod() {
        type = VideoConfigTypeAVCIntraPeriod;
        size = sizeof(VideoConfigAVCIntraPeriod);
    }

    uint32_t idrInterval;  //How many Intra frame will have a IDR frame
    uint32_t intraPeriod;
};

struct VideoConfigNALSize : VideoParamConfigSet {

    VideoConfigNALSize() {
        type = VideoConfigTypeNALSize;
        size = sizeof(VideoConfigNALSize);
    }

    uint32_t maxSliceSize;
};

struct VideoConfigResoltuion : VideoParamConfigSet {

    VideoConfigResoltuion() {
        type = VideoConfigTypeResolution;
        size = sizeof(VideoConfigResoltuion);
    }

    VideoResolution resolution;
};

struct VideoConfigIntraRefreshType : VideoParamConfigSet {

    VideoConfigIntraRefreshType() {
        type = VideoConfigTypeIntraRefreshType;
        size = sizeof(VideoConfigIntraRefreshType);
    }

    VideoIntraRefreshType refreshType;
};

struct VideoConfigCyclicFrameInterval : VideoParamConfigSet {

    VideoConfigCyclicFrameInterval() {
        type = VideoConfigTypeCyclicFrameInterval;
        size = sizeof(VideoConfigCyclicFrameInterval);
    }

    int32_t cyclicFrameInterval;
};

struct VideoConfigAIR : VideoParamConfigSet {

    VideoConfigAIR() {
        type = VideoConfigTypeAIR;
        size = sizeof(VideoConfigAIR);
    }

    AirParams airParams;
};

struct VideoConfigSliceNum : VideoParamConfigSet {

    VideoConfigSliceNum() {
        type = VideoConfigTypeSliceNum;
        size = sizeof(VideoConfigSliceNum);
    }

    SliceNum sliceNum;
};
#endif /*  __VIDEO_ENCODER_DEF_H__ */
