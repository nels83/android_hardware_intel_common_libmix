/*
 INTEL CONFIDENTIAL
 Copyright 2011 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */
#include <string.h>
#include "VideoEncoderLog.h"
#include "VideoEncoderBase.h"
#include <va/va_tpi.h>
#include <va/va_android.h>

VideoEncoderBase::VideoEncoderBase()
    :mInitialized(false)
    ,mVADisplay(NULL)
    ,mVAContext(0)
    ,mVAConfig(0)
    ,mVAEntrypoint(VAEntrypointEncSlice)
    ,mCurSegment(NULL)
    ,mOffsetInSeg(0)
    ,mTotalSize(0)
    ,mTotalSizeCopied(0)
    ,mBufferMode(BUFFER_SHARING_NONE)
    ,mUpstreamBufferList(NULL)
    ,mUpstreamBufferCnt(0)
    ,mForceKeyFrame(false)
    ,mNewHeader(false)
    ,mFirstFrame (true)
    ,mRenderMaxSliceSize(false)
    ,mRenderQP (false)
    ,mRenderAIR(false)
    ,mRenderFrameRate(false)
    ,mRenderBitRate(false)
    ,mLastCodedBuffer(0)
    ,mOutCodedBuffer(0)
    ,mSeqParamBuf(0)
    ,mPicParamBuf(0)
    ,mSliceParamBuf(0)
    ,mSharedSurfaces(NULL)
    ,mSurfaces(NULL)
    ,mSurfaceCnt(0)
    ,mSharedSurfacesCnt(0)
    ,mReqSurfacesCnt(0)
    ,mUsrPtr(NULL)
    ,mVideoSrcBufferList(NULL)
    ,mCurFrame(NULL)
    ,mRefFrame(NULL)
    ,mRecFrame(NULL)
    ,mLastFrame(NULL)
    ,mLastInputRawBuffer(NULL)
    ,mEncodedFrames(0)
    ,mFrameNum(0)
    ,mCodedBufSize(0)
    ,mCodedBufIndex(0)
    ,mPicSkipped(false)
    ,mIsIntra(true)
    ,mSliceSizeOverflow(false)
    ,mCodedBufferMapped(false)
    ,mDataCopiedOut(false)
    ,mKeyFrame(true) {

    VAStatus vaStatus = VA_STATUS_SUCCESS;
    // here the display can be any value, use following one
    // just for consistence purpose, so don't define it
    unsigned int display = 0x18C34078;
    int majorVersion = -1;
    int minorVersion = -1;

    setDefaultParams();
    mVACodedBuffer [0] = 0;
    mVACodedBuffer [1] = 0;

    LOG_V("vaGetDisplay \n");
    mVADisplay = vaGetDisplay(&display);
    if (mVADisplay == NULL) {
        LOG_E("vaGetDisplay failed.");
    }

    vaStatus = vaInitialize(mVADisplay, &majorVersion, &minorVersion);
    LOG_V("vaInitialize \n");
    if (vaStatus != VA_STATUS_SUCCESS) {
        LOG_E( "Failed vaInitialize, vaStatus = %d\n", vaStatus);
    }
}

VideoEncoderBase::~VideoEncoderBase() {

    VAStatus vaStatus = VA_STATUS_SUCCESS;
    vaStatus = vaTerminate(mVADisplay);
    LOG_V( "vaTerminate\n");
    if (vaStatus != VA_STATUS_SUCCESS) {
        LOG_W( "Failed vaTerminate, vaStatus = %d\n", vaStatus);
    } else {
        mVADisplay = NULL;
    }
}

