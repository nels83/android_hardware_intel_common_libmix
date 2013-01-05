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
#include <utils/List.h>
#include <utils/threads.h>

struct SurfaceMap {
    VASurfaceID surface;
    MetadataBufferType type;
    int32_t value;
    ValueInfo vinfo;
    bool added;
};

struct EncodeTask {
    VASurfaceID enc_surface;
    VASurfaceID ref_surface;
    VASurfaceID rec_surface;
    VABufferID coded_buffer;

    FrameType type;
    int flag;
    int64_t timestamp;  //corresponding input frame timestamp
    void *priv;  //input buffer data

    bool completed;   //if encode task is done complet by HW
};

class VideoEncoderBase : IVideoEncoder {

public:
    VideoEncoderBase();
    virtual ~VideoEncoderBase();

    virtual Encode_Status start(void);
    virtual void flush(void);
    virtual Encode_Status stop(void);
    virtual Encode_Status encode(VideoEncRawBuffer *inBuffer, uint32_t timeout);

    /*
    * getOutput can be called several time for a frame (such as first time  codec data, and second time others)
    * encoder will provide encoded data according to the format (whole frame, codec_data, sigle NAL etc)
    * If the buffer passed to encoded is not big enough, this API call will return ENCODE_BUFFER_TOO_SMALL
    * and caller should provide a big enough buffer and call again
    */
    virtual Encode_Status getOutput(VideoEncOutputBuffer *outBuffer, uint32_t timeout);

    virtual Encode_Status getParameters(VideoParamConfigSet *videoEncParams);
    virtual Encode_Status setParameters(VideoParamConfigSet *videoEncParams);
    virtual Encode_Status setConfig(VideoParamConfigSet *videoEncConfig);
    virtual Encode_Status getConfig(VideoParamConfigSet *videoEncConfig);
    virtual Encode_Status getMaxOutSize(uint32_t *maxSize);

protected:
    virtual Encode_Status sendEncodeCommand(EncodeTask* task) = 0;
    virtual Encode_Status derivedSetParams(VideoParamConfigSet *videoEncParams) = 0;
    virtual Encode_Status derivedGetParams(VideoParamConfigSet *videoEncParams) = 0;
    virtual Encode_Status derivedGetConfig(VideoParamConfigSet *videoEncConfig) = 0;
    virtual Encode_Status derivedSetConfig(VideoParamConfigSet *videoEncConfig) = 0;
    virtual Encode_Status getExtFormatOutput(VideoEncOutputBuffer *outBuffer) = 0;
    virtual Encode_Status updateFrameInfo(EncodeTask* task) ;

    Encode_Status renderDynamicFrameRate();
    Encode_Status renderDynamicBitrate();
    Encode_Status renderHrd();
    Encode_Status queryProfileLevelConfig(VADisplay dpy, VAProfile profile);

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
    SurfaceMap *findSurfaceMapByValue(int32_t value);
    Encode_Status manageSrcSurface(VideoEncRawBuffer *inBuffer, VASurfaceID *sid);
    void PrepareFrameInfo(EncodeTask* task);

    Encode_Status prepareForOutput(VideoEncOutputBuffer *outBuffer, bool *useLocalBuffer);
    Encode_Status cleanupForOutput();
    Encode_Status outputAllData(VideoEncOutputBuffer *outBuffer);
    Encode_Status queryAutoReferenceConfig(VAProfile profile);

protected:

    bool mInitialized;
    bool mStarted;
    VADisplay mVADisplay;
    VAContextID mVAContext;
    VAConfigID mVAConfig;
    VAEntrypoint mVAEntrypoint;


    VideoParamsCommon mComParams;
    VideoParamsHRD mHrdParam;
    VideoParamsStoreMetaDataInBuffers mStoreMetaDataInBuffers;

    bool mNewHeader;

    bool mRenderMaxSliceSize; //Max Slice Size
    bool mRenderQP;
    bool mRenderAIR;
    bool mRenderFrameRate;
    bool mRenderBitRate;
    bool mRenderHrd;

    VABufferID mSeqParamBuf;
    VABufferID mRcParamBuf;
    VABufferID mFrameRateParamBuf;
    VABufferID mPicParamBuf;
    VABufferID mSliceParamBuf;
    VASurfaceID* mAutoRefSurfaces;

    android::List <SurfaceMap *> mSrcSurfaceMapList;  //all mapped surface info list from input buffer
    android::List <EncodeTask *> mEncodeTaskList;  //all encode tasks list
    android::List <VABufferID> mVACodedBufferList;  //all available codedbuffer list

    VASurfaceID mRefSurface;        //reference surface, only used in base
    VASurfaceID mRecSurface;        //reconstructed surface, only used in base
    uint32_t mFrameNum;
    uint32_t mCodedBufSize;
    bool mAutoReference;
    uint32_t mAutoReferenceSurfaceNum;

    bool mSliceSizeOverflow;

    //Current Outputting task
    EncodeTask *mCurOutputTask;

    //Current outputting CodedBuffer status
    VABufferID mOutCodedBuffer;
    bool mCodedBufferMapped;
    VACodedBufferSegment *mCurSegment;
    uint32_t mOffsetInSeg;
    uint32_t mTotalSize;
    uint32_t mTotalSizeCopied;
    android::Mutex               mCodedBuffer_Lock, mEncodeTask_Lock;
    android::Condition           mCodedBuffer_Cond, mEncodeTask_Cond;

    bool mFrameSkipped;
};
#endif /* __VIDEO_ENCODER_BASE_H__ */
