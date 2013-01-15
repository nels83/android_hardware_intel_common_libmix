/*
 INTEL CONFIDENTIAL
 Copyright 2011 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

#ifndef __VIDEO_ENCODER_BASE_H__
#define __VIDEO_ENCODER_BASE_H__

#include <va/va.h>
#include <va/va_tpi.h>
#include "VideoEncoderDef.h"
#include "VideoEncoderInterface.h"
#include "IntelMetadataBuffer.h"

struct SurfaceMap {
    VASurfaceID surface;
    MetadataBufferType type;
    int32_t value;
    ValueInfo vinfo;
    uint32_t index;
    bool added;
    SurfaceMap *next;
};

class VideoEncoderBase : IVideoEncoder {

public:
    VideoEncoderBase();
    virtual ~VideoEncoderBase();

    virtual Encode_Status start(void);
    virtual void flush(void);
    virtual Encode_Status stop(void);
    virtual Encode_Status encode(VideoEncRawBuffer *inBuffer);

    /*
    * getOutput can be called several time for a frame (such as first time  codec data, and second time others)
    * encoder will provide encoded data according to the format (whole frame, codec_data, sigle NAL etc)
    * If the buffer passed to encoded is not big enough, this API call will return ENCODE_BUFFER_TOO_SMALL
    * and caller should provide a big enough buffer and call again
    */
    virtual Encode_Status getOutput(VideoEncOutputBuffer *outBuffer);

    virtual Encode_Status getParameters(VideoParamConfigSet *videoEncParams);
    virtual Encode_Status setParameters(VideoParamConfigSet *videoEncParams);
    virtual Encode_Status setConfig(VideoParamConfigSet *videoEncConfig);
    virtual Encode_Status getConfig(VideoParamConfigSet *videoEncConfig);

    virtual Encode_Status getMaxOutSize(uint32_t *maxSize);
    virtual Encode_Status getStatistics(VideoStatistics *videoStat);

protected:
    virtual Encode_Status sendEncodeCommand(void) = 0;
    virtual Encode_Status derivedSetParams(VideoParamConfigSet *videoEncParams) = 0;
    virtual Encode_Status derivedGetParams(VideoParamConfigSet *videoEncParams) = 0;
    virtual Encode_Status derivedGetConfig(VideoParamConfigSet *videoEncConfig) = 0;
    virtual Encode_Status derivedSetConfig(VideoParamConfigSet *videoEncConfig) = 0;

    Encode_Status prepareForOutput(VideoEncOutputBuffer *outBuffer, bool *useLocalBuffer);
    Encode_Status cleanupForOutput();
    Encode_Status outputAllData(VideoEncOutputBuffer *outBuffer);
    Encode_Status renderDynamicFrameRate();
    Encode_Status renderDynamicBitrate();
    Encode_Status renderHrd();
    void setKeyFrame(int32_t keyFramePeriod);

private:
    void setDefaultParams(void);
    Encode_Status setUpstreamBuffer(VideoParamsUpstreamBuffer *upStreamBuffer);
    Encode_Status getNewUsrptrFromSurface(uint32_t width, uint32_t height, uint32_t format,
            uint32_t expectedSize, uint32_t *outsize, uint32_t *stride, uint8_t **usrptr);
    Encode_Status surfaceMappingForSurface(SurfaceMap *map);
    Encode_Status surfaceMappingForGfxHandle(SurfaceMap *map);
    Encode_Status surfaceMappingForCI(SurfaceMap *map);
    Encode_Status surfaceMappingForKbufHandle(SurfaceMap *map);
    Encode_Status surfaceMappingForMalloc(SurfaceMap *map);
    Encode_Status surfaceMapping(SurfaceMap *map);

    SurfaceMap *appendSurfaceMap(
            SurfaceMap *head, SurfaceMap *map);
    SurfaceMap *removeSurfaceMap(
            SurfaceMap *head, SurfaceMap *map);
    SurfaceMap *findSurfaceMapByValue(
            SurfaceMap *head, int32_t value);

    Encode_Status manageSrcSurface(VideoEncRawBuffer *inBuffer);
    void updateProperities(void);
    void decideFrameType(void);
//    Encode_Status uploadDataToSurface(VideoEncRawBuffer *inBuffer);
    Encode_Status syncEncode(VideoEncRawBuffer *inBuffer);
    Encode_Status asyncEncode(VideoEncRawBuffer *inBuffer);

protected:

    bool mInitialized;
    VADisplay mVADisplay;
    VAContextID mVAContext;
    VAConfigID mVAConfig;
    VAEntrypoint mVAEntrypoint;

    VACodedBufferSegment *mCurSegment;
    uint32_t mOffsetInSeg;
    uint32_t mTotalSize;
    uint32_t mTotalSizeCopied;

    VideoParamsCommon mComParams;
    VideoParamsHRD mHrdParam;
    VideoParamsStoreMetaDataInBuffers mStoreMetaDataInBuffers;

    bool mForceKeyFrame;
    bool mNewHeader;
    bool mFirstFrame;

    bool mRenderMaxSliceSize; //Max Slice Size
    bool mRenderQP;
    bool mRenderAIR;
    bool mRenderFrameRate;
    bool mRenderBitRate;
    bool mRenderHrd;

    VABufferID mVACodedBuffer[2];
    VABufferID mLastCodedBuffer;
    VABufferID mOutCodedBuffer;
    VABufferID mSeqParamBuf;
    VABufferID mRcParamBuf;
    VABufferID mFrameRateParamBuf;
    VABufferID mPicParamBuf;
    VABufferID mSliceParamBuf;

    VASurfaceID *mSurfaces;
    uint32_t mSurfaceCnt;

    SurfaceMap *mSrcSurfaceMapList;

    //for new design
    VASurfaceID mCurSurface;        //current input surface to be encoded 
    VASurfaceID mRefSurface;        //reference surface
    VASurfaceID mRecSurface;        //reconstructed surface
    VASurfaceID mLastSurface;       //last surface

    VideoEncRawBuffer *mLastInputRawBuffer;

    uint32_t mEncodedFrames;
    uint32_t mFrameNum;
    uint32_t mCodedBufSize;
    uint32_t mCodedBufIndex;

    bool mPicSkipped;
    bool mIsIntra;
    bool mSliceSizeOverflow;
    bool mCodedBufferMapped;
    bool mDataCopiedOut;
    bool mKeyFrame;

    int32_t  mInitCheck;

#ifdef VIDEO_ENC_STATISTICS_ENABLE
    VideoStatistics mVideoStat;
#endif

    // Constants
    static const uint32_t VENCODER_NUMBER_EXTRA_SURFACES_SHARED_MODE = 2;
    static const uint32_t VENCODER_NUMBER_EXTRA_SURFACES_NON_SHARED_MODE = 8;
};


#endif /* __VIDEO_ENCODER_BASE_H__ */