Encode_Status VideoEncoderBase::start() {

    Encode_Status ret = ENCODE_SUCCESS;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VASurfaceID *surfaces = NULL;

    VAConfigAttrib vaAttrib[2];
    uint32_t index;
    uint32_t maxSize = 0;

    VideoEncSurfaceBuffer *videoSurfaceBuffer = NULL;
    uint32_t normalSurfacesCnt = 2;

    if (mInitialized) {
        LOG_V("Encoder has been started\n");
        return ENCODE_ALREADY_INIT;
    }

    // For upstream allocates buffer, it is mandatory to set buffer mode
    // and for other stuff, it is optional
    // Different buffer mode will have different surface handling approach

    // mSharedSurfacesCnt is for upstream buffer allocation case
    mSharedSurfacesCnt = 0;

    vaAttrib[0].type = VAConfigAttribRTFormat;
    vaAttrib[1].type = VAConfigAttribRateControl;
    vaAttrib[0].value = VA_RT_FORMAT_YUV420;
    vaAttrib[1].value = mComParams.rcMode;

    LOG_V( "======VA Configuration======\n");

    LOG_I( "profile = %d\n", mComParams.profile);
    LOG_I( "mVAEntrypoint = %d\n", mVAEntrypoint);
    LOG_I( "vaAttrib[0].type = %d\n", vaAttrib[0].type);
    LOG_I( "vaAttrib[1].type = %d\n", vaAttrib[1].type);
    LOG_I( "vaAttrib[0].value (Format) = %d\n", vaAttrib[0].value);
    LOG_I( "vaAttrib[1].value (RC mode) = %d\n", vaAttrib[1].value);

    LOG_V( "vaCreateConfig\n");

    vaStatus = vaCreateConfig(
            mVADisplay, mComParams.profile, mVAEntrypoint,
            &vaAttrib[0], 2, &(mVAConfig));
    CHECK_VA_STATUS_GOTO_CLEANUP("vaCreateConfig");

    if (mComParams.rcMode == VA_RC_VCM) {

        // Following three features are only enabled in VCM mode
        mRenderMaxSliceSize = true;
        mRenderAIR = true;
        mRenderBitRate = true;
    }

    LOG_I("mReqSurfacesCnt = %d\n", mReqSurfacesCnt);

    if (mReqSurfacesCnt == 0) {
        switch (mBufferMode) {
            case BUFFER_SHARING_CI: {
                mSharedSurfacesCnt = mUpstreamBufferCnt;
                normalSurfacesCnt = VENCODER_NUMBER_EXTRA_SURFACES_SHARED_MODE;

                if (mSharedSurfacesCnt != 0) {
                    mSharedSurfaces = new VASurfaceID[mSharedSurfacesCnt];

                    if (mSharedSurfaces == NULL) {
                        LOG_E("Failed allocate shared surface\n");
                        ret = ENCODE_NO_MEMORY;
                        goto CLEAN_UP;
                    }
                }
            }
            break;
            case BUFFER_SHARING_V4L2:
            case BUFFER_SHARING_SURFACE:
                // To be develped
                break;
            default:
                mBufferMode = BUFFER_SHARING_NONE;
                normalSurfacesCnt = VENCODER_NUMBER_EXTRA_SURFACES_NON_SHARED_MODE;
                break;
        }
    } else if (mReqSurfacesCnt == 1) {
        // TODO: Un-normal case,
        mBufferMode = BUFFER_SHARING_NONE;
        normalSurfacesCnt = VENCODER_NUMBER_EXTRA_SURFACES_NON_SHARED_MODE;
    } else {
        mBufferMode = BUFFER_SHARING_USRPTR;
        mUsrPtr = new  uint8_t *[mReqSurfacesCnt];
        if (mUsrPtr == NULL) {
            LOG_E("Failed allocate memory\n");
            ret = ENCODE_NO_MEMORY;
            goto CLEAN_UP;
        }
    }

    LOG_I("mBufferMode = %d\n", mBufferMode);

    mSurfaceCnt = normalSurfacesCnt + mSharedSurfacesCnt + mReqSurfacesCnt;

    surfaces = new VASurfaceID[normalSurfacesCnt];
    if (surfaces == NULL) {
        LOG_E("Failed allocate surface\n");
        ret = ENCODE_NO_MEMORY;
        goto CLEAN_UP;
    }

    mSurfaces = new VASurfaceID[mSurfaceCnt] ;
    if (mSurfaces == NULL) {
        LOG_E("Failed allocate private surface\n");
        ret = ENCODE_NO_MEMORY;
        goto CLEAN_UP;
    }

    vaStatus = vaCreateSurfaces(mVADisplay, mComParams.resolution.width,
            mComParams.resolution.height, VA_RT_FORMAT_YUV420,
            normalSurfacesCnt, surfaces, NULL , 0);
    CHECK_VA_STATUS_GOTO_CLEANUP("vaCreateSurfaces");

    switch (mBufferMode) {
        case BUFFER_SHARING_CI: {
            for (index = 0; index < mSharedSurfacesCnt; index++) {

                vaStatus = vaCreateSurfaceFromCIFrame(
                        mVADisplay, (uint32_t)mUpstreamBufferCnt, &mSharedSurfaces[index]);

                CHECK_VA_STATUS_GOTO_CLEANUP("vaCreateSurfaceFromCIFrame");

                mSurfaces[index] = mSharedSurfaces[index];

                videoSurfaceBuffer = new VideoEncSurfaceBuffer;
                if (videoSurfaceBuffer == NULL) {
                    LOG_E( "new VideoEncSurfaceBuffer failed\n");
                    return ENCODE_NO_MEMORY;
                }

                videoSurfaceBuffer->surface = mSharedSurfaces[index];
                videoSurfaceBuffer->usrptr = NULL;
                videoSurfaceBuffer->index = index;
                videoSurfaceBuffer->bufAvailable = true;
                videoSurfaceBuffer->next = NULL;

                mVideoSrcBufferList = appendVideoSurfaceBuffer
                        (mVideoSrcBufferList, videoSurfaceBuffer);
                videoSurfaceBuffer = NULL;
            }
        }
        break;
        case BUFFER_SHARING_V4L2:
        case BUFFER_SHARING_SURFACE:
            // To be develped
            break;
        case BUFFER_SHARING_NONE:
            break;
        case BUFFER_SHARING_USRPTR: {
            videoSurfaceBuffer = mVideoSrcBufferList;
            index = 0;
            while (videoSurfaceBuffer != NULL) {
                mSurfaces[index] = videoSurfaceBuffer->surface;
                mUsrPtr [index] = videoSurfaceBuffer->usrptr;
                videoSurfaceBuffer = videoSurfaceBuffer->next;
                index ++;
            }
        }
        break;
        default:
            break;
    }

    for (index = 0; index < normalSurfacesCnt; index++) {
        mSurfaces[mReqSurfacesCnt + mSharedSurfacesCnt + index] = surfaces[index];

        videoSurfaceBuffer = new VideoEncSurfaceBuffer;
        if (videoSurfaceBuffer == NULL) {
            LOG_E( "new VideoEncSurfaceBuffer failed\n");
            return ENCODE_NO_MEMORY;
        }

        videoSurfaceBuffer->surface = surfaces[index];
        videoSurfaceBuffer->usrptr = NULL;
        videoSurfaceBuffer->index = mReqSurfacesCnt + mSharedSurfacesCnt + index;
        videoSurfaceBuffer->bufAvailable = true;
        videoSurfaceBuffer->next = NULL;

        mVideoSrcBufferList = appendVideoSurfaceBuffer(mVideoSrcBufferList, videoSurfaceBuffer);

        videoSurfaceBuffer = NULL;
    }

    LOG_V( "assign surface Done\n");
    LOG_I( "Created %d libva surfaces\n", mSurfaceCnt);

    //Initialize and save the VA context ID
    LOG_V( "vaCreateContext\n");

    vaStatus = vaCreateContext(mVADisplay, mVAConfig,
            mComParams.resolution.width,
            mComParams.resolution.height,
            0, mSurfaces, mSurfaceCnt,
            &(mVAContext));
    CHECK_VA_STATUS_GOTO_CLEANUP("vaCreateContext");

    LOG_I("Created libva context width %d, height %d\n",
          mComParams.resolution.width, mComParams.resolution.height);

    ret = getMaxOutSize(&maxSize);
    CHECK_ENCODE_STATUS_CLEANUP("getMaxOutSize");

    // Create coded buffer for output
    vaStatus = vaCreateBuffer(mVADisplay, mVAContext,
            VAEncCodedBufferType,
            mCodedBufSize,
            1, NULL,
            &(mVACodedBuffer[0]));

    CHECK_VA_STATUS_GOTO_CLEANUP("vaCreateBuffer::VAEncCodedBufferType");

    // Create coded buffer for output
    vaStatus = vaCreateBuffer(mVADisplay, mVAContext,
            VAEncCodedBufferType,
            mCodedBufSize,
            1, NULL,
            &(mVACodedBuffer[1]));

    CHECK_VA_STATUS_GOTO_CLEANUP("vaCreateBuffer::VAEncCodedBufferType");

    mFirstFrame = true;

CLEAN_UP:

    if (ret == ENCODE_SUCCESS) {
        mInitialized = true;
    }

    if (surfaces) delete []surfaces;

    LOG_V( "end\n");
    return ret;
}

Encode_Status VideoEncoderBase::encode(VideoEncRawBuffer *inBuffer) {

    Encode_Status ret = ENCODE_SUCCESS;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    uint8_t *buf = NULL;

    if (!mInitialized) {
        LOG_E("Encoder has not initialized yet\n");
        return ENCODE_NOT_INIT;
    }

    CHECK_NULL_RETURN_IFFAIL(inBuffer);

    inBuffer->bufAvailable = false;
    if (mNewHeader) mFrameNum = 0;

    // current we use one surface for source data,
    // one for reference and one for reconstructed
    decideFrameType();
    ret = manageSrcSurface(inBuffer);
    CHECK_ENCODE_STATUS_RETURN("manageSrcSurface");

    // Start encoding process
    LOG_V( "vaBeginPicture\n");
    LOG_I( "mVAContext = 0x%08x\n",(uint32_t) mVAContext);
    LOG_I( "Surface = 0x%08x\n",(uint32_t) mCurFrame->surface);
    LOG_I( "mVADisplay = 0x%08x\n",(uint32_t)mVADisplay);

    vaStatus = vaBeginPicture(mVADisplay, mVAContext, mCurFrame->surface);
    CHECK_VA_STATUS_RETURN("vaBeginPicture");

    ret = sendEncodeCommand();
    CHECK_ENCODE_STATUS_RETURN("sendEncodeCommand");

    if ((mComParams.rcMode == VA_RC_NONE) || mFirstFrame) {
        vaStatus = vaEndPicture(mVADisplay, mVAContext);
        CHECK_VA_STATUS_RETURN("vaEndPicture");
    }

    LOG_V( "vaEndPicture\n");

    if (mFirstFrame) {
        updateProperities();
        decideFrameType();
    }

    LOG_I ("vaSyncSurface ID = 0x%08x\n", mLastFrame->surface);
    vaStatus = vaSyncSurface(mVADisplay, mLastFrame->surface);
    if (vaStatus != VA_STATUS_SUCCESS) {
        LOG_W( "Failed vaSyncSurface\n");
    }

    mOutCodedBuffer = mLastCodedBuffer;

    // Need map buffer before calling query surface below to get
    // the right skip frame flag for current frame
    // It is a requirement of video driver
    vaStatus = vaMapBuffer (mVADisplay, mOutCodedBuffer, (void **)&buf);
    vaStatus = vaUnmapBuffer(mVADisplay, mOutCodedBuffer);

    if (!((mComParams.rcMode == VA_RC_NONE) || mFirstFrame)) {
        vaStatus = vaEndPicture(mVADisplay, mVAContext);
        CHECK_VA_STATUS_RETURN("vaEndPicture");

    }

    if (mFirstFrame) {
        vaStatus = vaBeginPicture(mVADisplay, mVAContext, mCurFrame->surface);
        CHECK_VA_STATUS_RETURN("vaBeginPicture");

        ret = sendEncodeCommand();
        CHECK_ENCODE_STATUS_RETURN("sendEncodeCommand");

        vaStatus = vaEndPicture(mVADisplay, mVAContext);
        CHECK_VA_STATUS_RETURN("vaEndPicture");

        mKeyFrame = true;
    }

    // Query the status of current surface
    VASurfaceStatus vaSurfaceStatus;
    vaStatus = vaQuerySurfaceStatus(mVADisplay, mCurFrame->surface,  &vaSurfaceStatus);
    CHECK_VA_STATUS_RETURN("vaQuerySurfaceStatus");

    mPicSkipped = vaSurfaceStatus & VASurfaceSkipped;

    if (!mFirstFrame) {
        VideoEncoderBase::appendVideoSurfaceBuffer(mVideoSrcBufferList, mLastFrame);
    }

    mLastFrame = NULL;
    updateProperities();
    mCurFrame = NULL;

    if (mLastInputRawBuffer) mLastInputRawBuffer->bufAvailable = true;

    LOG_V("ref the current inBuffer\n");

    mLastInputRawBuffer = inBuffer;
    mFirstFrame = false;

    return ENCODE_SUCCESS;
}

Encode_Status VideoEncoderBase::getOutput(VideoEncOutputBuffer *outBuffer) {

    Encode_Status ret = ENCODE_SUCCESS;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    bool useLocalBuffer = false;

    CHECK_NULL_RETURN_IFFAIL(outBuffer);

    LOG_V("Begin\n");

    if (outBuffer->format != OUTPUT_EVERYTHING && outBuffer->format != OUTPUT_FRAME_DATA) {
        LOG_E("Output buffer mode not supported\n");
        goto CLEAN_UP;
    }

    // For first getOutput, the mFrameNum already increased to 2, and of course is key frame
    // frame 0 is already encoded and will be outputed here
    // frame 1 is encoding now, frame 2 will be sent to encoder for next encode() call
    if (mFrameNum > 2) {
        if (mComParams.intraPeriod != 0 &&
                (((mFrameNum - 2) % mComParams.intraPeriod) == 0)) {
            mKeyFrame = true;
        } else {
            mKeyFrame = false;
        }
    }

    ret = prepareForOutput(outBuffer, &useLocalBuffer);
    CHECK_ENCODE_STATUS_CLEANUP("prepareForOutput");

    ret = outputAllData(outBuffer);
    CHECK_ENCODE_STATUS_CLEANUP("outputAllData");

    LOG_I("out size for this getOutput call = %d\n", outBuffer->dataSize);

    ret = cleanupForOutput();
    CHECK_ENCODE_STATUS_CLEANUP("cleanupForOutput");

CLEAN_UP:

    if (ret < ENCODE_SUCCESS) {
        if (outBuffer->data && (useLocalBuffer == true)) {
            delete[] outBuffer->data;
            outBuffer->data = NULL;
            useLocalBuffer = false;
        }

        if (mCodedBufferMapped) {
            vaStatus = vaUnmapBuffer(mVADisplay, mOutCodedBuffer);
            mCodedBufferMapped = false;
            mCurSegment = NULL;
        }
    }

    LOG_V("End\n");
    return ret;
}


void VideoEncoderBase::flush() {

    LOG_V( "Begin\n");

    // put reconstructed surface back to list
    if (mRecFrame != NULL) {
        appendVideoSurfaceBuffer(mVideoSrcBufferList, mRecFrame);
        mRecFrame = NULL;
    }

    // put reference surface back to list
    if (mRefFrame != NULL) {
        appendVideoSurfaceBuffer(mVideoSrcBufferList, mRefFrame);
        mRefFrame = NULL;
    }

    // Here this raw buffer means the surface being encoding
    if (mLastInputRawBuffer) {
        mLastInputRawBuffer->bufAvailable = true;
        mLastInputRawBuffer = NULL;
    }

    // reset the properities
    mEncodedFrames = 0;
    mFrameNum = 0;
    mPicSkipped = false;
    mIsIntra = true;

    LOG_V( "end\n");
}

Encode_Status VideoEncoderBase::stop() {

    VAStatus vaStatus = VA_STATUS_SUCCESS;
    Encode_Status ret = ENCODE_SUCCESS;
    VideoEncSurfaceBuffer *videoSurfaceBuffer = NULL;
    VideoEncSurfaceBuffer *tmpBuffer = NULL;


    LOG_V( "Begin\n");

    if (mSharedSurfaces) {
        delete [] mSharedSurfaces;
        mSharedSurfaces = NULL;
    }

    if (mSurfaces) {
        delete [] mSurfaces;
        mSurfaces = NULL;
    }

    if (mUsrPtr) {
        delete [] mUsrPtr;
        mUsrPtr = NULL;
    }

    if (mUpstreamBufferList) {
        delete [] mUpstreamBufferList;
        mUpstreamBufferList = NULL;
    }

    // It is possible that above pointers have been allocated
    // before we set mInitialized to true
    if (!mInitialized) {
        LOG_V("Encoder has been stopped\n");
        return ENCODE_SUCCESS;
    }

    LOG_V( "Release frames\n");

    // put reconstructed surface back to list
    if (mRecFrame != NULL) {
        appendVideoSurfaceBuffer(mVideoSrcBufferList, mRecFrame);
        mRecFrame = NULL;
    }

    // put reference surface back to list
    if (mRefFrame != NULL) {
        appendVideoSurfaceBuffer(mVideoSrcBufferList, mRefFrame);
        mRefFrame = NULL;
    }

    // put Source surface back to list
    if (mLastFrame != NULL) {
        appendVideoSurfaceBuffer(mVideoSrcBufferList, mLastFrame);
        mLastFrame = NULL;
    }

    LOG_V( "Release surfaces\n");


    LOG_V( "vaDestroyContext\n");
    vaStatus = vaDestroyContext(mVADisplay, mVAContext);
    CHECK_VA_STATUS_GOTO_CLEANUP("vaDestroyContext");

    LOG_V( "vaDestroyConfig\n");
    vaStatus = vaDestroyConfig(mVADisplay, mVAConfig);
    CHECK_VA_STATUS_GOTO_CLEANUP("vaDestroyConfig");

    // Release Src Surface Buffer List
    LOG_V( "Rlease Src Surface Buffer \n");

    videoSurfaceBuffer = mVideoSrcBufferList;

    while (videoSurfaceBuffer != NULL) {
        tmpBuffer = videoSurfaceBuffer;
        videoSurfaceBuffer = videoSurfaceBuffer->next;
        delete tmpBuffer;
    }

CLEAN_UP:
    mInitialized = false;
    LOG_V( "end\n");
    return ret;
}


Encode_Status VideoEncoderBase::prepareForOutput(
        VideoEncOutputBuffer *outBuffer, bool *useLocalBuffer) {

    Encode_Status ret = ENCODE_SUCCESS;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VACodedBufferSegment *vaCodedSeg = NULL;
    uint32_t status = 0;
    uint8_t *buf = NULL;

    LOG_V( "begin\n");
    // Won't check parameters here as the caller already checked them
    // mCurSegment is NULL means it is first time to be here after finishing encoding a frame
    if (mCurSegment == NULL && !mCodedBufferMapped) {
        LOG_I ("Coded Buffer ID been mapped = 0x%08x\n", mOutCodedBuffer);
        vaStatus = vaMapBuffer (mVADisplay, mOutCodedBuffer, (void **)&buf);
        CHECK_VA_STATUS_RETURN("vaMapBuffer");
        CHECK_NULL_RETURN_IFFAIL(buf);

        mCodedBufferMapped = true;
        mTotalSize = 0;
        mOffsetInSeg = 0;
        mTotalSizeCopied = 0;
        vaCodedSeg = (VACodedBufferSegment *)buf;
        mCurSegment = (VACodedBufferSegment *)buf;

        while (1) {

            mTotalSize += vaCodedSeg->size;
            status = vaCodedSeg->status;

            if (!mSliceSizeOverflow) {
                mSliceSizeOverflow = status & VA_CODED_BUF_STATUS_SLICE_OVERFLOW_MASK;
            }

            if (vaCodedSeg->next == NULL)
                break;

            vaCodedSeg = (VACodedBufferSegment *)vaCodedSeg->next;
        }
    }


    // We will support two buffer allocation mode,
    // one is application allocates the buffer and passes to encode,
    // the other is encode allocate memory

    //means  app doesn't allocate the buffer, so _encode will allocate it.
    if (outBuffer->data == NULL) {
        *useLocalBuffer = true;
        outBuffer->data = new  uint8_t[mTotalSize - mTotalSizeCopied + 100];
        if (outBuffer->data == NULL) {
            LOG_E( "outBuffer->data == NULL\n");
            return ENCODE_NO_MEMORY;
        }
        outBuffer->bufferSize = mTotalSize + 100;
        outBuffer->dataSize = 0;
    }

    // Clear all flag for every call
    outBuffer->flag = 0;
    if (mSliceSizeOverflow) outBuffer->flag |= ENCODE_BUFFERFLAG_SLICEOVERFOLOW;

    if (mCurSegment->size < mOffsetInSeg) {
        LOG_E("mCurSegment->size < mOffsetInSeg\n");
        return ENCODE_FAIL;
    }

    // Make sure we have data in current segment
    if (mCurSegment->size == mOffsetInSeg) {
        if (mCurSegment->next != NULL) {
            mCurSegment = (VACodedBufferSegment *)mCurSegment->next;
            mOffsetInSeg = 0;
        } else {
            LOG_V("No more data available\n");
            outBuffer->flag |= ENCODE_BUFFERFLAG_DATAINVALID;
            outBuffer->dataSize = 0;
            mCurSegment = NULL;
            return ENCODE_NO_REQUEST_DATA;
        }
    }

    LOG_V( "end\n");
    return ENCODE_SUCCESS;
}

Encode_Status VideoEncoderBase::cleanupForOutput() {

    VAStatus vaStatus = VA_STATUS_SUCCESS;
    Encode_Status ret = ENCODE_SUCCESS;

    //mCurSegment is NULL means all data has been copied out
    if (mCurSegment == NULL && mCodedBufferMapped) {
        vaStatus = vaUnmapBuffer(mVADisplay, mOutCodedBuffer);
        CHECK_VA_STATUS_RETURN("vaUnmapBuffer");
        mCodedBufferMapped = false;
    }
    return ENCODE_SUCCESS;
}


Encode_Status VideoEncoderBase::outputAllData(
        VideoEncOutputBuffer *outBuffer) {

    // Data size been copied for every single call
    uint32_t sizeCopiedHere = 0;
    uint32_t sizeToBeCopied = 0;

    CHECK_NULL_RETURN_IFFAIL(outBuffer->data);

    while (1) {

        LOG_I("mCurSegment->size = %d, mOffsetInSeg = %d\n", mCurSegment->size, mOffsetInSeg);
        LOG_I("outBuffer->bufferSize = %d, sizeCopiedHere = %d, mTotalSizeCopied = %d\n",
              outBuffer->bufferSize, sizeCopiedHere, mTotalSizeCopied);

        if (mCurSegment->size < mOffsetInSeg || outBuffer->bufferSize < sizeCopiedHere) {
            LOG_E("mCurSegment->size < mOffsetInSeg  || outBuffer->bufferSize < sizeCopiedHere\n");
            return ENCODE_FAIL;
        }

        if ((mCurSegment->size - mOffsetInSeg) <= outBuffer->bufferSize - sizeCopiedHere) {
            sizeToBeCopied = mCurSegment->size - mOffsetInSeg;
            memcpy(outBuffer->data + sizeCopiedHere,
                   (uint8_t *)mCurSegment->buf + mOffsetInSeg, sizeToBeCopied);
            sizeCopiedHere += sizeToBeCopied;
            mTotalSizeCopied += sizeToBeCopied;
            mOffsetInSeg = 0;
        } else {
            sizeToBeCopied = outBuffer->bufferSize - sizeCopiedHere;
            memcpy(outBuffer->data + sizeCopiedHere,
                   (uint8_t *)mCurSegment->buf + mOffsetInSeg, outBuffer->bufferSize - sizeCopiedHere);
            mTotalSizeCopied += sizeToBeCopied;
            mOffsetInSeg += sizeToBeCopied;
            outBuffer->dataSize = outBuffer->bufferSize;
            outBuffer->remainingSize = mTotalSize - mTotalSizeCopied;
            outBuffer->flag |= ENCODE_BUFFERFLAG_PARTIALFRAME;
            if (mKeyFrame) outBuffer->flag |= ENCODE_BUFFERFLAG_SYNCFRAME;
            return ENCODE_BUFFER_TOO_SMALL;
        }

        if (mCurSegment->next == NULL) {
            outBuffer->dataSize = sizeCopiedHere;
            outBuffer->remainingSize = 0;
            outBuffer->flag |= ENCODE_BUFFERFLAG_ENDOFFRAME;
            if (mKeyFrame) outBuffer->flag |= ENCODE_BUFFERFLAG_SYNCFRAME;
            mCurSegment = NULL;
            return ENCODE_SUCCESS;
        }

        mCurSegment = (VACodedBufferSegment *)mCurSegment->next;
        mOffsetInSeg = 0;
    }
}

void VideoEncoderBase::setDefaultParams() {

    // Set default value for input parameters
    mComParams.profile = VAProfileH264Baseline;
    mComParams.level = 40;
    mComParams.rawFormat = RAW_FORMAT_NV12;
    mComParams.frameRate.frameRateNum = 30;
    mComParams.frameRate.frameRateDenom = 1;
    mComParams.resolution.width = 0;
    mComParams.resolution.height = 0;
    mComParams.intraPeriod = 30;
    mComParams.rcMode = RATE_CONTROL_NONE;
    mComParams.rcParams.initQP = 15;
    mComParams.rcParams.minQP = 1;
    mComParams.rcParams.bitRate = 640000;
    mComParams.rcParams.targetPercentage= 95;
    mComParams.rcParams.windowSize = 500;
    mComParams.cyclicFrameInterval = 30;
    mComParams.refreshType = VIDEO_ENC_NONIR;
    mComParams.airParams.airMBs = 0;
    mComParams.airParams.airThreshold = 0;
    mComParams.airParams.airAuto = 1;
    mComParams.disableDeblocking = 2;
}

Encode_Status VideoEncoderBase::setParameters(
        VideoParamConfigSet *videoEncParams) {

    Encode_Status ret = ENCODE_SUCCESS;
    CHECK_NULL_RETURN_IFFAIL(videoEncParams);
    LOG_I("Config type = %d\n", (int)videoEncParams->type);

    if (mInitialized) {
        LOG_E("Encoder has been initialized, should use setConfig to change configurations\n");
        return ENCODE_ALREADY_INIT;
    }

    switch (videoEncParams->type) {
        case VideoParamsTypeCommon: {

            VideoParamsCommon *paramsCommon =
                    reinterpret_cast <VideoParamsCommon *> (videoEncParams);

            if (paramsCommon->size != sizeof (VideoParamsCommon)) {
                return ENCODE_INVALID_PARAMS;
            }
            mComParams = *paramsCommon;
            break;
        }

        case VideoParamsTypeUpSteamBuffer: {

            VideoParamsUpstreamBuffer *upStreamBuffer =
                    reinterpret_cast <VideoParamsUpstreamBuffer *> (videoEncParams);

            if (upStreamBuffer->size != sizeof (VideoParamsUpstreamBuffer)) {
                return ENCODE_INVALID_PARAMS;
            }

            ret = setUpstreamBuffer(
                    upStreamBuffer->bufferMode, upStreamBuffer->bufList, upStreamBuffer->bufCnt);
            break;
        }

        case VideoParamsTypeUsrptrBuffer: {

            // usrptr only can be get
            // this case should not happen
            break;
        }

        case VideoParamsTypeAVC:
        case VideoParamsTypeH263:
        case VideoParamsTypeMP4:
        case VideoParamsTypeVC1: {
            ret = derivedSetParams(videoEncParams);
            break;
        }

        default: {
            LOG_E ("Wrong ParamType here\n");
            break;
        }

    }

    return ret;

}


Encode_Status VideoEncoderBase::getParameters(
        VideoParamConfigSet *videoEncParams) {

    Encode_Status ret = ENCODE_SUCCESS;
    CHECK_NULL_RETURN_IFFAIL(videoEncParams);
    LOG_I("Config type = %d\n", (int)videoEncParams->type);

    switch (videoEncParams->type) {
        case VideoParamsTypeCommon: {

            VideoParamsCommon *paramsCommon =
                    reinterpret_cast <VideoParamsCommon *> (videoEncParams);

            if (paramsCommon->size != sizeof (VideoParamsCommon)) {
                return ENCODE_INVALID_PARAMS;
            }
            *paramsCommon = mComParams;
            break;
        }

        case VideoParamsTypeUpSteamBuffer: {

            // Get upstream buffer could happen
            // but not meaningful a lot
            break;
        }

        case VideoParamsTypeUsrptrBuffer: {
            VideoParamsUsrptrBuffer *usrptrBuffer =
                    reinterpret_cast <VideoParamsUsrptrBuffer *> (videoEncParams);

            if (usrptrBuffer->size != sizeof (VideoParamsUsrptrBuffer)) {
                return ENCODE_INVALID_PARAMS;
            }

            ret = getNewUsrptrFromSurface(
                    usrptrBuffer->width, usrptrBuffer->height, usrptrBuffer->format,
                    usrptrBuffer->expectedSize, &(usrptrBuffer->actualSize),
                    &(usrptrBuffer->stride), &(usrptrBuffer->usrPtr));

            break;
        }

        case VideoParamsTypeAVC:
        case VideoParamsTypeH263:
        case VideoParamsTypeMP4:
        case VideoParamsTypeVC1: {
            derivedGetParams(videoEncParams);
            break;
        }

        default: {
            LOG_E ("Wrong ParamType here\n");
            break;
        }

    }
    return ENCODE_SUCCESS;
}

Encode_Status VideoEncoderBase::setConfig(VideoParamConfigSet *videoEncConfig) {

    Encode_Status ret = ENCODE_SUCCESS;
    CHECK_NULL_RETURN_IFFAIL(videoEncConfig);
    LOG_I("Config type = %d\n", (int)videoEncConfig->type);

    if (!mInitialized) {
        LOG_E("Encoder has not initialized yet, can't call setConfig\n");
        return ENCODE_NOT_INIT;
    }

    switch (videoEncConfig->type) {
        case VideoConfigTypeFrameRate: {
            VideoConfigFrameRate *configFrameRate =
                    reinterpret_cast <VideoConfigFrameRate *> (videoEncConfig);

            if (configFrameRate->size != sizeof (VideoConfigFrameRate)) {
                return ENCODE_INVALID_PARAMS;
            }
            mComParams.frameRate = configFrameRate->frameRate;
            mRenderFrameRate = true;
            break;
        }

        case VideoConfigTypeBitRate: {
            VideoConfigBitRate *configBitRate =
                    reinterpret_cast <VideoConfigBitRate *> (videoEncConfig);

            if (configBitRate->size != sizeof (VideoConfigBitRate)) {
                return ENCODE_INVALID_PARAMS;
            }
            mComParams.rcParams = configBitRate->rcParams;

            mRenderBitRate = true;

            break;
        }
        case VideoConfigTypeResolution: {

            // Not Implemented
            break;
        }
        case VideoConfigTypeIntraRefreshType: {

            VideoConfigIntraRefreshType *configIntraRefreshType =
                    reinterpret_cast <VideoConfigIntraRefreshType *> (videoEncConfig);

            if (configIntraRefreshType->size != sizeof (VideoConfigIntraRefreshType)) {
                return ENCODE_INVALID_PARAMS;
            }
            mComParams.refreshType = configIntraRefreshType->refreshType;

            break;
        }

        case VideoConfigTypeCyclicFrameInterval: {
            VideoConfigCyclicFrameInterval *configCyclicFrameInterval =
                    reinterpret_cast <VideoConfigCyclicFrameInterval *> (videoEncConfig);
            if (configCyclicFrameInterval->size != sizeof (VideoConfigCyclicFrameInterval)) {
                return ENCODE_INVALID_PARAMS;
            }

            mComParams.cyclicFrameInterval = configCyclicFrameInterval->cyclicFrameInterval;
            break;
        }

        case VideoConfigTypeAIR: {

            VideoConfigAIR *configAIR = reinterpret_cast <VideoConfigAIR *> (videoEncConfig);

            if (configAIR->size != sizeof (VideoConfigAIR)) {
                return ENCODE_INVALID_PARAMS;
            }

            mComParams.airParams = configAIR->airParams;

            mRenderAIR = true;

            break;
        }
        case VideoConfigTypeAVCIntraPeriod:
        case VideoConfigTypeNALSize:
        case VideoConfigTypeIDRRequest:
        case VideoConfigTypeSliceNum: {

            ret = derivedSetConfig(videoEncConfig);

            break;
        }
        default: {
            LOG_E ("Wrong Config Type here\n");
            break;
        }
    }


    return ret;
}

Encode_Status VideoEncoderBase::getConfig(VideoParamConfigSet *videoEncConfig) {

    Encode_Status ret = ENCODE_SUCCESS;
    CHECK_NULL_RETURN_IFFAIL(videoEncConfig);
    LOG_I("Config type = %d\n", (int)videoEncConfig->type);

    switch (videoEncConfig->type) {
        case VideoConfigTypeFrameRate: {
            VideoConfigFrameRate *configFrameRate =
                    reinterpret_cast <VideoConfigFrameRate *> (videoEncConfig);

            if (configFrameRate->size != sizeof (VideoConfigFrameRate)) {
                return ENCODE_INVALID_PARAMS;
            }

            configFrameRate->frameRate = mComParams.frameRate;

            break;
        }

        case VideoConfigTypeBitRate: {
            VideoConfigBitRate *configBitRate =
                    reinterpret_cast <VideoConfigBitRate *> (videoEncConfig);

            if (configBitRate->size != sizeof (VideoConfigBitRate)) {
                return ENCODE_INVALID_PARAMS;
            }
            configBitRate->rcParams = mComParams.rcParams;


            break;
        }
        case VideoConfigTypeResolution: {
            // Not Implemented
            break;
        }
        case VideoConfigTypeIntraRefreshType: {

            VideoConfigIntraRefreshType *configIntraRefreshType =
                    reinterpret_cast <VideoConfigIntraRefreshType *> (videoEncConfig);

            if (configIntraRefreshType->size != sizeof (VideoConfigIntraRefreshType)) {
                return ENCODE_INVALID_PARAMS;
            }
            configIntraRefreshType->refreshType = mComParams.refreshType;

            break;
        }

        case VideoConfigTypeCyclicFrameInterval: {
            VideoConfigCyclicFrameInterval *configCyclicFrameInterval =
                    reinterpret_cast <VideoConfigCyclicFrameInterval *> (videoEncConfig);
            if (configCyclicFrameInterval->size != sizeof (VideoConfigCyclicFrameInterval)) {
                return ENCODE_INVALID_PARAMS;
            }

            configCyclicFrameInterval->cyclicFrameInterval = mComParams.cyclicFrameInterval;

            break;
        }

        case VideoConfigTypeAIR: {

            VideoConfigAIR *configAIR = reinterpret_cast <VideoConfigAIR *> (videoEncConfig);

            if (configAIR->size != sizeof (VideoConfigAIR)) {
                return ENCODE_INVALID_PARAMS;
            }

            configAIR->airParams = mComParams.airParams;

            break;
        }
        case VideoConfigTypeAVCIntraPeriod:
        case VideoConfigTypeNALSize:
        case VideoConfigTypeIDRRequest:
        case VideoConfigTypeSliceNum: {

            ret = derivedGetConfig(videoEncConfig);

            break;
        }
        default: {
            LOG_E ("Wrong ParamType here\n");
            break;
        }
    }

    return ret;
}

void VideoEncoderBase:: decideFrameType () {

    LOG_I( "mEncodedFrames = %d\n", mEncodedFrames);
    LOG_I( "mFrameNum = %d\n", mFrameNum);
    LOG_I( "mIsIntra = %d\n", mIsIntra);

    // determine the picture type
    if (mComParams.intraPeriod == 0) {
        if (mFrameNum == 0)
            mIsIntra = true;
        else
            mIsIntra = false;
    } else if ((mFrameNum % mComParams.intraPeriod) == 0) {
        mIsIntra = true;
    } else {
        mIsIntra = false;
    }

    LOG_I( "mIsIntra = %d\n",mIsIntra);
}


void VideoEncoderBase:: updateProperities () {

    VideoEncSurfaceBuffer *tmpFrame = NULL;
    LOG_V( "Begin\n");

    mEncodedFrames ++;
    mFrameNum ++;
    mLastCodedBuffer = mVACodedBuffer[mCodedBufIndex];
    mCodedBufIndex ++;
    mCodedBufIndex %=2;

    mLastFrame = mCurFrame;

    if (!mPicSkipped) {
        tmpFrame = mRecFrame;
        mRecFrame = mRefFrame;
        mRefFrame = tmpFrame;
    }

    LOG_V( "End\n");
}


Encode_Status  VideoEncoderBase::getMaxOutSize (uint32_t *maxSize) {

    uint32_t size = mComParams.resolution.width * mComParams.resolution.height;

    if (maxSize == NULL) {
        LOG_E("maxSize == NULL\n");
        return ENCODE_NULL_PTR;
    }


    LOG_V( "Begin\n");


    if (mCodedBufSize > 0) {
        *maxSize = mCodedBufSize;
        LOG_V ("Already calculate the max encoded size, get the value directly");
        return ENCODE_SUCCESS;
    }

    // base on the rate control mode to calculate the defaule encoded buffer size
    if (mComParams.rcMode == VA_RC_NONE) {
        mCodedBufSize = (size * 400) / (16 * 16);
        // set to value according to QP
    } else {
        mCodedBufSize = mComParams.rcParams.bitRate / 4;
    }

    mCodedBufSize =
        max (mCodedBufSize , (size * 400) / (16 * 16));

    // in case got a very large user input bit rate value
    mCodedBufSize =
        min(mCodedBufSize, (size * 1.5 * 8));
    mCodedBufSize =  (mCodedBufSize + 15) &(~15);

    *maxSize = mCodedBufSize;
    return ENCODE_SUCCESS;
}

Encode_Status VideoEncoderBase::getNewUsrptrFromSurface(
    uint32_t width, uint32_t height, uint32_t format,
    uint32_t expectedSize, uint32_t *outsize, uint32_t *stride, uint8_t **usrptr) {

    Encode_Status ret = ENCODE_FAIL;
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    VASurfaceID surface = VA_INVALID_SURFACE;
    VAImage image;
    uint32_t index = 0;

    VideoEncSurfaceBuffer *videoSurfaceBuffer = NULL;

    LOG_V( "Begin\n");

    // If encode session has been configured, we can not request surface creation anymore
    if (mInitialized) {
        LOG_E( "Already Initialized, can not request VA surface anymore\n");
        return ENCODE_WRONG_STATE;
    }

    if (width<=0 || height<=0 ||outsize == NULL ||stride == NULL || usrptr == NULL) {
        LOG_E("width<=0 || height<=0 || outsize == NULL || stride == NULL ||usrptr == NULL\n");
        return ENCODE_NULL_PTR;
    }

    // Current only NV12 is supported in VA API
    // Through format we can get known the number of planes
    if (format != STRING_TO_FOURCC("NV12")) {

        LOG_W ("Format is not supported\n");
        return ENCODE_NOT_SUPPORTED;
    }

    vaStatus = vaCreateSurfacesForUserPtr(mVADisplay, width, height, VA_RT_FORMAT_YUV420, 1,
            &surface, expectedSize, VA_FOURCC_NV12, width, width, width,
            0, width * height, width * height);

    CHECK_VA_STATUS_RETURN("vaCreateSurfacesForUserPtr");

    vaStatus = vaDeriveImage(mVADisplay, surface, &image);
    CHECK_VA_STATUS_RETURN("vaDeriveImage");

    LOG_V( "vaDeriveImage Done\n");

    vaStatus = vaMapBuffer(mVADisplay, image.buf, (void **) usrptr);
    CHECK_VA_STATUS_RETURN("vaMapBuffer");

    // make sure the physical page been allocated
    for (index = 0; index < image.data_size; index = index + 4096) {
        unsigned char tmp =  *(*usrptr + index);
        if (tmp == 0)
            *(*usrptr + index) = 0;
    }

    *outsize = image.data_size;
    *stride = image.pitches[0];

    videoSurfaceBuffer = new VideoEncSurfaceBuffer;
    if (videoSurfaceBuffer == NULL) {
        LOG_E( "new VideoEncSurfaceBuffer failed\n");
        return ENCODE_NO_MEMORY;
    }

    videoSurfaceBuffer->surface = surface;
    videoSurfaceBuffer->usrptr = *usrptr;
    videoSurfaceBuffer->index = mReqSurfacesCnt;
    videoSurfaceBuffer->bufAvailable = true;
    videoSurfaceBuffer->next = NULL;

    mVideoSrcBufferList = appendVideoSurfaceBuffer(mVideoSrcBufferList, videoSurfaceBuffer);

    LOG_I( "surface = 0x%08x\n",(uint32_t)surface);
    LOG_I("image->pitches[0] = %d\n", image.pitches[0]);
    LOG_I("image->pitches[1] = %d\n", image.pitches[1]);
    LOG_I("image->offsets[0] = %d\n", image.offsets[0]);
    LOG_I("image->offsets[1] = %d\n", image.offsets[1]);
    LOG_I("image->num_planes = %d\n", image.num_planes);
    LOG_I("image->width = %d\n", image.width);
    LOG_I("image->height = %d\n", image.height);

    LOG_I ("data_size = %d\n", image.data_size);
    LOG_I ("usrptr = 0x%p\n", *usrptr);
    LOG_I ("mReqSurfacesCnt = %d\n", mReqSurfacesCnt);
    LOG_I ("videoSurfaceBuffer->usrptr = 0x%p\n ", videoSurfaceBuffer->usrptr);

    videoSurfaceBuffer = NULL;

    vaStatus = vaUnmapBuffer(mVADisplay, image.buf);
    CHECK_VA_STATUS_RETURN("vaUnmapBuffer");


    vaStatus = vaDestroyImage(mVADisplay, image.image_id);
    CHECK_VA_STATUS_RETURN("vaDestroyImage");

    if (*outsize < expectedSize) {
        LOG_E ("Allocated buffer size is small than the expected size, destroy the surface");
        LOG_I ("Allocated size is %d, expected size is %d\n", *outsize, expectedSize);
        vaStatus = vaDestroySurfaces(mVADisplay, &surface, 1);
        CHECK_VA_STATUS_RETURN("vaDestroySurfaces");
        return ENCODE_FAIL;
    }

    mReqSurfacesCnt ++;
    ret = ENCODE_SUCCESS;

    return ret;
}


Encode_Status VideoEncoderBase::setUpstreamBuffer(
        VideoBufferSharingMode bufferMode, uint32_t *bufList, uint32_t bufCnt) {

    CHECK_NULL_RETURN_IFFAIL(bufList);
    if (bufCnt == 0) {
        LOG_E("bufCnt == 0\n");
        return ENCODE_FAIL;
    }

    if (mUpstreamBufferList) delete [] mUpstreamBufferList;

    mUpstreamBufferCnt = bufCnt;
    mUpstreamBufferList = new uint32_t [bufCnt];
    if (!mUpstreamBufferList) {
        LOG_E ("mUpstreamBufferList NULL\n");
        return ENCODE_NO_MEMORY;
    }

    memcpy(mUpstreamBufferList, bufList, bufCnt * sizeof (uint32_t));
    return ENCODE_SUCCESS;

}


Encode_Status VideoEncoderBase::manageSrcSurface(VideoEncRawBuffer *inBuffer) {

    Encode_Status ret = ENCODE_SUCCESS;
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    uint32_t idx = 0;
    uint32_t bufIndex = 0;

    if (mBufferMode == BUFFER_SHARING_CI) {

        memcpy(&bufIndex, inBuffer->data, sizeof(unsigned int));
        // bufIndex = *(uint32_t*)inBuffer->data;

        LOG_I("mSurfaceCnt = %d\n", mSurfaceCnt);
        LOG_I("bufIndex = %d\n", bufIndex);

        if (bufIndex > mSurfaceCnt - 2) {
            LOG_E("the CI frame idx is bigger than total CI frame count\n");
            ret = ENCODE_FAIL;
            return ret;

        }

    } else if (mBufferMode == BUFFER_SHARING_USRPTR) {

        bufIndex = (uint32_t) -1; //fixme, temp use a big value

        LOG_I("bufin->data = 0x%p	\n", inBuffer->data);

        for (idx = 0; idx < mReqSurfacesCnt; idx++) {
            LOG_I("mUsrPtr[%d] = 0x%p\n", idx, mUsrPtr[idx]);

            if (inBuffer->data == mUsrPtr[idx])
                bufIndex = idx;
        }

        LOG_I("mSurfaceCnt = %d\n", mSurfaceCnt);
        LOG_I("bufIndex = %d\n", bufIndex);

        if (bufIndex > mSurfaceCnt - 2) {
            LOG_W("the Surface idx is too big, most likely the buffer passed in is not allocated by us\n");
            ret = ENCODE_FAIL;
            goto no_share_mode;

        }
    }


    switch (mBufferMode) {

        case BUFFER_SHARING_CI:
        case BUFFER_SHARING_USRPTR: {

            if (mRefFrame== NULL) {
                mRefFrame = getVideoSurfaceBufferByIndex(mVideoSrcBufferList, mSurfaceCnt -1 );
                if (mRefFrame == NULL) {
                    LOG_E ("No Surface buffer available, something should be wrong\n");
                    return ENCODE_FAIL;
                }
                mVideoSrcBufferList = removeVideoSurfaceBuffer(mVideoSrcBufferList, mRefFrame);

            }

            if (mRecFrame== NULL) {
                mRecFrame = getVideoSurfaceBufferByIndex(mVideoSrcBufferList, mSurfaceCnt - 2);
                if (mRecFrame == NULL) {
                    LOG_E ("No Surface buffer available, something should be wrong\n");
                    return ENCODE_FAIL;
                }
                mVideoSrcBufferList = removeVideoSurfaceBuffer(mVideoSrcBufferList, mRecFrame);

            }

            if (mCurFrame== NULL) {
                mCurFrame = getVideoSurfaceBufferByIndex(mVideoSrcBufferList, bufIndex);
                if (mCurFrame == NULL) {
                    LOG_E ("No Surface buffer available, something should be wrong\n");
                    return ENCODE_FAIL;
                }
                mVideoSrcBufferList = removeVideoSurfaceBuffer(mVideoSrcBufferList, mCurFrame);
            }
        }

        break;
        case BUFFER_SHARING_V4L2:
        case BUFFER_SHARING_SURFACE:
            LOG_E("Not Implemented\n");
            break;

        case BUFFER_SHARING_NONE: {
no_share_mode:

            if (mRefFrame== NULL) {
                mRefFrame = mVideoSrcBufferList;
                if (mRefFrame == NULL) {
                    LOG_E("No Surface buffer available, something should be wrong\n");
                    return ENCODE_FAIL;
                }
                mVideoSrcBufferList = removeVideoSurfaceBuffer(mVideoSrcBufferList, mRefFrame);

            }

            if (mRecFrame== NULL) {
                mRecFrame = mVideoSrcBufferList;
                if (mRecFrame == NULL) {
                    LOG_E ("No Surface buffer available, something should be wrong\n");
                    return ENCODE_FAIL;
                }
                mVideoSrcBufferList = removeVideoSurfaceBuffer(mVideoSrcBufferList, mRecFrame);

            }

            if (mCurFrame== NULL) {
                mCurFrame = mVideoSrcBufferList;
                if (mCurFrame == NULL) {
                    LOG_E ("No Surface buffer available, something should be wrong\n");
                    return ENCODE_FAIL;
                }
                mVideoSrcBufferList = removeVideoSurfaceBuffer(mVideoSrcBufferList, mCurFrame);
            }

            LOG_V( "Get Surface Done\n");
            ret = uploadDataToSurface (inBuffer);
            CHECK_ENCODE_STATUS_RETURN("uploadDataToSurface");
        }
        break;
        default:
            break;

    }

    return ENCODE_SUCCESS;
}

VideoEncSurfaceBuffer *VideoEncoderBase::appendVideoSurfaceBuffer(
        VideoEncSurfaceBuffer *head, VideoEncSurfaceBuffer *buffer) {

    if (head == NULL) {
        return buffer;
    }

    VideoEncSurfaceBuffer *node = head;
    VideoEncSurfaceBuffer *tail = NULL;

    while (node != NULL) {
        tail = node;
        node = node->next;
    }
    tail->next = buffer;

    return head;
}

VideoEncSurfaceBuffer *VideoEncoderBase::removeVideoSurfaceBuffer(
        VideoEncSurfaceBuffer *head, VideoEncSurfaceBuffer *buffer) {

    VideoEncSurfaceBuffer *node = head;
    VideoEncSurfaceBuffer *tmpNode = NULL;

    if (head == buffer) {
        tmpNode = head->next;
        buffer->next = NULL;
        return tmpNode;
    }

    while (node != NULL) {
        if (node->next == buffer)
            break;
        node = node->next;
    }

    if (node != NULL) {
        node->next = buffer->next;
    }

    buffer->next = NULL;
    return head;

}

VideoEncSurfaceBuffer *VideoEncoderBase::getVideoSurfaceBufferByIndex(
        VideoEncSurfaceBuffer *head, uint32_t index) {
    VideoEncSurfaceBuffer *node = head;

    while (node != NULL) {
        if (node->index == index)
            break;
        node = node->next;
    }

    return node;
}

Encode_Status VideoEncoderBase::uploadDataToSurface(VideoEncRawBuffer *inBuffer) {

    VAStatus vaStatus = VA_STATUS_SUCCESS;

    uint32_t width = mComParams.resolution.width;
    uint32_t height = mComParams.resolution.height;

    VAImage srcImage;
    uint8_t *pvBuffer;
    uint8_t *dstY;
    uint8_t *dstUV;
    uint32_t i,j;

    uint8_t *inBuf = inBuffer->data;
    VAImage *image = NULL;

    int uvOffset = width * height;
    uint8_t *uvBufIn = inBuf + uvOffset;
    uint32_t uvHeight = height / 2;
    uint32_t uvWidth = width;

    LOG_V("map source data to surface\n");
    LOG_I("Surface ID = 0x%08x\n", (uint32_t) mCurFrame->surface);

    vaStatus = vaDeriveImage(mVADisplay, mCurFrame->surface, &srcImage);
    CHECK_VA_STATUS_RETURN("vaDeriveImage");

    LOG_V( "vaDeriveImage Done\n");

    image = &srcImage;

    vaStatus = vaMapBuffer(mVADisplay, image->buf, (void **)&pvBuffer);
    CHECK_VA_STATUS_RETURN("vaMapBuffer");

    LOG_V("vaImage information\n");
    LOG_I("image->pitches[0] = %d\n", image->pitches[0]);
    LOG_I("image->pitches[1] = %d\n", image->pitches[1]);
    LOG_I("image->offsets[0] = %d\n", image->offsets[0]);
    LOG_I("image->offsets[1] = %d\n", image->offsets[1]);
    LOG_I("image->num_planes = %d\n", image->num_planes);
    LOG_I("image->width = %d\n", image->width);
    LOG_I("image->height = %d\n", image->height);

    LOG_I("input buf size = %d\n", inBuffer->size);

    if (mComParams.rawFormat == RAW_FORMAT_YUV420) {
        dstY = pvBuffer +image->offsets[0];

        for (i = 0; i < height; i ++) {
            memcpy(dstY, inBuf + i * width, width);
            dstY += image->pitches[0];
        }

        dstUV = pvBuffer + image->offsets[1];

        for (i = 0; i < height / 2; i ++) {
            for (j = 0; j < width; j+=2) {
                dstUV [j] = inBuf [width * height + i * width / 2 + j / 2];
                dstUV [j + 1] =
                    inBuf [width * height * 5 / 4 + i * width / 2 + j / 2];
            }
            dstUV += image->pitches[1];
        }
    }

    else if (mComParams.rawFormat == RAW_FORMAT_NV12) {

        dstY = pvBuffer + image->offsets[0];
        for (i = 0; i < height; i++) {
            memcpy(dstY, inBuf + i * width, width);
            dstY += image->pitches[0];
        }

        dstUV = pvBuffer + image->offsets[1];
        for (i = 0; i < uvHeight; i++) {
            memcpy(dstUV, uvBufIn + i * uvWidth, uvWidth);
            dstUV += image->pitches[1];
        }
    } else {
        LOG_E("Raw format not supoort\n");
        return ENCODE_FAIL;
    }

    vaStatus = vaUnmapBuffer(mVADisplay, image->buf);
    CHECK_VA_STATUS_RETURN("vaUnmapBuffer");

    vaStatus = vaDestroyImage(mVADisplay, srcImage.image_id);
    CHECK_VA_STATUS_RETURN("vaDestroyImage");

    return ENCODE_SUCCESS;
}

Encode_Status VideoEncoderBase::renderDynamicBitrate() {
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    LOG_V( "Begin\n\n");

    if (mComParams.rcMode != RATE_CONTROL_VCM) {

        LOG_W("Not in VCM mode, but call renderDynamicBitrate\n");
        return ENCODE_SUCCESS;
    }

    VAEncMiscParameterBuffer   *miscEncParamBuf;
    VAEncMiscParameterRateControl *bitrateControlParam;
    VABufferID miscParamBufferID;

    vaStatus = vaCreateBuffer(mVADisplay, mVAContext,
            VAEncMiscParameterBufferType,
            sizeof (VAEncMiscParameterBuffer) + sizeof (VAEncMiscParameterRateControl),
            1, NULL,
            &miscParamBufferID);

    CHECK_VA_STATUS_RETURN("vaCreateBuffer");

    vaStatus = vaMapBuffer(mVADisplay, miscParamBufferID, (void **)&miscEncParamBuf);
    CHECK_VA_STATUS_RETURN("vaMapBuffer");

    miscEncParamBuf->type = VAEncMiscParameterTypeRateControl;
    bitrateControlParam = (VAEncMiscParameterRateControl *)miscEncParamBuf->data;

    bitrateControlParam->bits_per_second = mComParams.rcParams.bitRate;
    bitrateControlParam->initial_qp = mComParams.rcParams.initQP;
    bitrateControlParam->min_qp = mComParams.rcParams.minQP;
    bitrateControlParam->target_percentage = mComParams.rcParams.targetPercentage;
    bitrateControlParam->window_size = mComParams.rcParams.windowSize;

    vaStatus = vaUnmapBuffer(mVADisplay, miscParamBufferID);
    CHECK_VA_STATUS_RETURN("vaUnmapBuffer");


    vaStatus = vaRenderPicture(mVADisplay, mVAContext,
            &miscParamBufferID, 1);
    CHECK_VA_STATUS_RETURN("vaRenderPicture");

    return ENCODE_SUCCESS;
}


Encode_Status VideoEncoderBase::renderDynamicFrameRate() {

    VAStatus vaStatus = VA_STATUS_SUCCESS;

    if (mComParams.rcMode != RATE_CONTROL_VCM) {

        LOG_W("Not in VCM mode, but call SendDynamicFramerate\n");
        return ENCODE_SUCCESS;
    }

    VAEncMiscParameterBuffer   *miscEncParamBuf;
    VAEncMiscParameterFrameRate *frameRateParam;
    VABufferID miscParamBufferID;

    vaStatus = vaCreateBuffer(mVADisplay, mVAContext,
            VAEncMiscParameterBufferType,
            sizeof(miscEncParamBuf) + sizeof(VAEncMiscParameterFrameRate),
            1, NULL, &miscParamBufferID);
    CHECK_VA_STATUS_RETURN("vaCreateBuffer");

    vaStatus = vaMapBuffer(mVADisplay, miscParamBufferID, (void **)&miscEncParamBuf);
    CHECK_VA_STATUS_RETURN("vaMapBuffer");

    miscEncParamBuf->type = VAEncMiscParameterTypeFrameRate;
    frameRateParam = (VAEncMiscParameterFrameRate *)miscEncParamBuf->data;
    frameRateParam->framerate =
            (unsigned int) (mComParams.frameRate.frameRateNum + mComParams.frameRate.frameRateDenom/2)
            / mComParams.frameRate.frameRateDenom;

    vaStatus = vaUnmapBuffer(mVADisplay, miscParamBufferID);
    CHECK_VA_STATUS_RETURN("vaUnmapBuffer");

    vaStatus = vaRenderPicture(mVADisplay, mVAContext, &miscParamBufferID, 1);
    CHECK_VA_STATUS_RETURN("vaRenderPicture");

    LOG_I( "frame rate = %d\n", frameRateParam->framerate);
    return ENCODE_SUCCESS;
}
