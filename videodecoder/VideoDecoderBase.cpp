/* INTEL CONFIDENTIAL
* Copyright (c) 2009-2011 Intel Corporation.  All rights reserved.
*
* The source code contained or described herein and all documents
* related to the source code ("Material") are owned by Intel
* Corporation or its suppliers or licensors.  Title to the
* Material remains with Intel Corporation or its suppliers and
* licensors.  The Material contains trade secrets and proprietary
* and confidential information of Intel or its suppliers and
* licensors. The Material is protected by worldwide copyright and
* trade secret laws and treaty provisions.  No part of the Material
* may be used, copied, reproduced, modified, published, uploaded,
* posted, transmitted, distributed, or disclosed in any way without
* Intel's prior express written permission.
*
* No license under any patent, copyright, trade secret or other
* intellectual property right is granted to or conferred upon you
* by disclosure or delivery of the Materials, either expressly, by
* implication, inducement, estoppel or otherwise. Any license
* under such intellectual property rights must be express and
* approved by Intel in writing.
*
*/

#include "VideoDecoderBase.h"
#include "VideoDecoderTrace.h"
#include <string.h>
#include <va/va_android.h>
#include <va/va_tpi.h>

#define INVALID_PTS ((uint64_t)-1)
#define MAXIMUM_POC  0x7FFFFFFF
#define MINIMUM_POC  0x80000000
#define ANDROID_DISPLAY_HANDLE 0x18C34078

// TODO: check what is the best number. Must be at least 2 to support one backward reference frame.
// Currently set to 8 to support 7 backward reference frames. This value is used for AVC frame reordering only.
// e.g:
// POC: 4P,  8P,  10P,  6B and mNextOutputPOC = 5
#define OUTPUT_WINDOW_SIZE 8


VideoDecoderBase::VideoDecoderBase(const char *mimeType, _vbp_parser_type type)
    : mDisplay(NULL),
      mVADisplay(NULL),
      mVAContext(VA_INVALID_ID),
      mVAConfig(VA_INVALID_ID),

      mVAStarted(false),

      mCurrentPTS(INVALID_PTS),
      mAcquiredBuffer(NULL),
      mLastReference(NULL),
      mForwardReference(NULL),
      mDecodingFrame(false),
      mSizeChanged(false),

      // private member variables
      mLowDelay(false),
      mRawOutput(false),
      mManageReference(true),
      mOutputMethod(OUTPUT_BY_PCT),
      mOutputWindowSize(OUTPUT_WINDOW_SIZE),
      mNumSurfaces(0),
      mSurfaceBuffers(NULL),
      mOutputHead(NULL),
      mOutputTail(NULL),
      mSurfaces(NULL),
      mVASurfaceAttrib(NULL),
      mVAExternalMemoryBuffers(NULL),
      mSurfaceUserPtr(NULL),
      mSurfaceAcquirePos(0),
      mNextOutputPOC(MINIMUM_POC),
      mParserType(type),
      mParserHandle(NULL),
      mInitialized(false),
      mSignalBufferSize(0) {

    memset(&mVideoFormatInfo, 0, sizeof(VideoFormatInfo));
    memset(&mConfigBuffer, 0, sizeof(mConfigBuffer));
    for (int i =0; i < MAX_GRAPHIC_NUM; i++) {
         mSignalBufferPre[i] = NULL;
    }
    pthread_mutex_init(&mLock, NULL);
    mVideoFormatInfo.mimeType = strdup(mimeType);
}

VideoDecoderBase::~VideoDecoderBase() {
    pthread_mutex_destroy(&mLock);
    stop();
    free(mVideoFormatInfo.mimeType);
}

Decode_Status VideoDecoderBase::start(VideoConfigBuffer *buffer) {
    if (buffer == NULL) {
        return DECODE_INVALID_DATA;
    }

    if (mParserHandle != NULL) {
        WTRACE("Decoder has already started.");
        return DECODE_SUCCESS;
    }

    if ((int32_t)mParserType != VBP_INVALID) {
        if (vbp_open(mParserType, &mParserHandle) != VBP_OK) {
            ETRACE("Failed to open VBP parser.");
            return DECODE_NO_PARSER;
        }
    }
    // keep a copy of configure buffer, meta data only. It can be used to override VA setup parameter.
    mConfigBuffer = *buffer;
    mConfigBuffer.data = NULL;
    mConfigBuffer.size = 0;

    mVideoFormatInfo.width = buffer->width;
    mVideoFormatInfo.height = buffer->height;
    mLowDelay = buffer->flag & WANT_LOW_DELAY;
    mRawOutput = buffer->flag & WANT_RAW_OUTPUT;
    if (mRawOutput) {
        WTRACE("Output is raw data.");
    }

    return DECODE_SUCCESS;
}

void VideoDecoderBase::stop(void) {
    terminateVA();

    mCurrentPTS = INVALID_PTS;
    mAcquiredBuffer = NULL;
    mLastReference = NULL;
    mForwardReference = NULL;
    mDecodingFrame = false;
    mSizeChanged = false;

    // private variables
    mLowDelay = false;
    mRawOutput = false;
    mNumSurfaces = 0;
    mSurfaceAcquirePos = 0;
    mNextOutputPOC = MINIMUM_POC;

    mVideoFormatInfo.valid = false;
    if (mParserHandle){
        vbp_close(mParserHandle);
        mParserHandle = NULL;
    }
}

void VideoDecoderBase::flush(void) {
    if (mVAStarted == false) {
        // nothing to flush at this stage
        return;
    }

    endDecodingFrame(true);

    // avoid setting mSurfaceAcquirePos  to 0 as it may cause tearing
    // (surface is still being rendered)
    mSurfaceAcquirePos = (mSurfaceAcquirePos  + 1) % mNumSurfaces;
    mNextOutputPOC = MINIMUM_POC;
    mCurrentPTS = INVALID_PTS;
    mAcquiredBuffer = NULL;
    mLastReference = NULL;
    mForwardReference = NULL;
    mOutputHead = NULL;
    mOutputTail = NULL;
    mDecodingFrame = false;

    // initialize surface buffer without resetting mapped/raw data
    initSurfaceBuffer(false);

}

const VideoFormatInfo* VideoDecoderBase::getFormatInfo(void) {
    return &mVideoFormatInfo;
}

const VideoRenderBuffer* VideoDecoderBase::getOutput(bool draining) {
    if (mVAStarted == false) {
        return NULL;
    }
    bool useGraphicBuffer = mConfigBuffer.flag & USE_NATIVE_GRAPHIC_BUFFER;

    if (draining) {
        // complete decoding the last frame and ignore return
        endDecodingFrame(false);
    }

    if (mOutputHead == NULL) {
        return NULL;
    }

    // output by position (the first buffer)
    VideoSurfaceBuffer *outputByPos = mOutputHead;

    if (mLowDelay) {
        mOutputHead = mOutputHead->next;
        if (mOutputHead == NULL) {
            mOutputTail = NULL;
        }
        if (useGraphicBuffer ) {
            vaSyncSurface(mVADisplay, outputByPos->renderBuffer.surface);
        }
        return &(outputByPos->renderBuffer);
    }

    // output by presentation time stamp (the smallest pts)
    VideoSurfaceBuffer *outputByPts = findOutputByPts(draining);

    VideoSurfaceBuffer *output = NULL;
    if (mOutputMethod == OUTPUT_BY_POC) {
        output = findOutputByPoc(draining);
    } else if (mOutputMethod == OUTPUT_BY_PCT) {
        output = findOutputByPct(draining);
    } else {
        ETRACE("Invalid output method.");
        return NULL;
    }

    if (output == NULL) {
        return NULL;
    }

    if (output != outputByPts) {
        // swap time stamp
        uint64_t ts = output->renderBuffer.timeStamp;
        output->renderBuffer.timeStamp = outputByPts->renderBuffer.timeStamp;
        outputByPts->renderBuffer.timeStamp = ts;
    }

    if (output != outputByPos) {
        // remove this output from middle or end of the list
        VideoSurfaceBuffer *p = outputByPos;
        while (p->next != output) {
            p = p->next;
        }
        p->next = output->next;
        if (mOutputTail == output) {
            mOutputTail = p;
        }
    } else {
        // remove this output from head of the list
        mOutputHead = mOutputHead->next;
        if (mOutputHead == NULL) {
            mOutputTail = NULL;
        }
    }
    if (useGraphicBuffer ) {
         vaSyncSurface(mVADisplay, output->renderBuffer.surface);
    }
    //VTRACE("Output POC %d for display (pts = %.2f)", output->pictureOrder, output->renderBuffer.timeStamp/1E6);
    return &(output->renderBuffer);
}

VideoSurfaceBuffer* VideoDecoderBase::findOutputByPts(bool draining) {
    // output by presentation time stamp - buffer with the smallest time stamp is output
    VideoSurfaceBuffer *p = mOutputHead;
    VideoSurfaceBuffer *outputByPts = NULL;
    uint64_t pts = INVALID_PTS;
    do {
        if ((uint64_t)(p->renderBuffer.timeStamp) <= pts) {
            // find buffer with the smallest PTS
            pts = p->renderBuffer.timeStamp;
            outputByPts = p;
        }
        p = p->next;
    } while (p != NULL);

    return outputByPts;
}

VideoSurfaceBuffer* VideoDecoderBase::findOutputByPct(bool draining) {
    // output by picture coding type (PCT)
    // if there is more than one reference frame, the first reference frame is ouput, otherwise,
    // output non-reference frame if there is any.

    VideoSurfaceBuffer *p = mOutputHead;
    VideoSurfaceBuffer *outputByPct = NULL;
    int32_t reference = 0;
    do {
        if (p->referenceFrame) {
            reference++;
            if (reference > 1) {
                // mOutputHead must be a reference frame
                outputByPct = mOutputHead;
                break;
            }
        } else {
            // first non-reference frame
            outputByPct = p;
            break;
        }
        p = p->next;
    } while (p != NULL);

    if (outputByPct == NULL && draining) {
        outputByPct = mOutputHead;
    }
    return  outputByPct;
}

#if 0
VideoSurfaceBuffer* VideoDecoderBase::findOutputByPoc(bool draining) {
    // output by picture order count (POC)
    // Output criteria:
    // if there is IDR frame (POC == 0), all the frames before IDR must be output;
    // Otherwise, if draining flag is set or list is full, frame with the least POC is output;
    // Otherwise, NOTHING is output

    int32_t dpbFullness = 0;
    for (int32_t i = 0; i < mNumSurfaces; i++) {
        // count num of reference frames
        if (mSurfaceBuffers[i].asReferernce) {
            dpbFullness++;
        }
    }

    if (mAcquiredBuffer && mAcquiredBuffer->asReferernce) {
        // frame is being decoded and is not ready for output yet
        dpbFullness--;
    }

    VideoSurfaceBuffer *p = mOutputHead;
    while (p != NULL) {
        // count dpbFullness with non-reference frame in the output queue
        if (p->asReferernce == false) {
            dpbFullness++;
        }
        p = p->next;
    }

Retry:
    p = mOutputHead;
    VideoSurfaceBuffer *outputByPoc = NULL;
    int32_t count = 0;
    int32_t poc = MAXIMUM_POC;

    do {
        if (p->pictureOrder == 0) {
            // output picture with the least POC before IDR
            if (outputByPoc != NULL) {
                mNextOutputPOC = outputByPoc->pictureOrder + 1;
                return outputByPoc;
            } else {
                mNextOutputPOC = MINIMUM_POC;
            }
        }

        // POC of  the output candidate must not be less than mNextOutputPOC
        if (p->pictureOrder < mNextOutputPOC) {
            break;
        }

        if (p->pictureOrder < poc) {
            // update the least POC.
            poc = p->pictureOrder;
            outputByPoc = p;
        }
        count++;
        p = p->next;
    } while (p != NULL && count < mOutputWindowSize);

    if (draining == false && dpbFullness < mOutputWindowSize) {
        // list is not  full and we are not  in draining state
        // if DPB is already full, one frame must be output
        return NULL;
    }

    if (outputByPoc == NULL) {
        mNextOutputPOC = MINIMUM_POC;
        goto Retry;
    }

    // for debugging purpose
    if (outputByPoc->pictureOrder != 0 && outputByPoc->pictureOrder < mNextOutputPOC) {
        ETRACE("Output POC is not incremental, expected %d, actual %d", mNextOutputPOC, outputByPoc->pictureOrder);
        //gaps_in_frame_num_value_allowed_flag is not currently supported
    }

    mNextOutputPOC = outputByPoc->pictureOrder + 1;

    return outputByPoc;
}
#else
VideoSurfaceBuffer* VideoDecoderBase::findOutputByPoc(bool draining) {
    VideoSurfaceBuffer *output = NULL;
    VideoSurfaceBuffer *p = mOutputHead;
    int32_t count = 0;
    int32_t poc = MAXIMUM_POC;
    VideoSurfaceBuffer *outputleastpoc = mOutputHead;
    do {
        count++;
        if (p->pictureOrder == 0) {
            // any picture before this POC (new IDR) must be output
            if (output == NULL) {
                mNextOutputPOC = MINIMUM_POC;
                // looking for any POC with negative value
            } else {
                mNextOutputPOC = output->pictureOrder + 1;
                break;
            }
        }
        if (p->pictureOrder < poc && p->pictureOrder >= mNextOutputPOC) {
            // this POC meets ouput criteria.
            poc = p->pictureOrder;
            output = p;
            outputleastpoc = p;
        }
        if (poc == mNextOutputPOC || count == OUTPUT_WINDOW_SIZE) {
            if (output != NULL) {
                // this indicates two cases:
                // 1) the next output POC is found.
                // 2) output queue is full and there is at least one buffer meeting the output criteria.
                mNextOutputPOC = output->pictureOrder + 1;
                break;
            } else {
                // this indicates output queue is full and no buffer in the queue meets the output criteria
                // restart processing as queue is FULL and output criteria is changed. (next output POC is 0)
                mNextOutputPOC = MINIMUM_POC;
                count = 0;
                poc = MAXIMUM_POC;
                p = mOutputHead;
                continue;
            }
        }
        if (p->next == NULL) {
            output = NULL;
        }

        p = p->next;
    } while (p != NULL);

    if (draining == true && output == NULL) {
        output = outputleastpoc;
    }

    return output;
}
#endif

bool VideoDecoderBase::checkBufferAvail(void) {
    if (!mInitialized) {
        if ((mConfigBuffer.flag & USE_NATIVE_GRAPHIC_BUFFER) == 0) {
            return true;
        }
        for (int i = 0; i < MAX_GRAPHIC_NUM; i++) {
            if (mSignalBufferPre[i] != NULL) {
                return true;
            }
        }
        return false;
    }
    // check whether there is buffer available for decoding
    // TODO: check frame being referenced for frame skipping
    VideoSurfaceBuffer *buffer = NULL;
    for (int32_t i = 0; i < mNumSurfaces; i++) {
        buffer = mSurfaceBuffers + i;
        if (buffer->asReferernce == false && buffer->renderBuffer.renderDone == true) {
            return true;
        }
     }
    return false;
}

Decode_Status VideoDecoderBase::acquireSurfaceBuffer(void) {
    if (mVAStarted == false) {
        return DECODE_FAIL;
    }

    if (mAcquiredBuffer != NULL) {
        ETRACE("mAcquiredBuffer is not NULL. Implementation bug.");
        return DECODE_FAIL;
    }

    int nextAcquire = mSurfaceAcquirePos;
    VideoSurfaceBuffer *acquiredBuffer = NULL;
    bool acquired = false;
    while (acquired == false) {
        acquiredBuffer = mSurfaceBuffers + nextAcquire;
        if (acquiredBuffer->asReferernce == false && acquiredBuffer->renderBuffer.renderDone == true) {
            // this is potential buffer for acquisition. Check if it is referenced by other surface for frame skipping
            VideoSurfaceBuffer *temp;
            acquired = true;
            for (int i = 0; i < mNumSurfaces; i++) {
                if (i == nextAcquire) {
                    continue;
                }
                temp = mSurfaceBuffers + i;
                // use mSurfaces[nextAcquire] instead of acquiredBuffer->renderBuffer.surface as its the actual surface to use.
                if (temp->renderBuffer.surface == mSurfaces[nextAcquire] &&
                    temp->renderBuffer.renderDone == false) {
                    ITRACE("Surface is referenced by other surface buffer.");
                    acquired = false;
                    break;
                }
            }
        }
        if (acquired) {
            break;
        }
        nextAcquire++;
        if (nextAcquire == mNumSurfaces) {
            nextAcquire = 0;
        }
        if (nextAcquire == mSurfaceAcquirePos) {
            return DECODE_NO_SURFACE;
        }
    }

    if (acquired == false) {
        return DECODE_NO_SURFACE;
    }

    mAcquiredBuffer = acquiredBuffer;
    mSurfaceAcquirePos = nextAcquire;

    // set surface again as surface maybe reset by skipped frame.
    // skipped frame is a "non-coded frame" and decoder needs to duplicate the previous reference frame as the output.
    mAcquiredBuffer->renderBuffer.surface = mSurfaces[mSurfaceAcquirePos];
    if (mSurfaceUserPtr && mAcquiredBuffer->mappedData) {
        mAcquiredBuffer->mappedData->data = mSurfaceUserPtr[mSurfaceAcquirePos];
    }
    mAcquiredBuffer->renderBuffer.timeStamp = INVALID_PTS;
    mAcquiredBuffer->renderBuffer.display = mVADisplay;
    mAcquiredBuffer->renderBuffer.flag = 0;
    mAcquiredBuffer->renderBuffer.renderDone = false;
    mAcquiredBuffer->asReferernce = false;

    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderBase::outputSurfaceBuffer(void) {
    Decode_Status status;
    if (mAcquiredBuffer == NULL) {
        ETRACE("mAcquiredBuffer is NULL. Implementation bug.");
        return DECODE_FAIL;
    }

    if (mRawOutput) {
        status = getRawDataFromSurface();
        CHECK_STATUS();
    }
    // frame is successfly decoded to the current surface,  it is ready for output
    mAcquiredBuffer->renderBuffer.renderDone = false;

    // decoder must set "asReference and referenceFrame" flags properly

    // update reference frames
    if (mAcquiredBuffer->referenceFrame) {
        if (mManageReference) {
            // managing reference for MPEG4/H.263/WMV.
            // AVC should manage reference frame in a different way
            if (mForwardReference != NULL) {
                // this foward reference is no longer needed
                mForwardReference->asReferernce = false;
            }
            // Forware reference for either P or B frame prediction
            mForwardReference = mLastReference;
            mAcquiredBuffer->asReferernce = true;
        }

        // the last reference frame.
        mLastReference = mAcquiredBuffer;
    }
    // add to the output list
    if (mOutputHead == NULL) {
        mOutputHead = mAcquiredBuffer;
    } else {
        mOutputTail->next = mAcquiredBuffer;
    }
    mOutputTail = mAcquiredBuffer;
    mOutputTail->next = NULL;

    //VTRACE("Pushing POC %d to queue (pts = %.2f)", mAcquiredBuffer->pictureOrder, mAcquiredBuffer->renderBuffer.timeStamp/1E6);

    mAcquiredBuffer = NULL;
    mSurfaceAcquirePos = (mSurfaceAcquirePos  + 1 ) % mNumSurfaces;
    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderBase::releaseSurfaceBuffer(void) {
    if (mAcquiredBuffer == NULL) {
        // this is harmless error
        return DECODE_SUCCESS;
    }

    // frame is not decoded to the acquired buffer, current surface is invalid, and can't be output.
    mAcquiredBuffer->asReferernce = false;
    mAcquiredBuffer->renderBuffer.renderDone = true;
    mAcquiredBuffer = NULL;
    return DECODE_SUCCESS;
}

void VideoDecoderBase::flushSurfaceBuffers(void) {
    endDecodingFrame(true);
    VideoSurfaceBuffer *p = NULL;
    while (mOutputHead) {
        mOutputHead->renderBuffer.renderDone = true;
        p = mOutputHead;
        mOutputHead = mOutputHead->next;
        p->next = NULL;
    }
    mOutputHead = NULL;
    mOutputTail = NULL;
}

Decode_Status VideoDecoderBase::endDecodingFrame(bool dropFrame) {
    Decode_Status status = DECODE_SUCCESS;
    VAStatus vaStatus;

    if (mDecodingFrame == false) {
        if (mAcquiredBuffer != NULL) {
            //ETRACE("mAcquiredBuffer is not NULL. Implementation bug.");
            releaseSurfaceBuffer();
            status = DECODE_FAIL;
        }
        return status;
    }
    // return through exit label to reset mDecodingFrame
    if (mAcquiredBuffer == NULL) {
        ETRACE("mAcquiredBuffer is NULL. Implementation bug.");
        status = DECODE_FAIL;
        goto exit;
    }

    vaStatus = vaEndPicture(mVADisplay, mVAContext);
    if (vaStatus != VA_STATUS_SUCCESS) {
        releaseSurfaceBuffer();
        ETRACE("vaEndPicture failed. vaStatus = %d", vaStatus);
        status = DECODE_DRIVER_FAIL;
        goto exit;
    }

    if (dropFrame) {
        // we are asked to drop this decoded picture
        LOGW("Frame dropped");
        releaseSurfaceBuffer();
        goto exit;
    }

    status = outputSurfaceBuffer();
    // fall through
exit:
    mDecodingFrame = false;
    return status;
}


Decode_Status VideoDecoderBase::setupVA(int32_t numSurface, VAProfile profile) {
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    Decode_Status status;
    VAConfigAttrib attrib;

    if (mVAStarted) {
        return DECODE_SUCCESS;
    }

    if (mConfigBuffer.flag & USE_NATIVE_GRAPHIC_BUFFER){
       numSurface = mConfigBuffer.surfaceNumber;
    }
    // TODO: validate profile
    if (numSurface == 0) {
        return DECODE_FAIL;
    }

    if (mConfigBuffer.flag & HAS_MINIMUM_SURFACE_NUMBER) {
        if (numSurface < mConfigBuffer.surfaceNumber) {
            WTRACE("surface to allocated %d is less than minimum number required %d",
                    numSurface, mConfigBuffer.surfaceNumber);
            numSurface = mConfigBuffer.surfaceNumber;
        }
    }

    if (mVADisplay != NULL) {
        ETRACE("VA is partially started.");
        return DECODE_FAIL;
    }

    // Display is defined as "unsigned int"

    mDisplay = new Display;
    *mDisplay = ANDROID_DISPLAY_HANDLE;

    mVADisplay = vaGetDisplay(mDisplay);
    if (mVADisplay == NULL) {
        ETRACE("vaGetDisplay failed.");
        return DECODE_DRIVER_FAIL;
    }

    int majorVersion, minorVersion;
    vaStatus = vaInitialize(mVADisplay, &majorVersion, &minorVersion);
    CHECK_VA_STATUS("vaInitialize");

    if ((int32_t)profile != VAProfileSoftwareDecoding) {
        //We are requesting RT attributes
        attrib.type = VAConfigAttribRTFormat;
        attrib.value = VA_RT_FORMAT_YUV420;

        vaStatus = vaCreateConfig(
                mVADisplay,
                profile,
                VAEntrypointVLD,
                &attrib,
                1,
                &mVAConfig);
        CHECK_VA_STATUS("vaCreateConfig");
    }

    mNumSurfaces = numSurface;
    mSurfaces = new VASurfaceID [mNumSurfaces];
    if (mSurfaces == NULL) {
        return DECODE_MEMORY_FAIL;
    }

    int32_t format = VA_RT_FORMAT_YUV420;
    if (mConfigBuffer.flag & WANT_SURFACE_PROTECTION) {
        format |= VA_RT_FORMAT_PROTECTED;
        WTRACE("Surface is protected.");
    }
    if (mConfigBuffer.flag & USE_NATIVE_GRAPHIC_BUFFER) {
        mVASurfaceAttrib = new VASurfaceAttrib;
        if (mVASurfaceAttrib == NULL) {
            return DECODE_MEMORY_FAIL;
        }
        mVAExternalMemoryBuffers = new VAExternalMemoryBuffers;
        if (mVAExternalMemoryBuffers == NULL) {
            return DECODE_MEMORY_FAIL;
        }
        mVAExternalMemoryBuffers->buffers= (unsigned int *)malloc(sizeof(unsigned int)*mNumSurfaces);
        if (mVAExternalMemoryBuffers->buffers == NULL) {
            return DECODE_MEMORY_FAIL;
        }
        mVAExternalMemoryBuffers->count = mNumSurfaces;
        mVAExternalMemoryBuffers->luma_stride = mConfigBuffer.graphicBufferStride;
        mVAExternalMemoryBuffers->pixel_format = mConfigBuffer.graphicBufferColorFormat;
        mVAExternalMemoryBuffers->native_window = mConfigBuffer.nativeWindow;
        mVAExternalMemoryBuffers->width = mVideoFormatInfo.width;
        mVAExternalMemoryBuffers->height = mVideoFormatInfo.height;
        mVAExternalMemoryBuffers->type = VAExternalMemoryAndroidGrallocBuffer;
        for (int i = 0; i < mNumSurfaces; i++) {
            mVAExternalMemoryBuffers->buffers[i] = (unsigned int )mConfigBuffer.graphicBufferHandler[i];
        }
        mVASurfaceAttrib->flags = VA_SURFACE_ATTRIB_SETTABLE;
        mVASurfaceAttrib->type = VASurfaceAttribNativeHandle;
        mVASurfaceAttrib->value.type = VAGenericValueTypePointer;
        mVASurfaceAttrib->value.value.p_val = (void *)mVAExternalMemoryBuffers;
        vaStatus = vaCreateSurfaces(
            mVADisplay,
            mVideoFormatInfo.width,
            mVideoFormatInfo.height,
            format,
            mNumSurfaces,
            mSurfaces,
            mVASurfaceAttrib,
            1);
    } else {
        vaStatus = vaCreateSurfaces(
            mVADisplay,
            mVideoFormatInfo.width,
            mVideoFormatInfo.height,
            format,
            mNumSurfaces,
            mSurfaces,
            NULL,
            0);
    }
    CHECK_VA_STATUS("vaCreateSurfaces");

    mVideoFormatInfo.surfaceWidth = mVideoFormatInfo.width;
    mVideoFormatInfo.surfaceHeight = mVideoFormatInfo.height;
    mVideoFormatInfo.surfaceNumber = mNumSurfaces;
    mVideoFormatInfo.ctxSurfaces = mSurfaces;

    if ((int32_t)profile != VAProfileSoftwareDecoding) {
        vaStatus = vaCreateContext(
                mVADisplay,
                mVAConfig,
                mVideoFormatInfo.width,
                mVideoFormatInfo.height,
                0,
                mSurfaces,
                mNumSurfaces,
                &mVAContext);
        CHECK_VA_STATUS("vaCreateContext");
    }

    mSurfaceBuffers = new VideoSurfaceBuffer [mNumSurfaces];
    if (mSurfaceBuffers == NULL) {
        return DECODE_MEMORY_FAIL;
    }

    initSurfaceBuffer(true);

    if ((int32_t)profile == VAProfileSoftwareDecoding) {
        // derive user pointer from surface for direct access
        status = mapSurface();
        CHECK_STATUS("mapSurface")
    }

    mVAStarted = true;
    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderBase::terminateVA(void) {
    if (mSurfaceBuffers) {
        for (int32_t i = 0; i < mNumSurfaces; i++) {
            if (mSurfaceBuffers[i].renderBuffer.rawData) {
                if (mSurfaceBuffers[i].renderBuffer.rawData->data) {
                    delete [] mSurfaceBuffers[i].renderBuffer.rawData->data;
                }
                delete mSurfaceBuffers[i].renderBuffer.rawData;
            }
            if (mSurfaceBuffers[i].mappedData) {
                // don't  delete data pointer as it is mapped from surface
                delete mSurfaceBuffers[i].mappedData;
            }
        }
        delete [] mSurfaceBuffers;
        mSurfaceBuffers = NULL;
    }


    if (mVAExternalMemoryBuffers) {
        if (mVAExternalMemoryBuffers->buffers) {
            free(mVAExternalMemoryBuffers->buffers);
            mVAExternalMemoryBuffers->buffers = NULL;
        }
        delete mVAExternalMemoryBuffers;
        mVAExternalMemoryBuffers = NULL;
    }

    if (mVASurfaceAttrib) {
        delete mVASurfaceAttrib;
        mVASurfaceAttrib = NULL;
    }


    if (mSurfaceUserPtr) {
        delete [] mSurfaceUserPtr;
        mSurfaceUserPtr = NULL;
    }

    if (mSurfaces)
    {
        vaDestroySurfaces(mVADisplay, mSurfaces, mNumSurfaces);
        delete [] mSurfaces;
        mSurfaces = NULL;
    }

    if (mVAContext != VA_INVALID_ID) {
         vaDestroyContext(mVADisplay, mVAContext);
         mVAContext = VA_INVALID_ID;
    }

    if (mVAConfig != VA_INVALID_ID) {
        vaDestroyConfig(mVADisplay, mVAConfig);
        mVAConfig = VA_INVALID_ID;
    }

    if (mVADisplay) {
        vaTerminate(mVADisplay);
        mVADisplay = NULL;
    }

    if (mDisplay) {
        delete mDisplay;
        mDisplay = NULL;
    }

    mVAStarted = false;
    mInitialized = false;
    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderBase::parseBuffer(uint8_t *buffer, int32_t size, bool config, void** vbpData) {
     // DON'T check if mVAStarted == true
    if (mParserHandle == NULL) {
        return DECODE_NO_PARSER;
    }

    uint32_t vbpStatus;
    if (buffer == NULL || size <= 0) {
        return DECODE_INVALID_DATA;
    }

    uint8_t configFlag = config ? 1 : 0;
    vbpStatus = vbp_parse(mParserHandle, buffer, size, configFlag);
    CHECK_VBP_STATUS("vbp_parse");

    vbpStatus = vbp_query(mParserHandle, vbpData);
    CHECK_VBP_STATUS("vbp_query");

    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderBase::mapSurface(void){
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAImage image;
    uint8_t *userPtr;
    mSurfaceUserPtr = new uint8_t* [mNumSurfaces];
    if (mSurfaceUserPtr == NULL) {
        return DECODE_MEMORY_FAIL;
    }

    for (int32_t i = 0; i< mNumSurfaces; i++) {
        vaStatus = vaDeriveImage(mVADisplay, mSurfaces[i], &image);
        CHECK_VA_STATUS("vaDeriveImage");
        vaStatus = vaMapBuffer(mVADisplay, image.buf, (void**)&userPtr);
        CHECK_VA_STATUS("vaMapBuffer");
        mSurfaceUserPtr[i] = userPtr;
        mSurfaceBuffers[i].mappedData = new VideoFrameRawData;
        if (mSurfaceBuffers[i].mappedData == NULL) {
            return DECODE_MEMORY_FAIL;
        }
        mSurfaceBuffers[i].mappedData->own = false; // derived from surface so can't be released
        mSurfaceBuffers[i].mappedData->data = NULL;  // specified during acquireSurfaceBuffer
        mSurfaceBuffers[i].mappedData->fourcc = image.format.fourcc;
        mSurfaceBuffers[i].mappedData->width = mVideoFormatInfo.width;
        mSurfaceBuffers[i].mappedData->height = mVideoFormatInfo.height;
        mSurfaceBuffers[i].mappedData->size = image.data_size;
        for (int pi = 0; pi < 3; pi++) {
            mSurfaceBuffers[i].mappedData->pitch[pi] = image.pitches[pi];
            mSurfaceBuffers[i].mappedData->offset[pi] = image.offsets[pi];
        }
        // debug information
        if (image.pitches[0] != image.pitches[1] ||
            image.width != mVideoFormatInfo.width ||
            image.height != mVideoFormatInfo.height ||
            image.offsets[0] != 0) {
            WTRACE("Unexpected VAImage format, w = %d, h = %d, offset = %d", image.width, image.height, image.offsets[0]);
        }
        // TODO: do we need to unmap buffer?
        //vaStatus = vaUnmapBuffer(mVADisplay, image.buf);
        //CHECK_VA_STATUS("vaMapBuffer");
        vaStatus = vaDestroyImage(mVADisplay,image.image_id);
        CHECK_VA_STATUS("vaDestroyImage");

    }
    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderBase::getRawDataFromSurface(void) {
    if (mAcquiredBuffer == NULL) {
        return DECODE_FAIL;
    }

    VAStatus vaStatus;
    VAImageFormat imageFormat;
    VAImage vaImage;
    vaStatus = vaSyncSurface(mVADisplay, mAcquiredBuffer->renderBuffer.surface);
    CHECK_VA_STATUS("vaSyncSurface");

    vaImage.image_id = VA_INVALID_ID;
    // driver currently only supports NV12 and IYUV format.
    // byte_order information is from driver  and hard-coded here
    imageFormat.fourcc = VA_FOURCC_NV12;
    imageFormat.byte_order = VA_LSB_FIRST;
    imageFormat.bits_per_pixel = 16;
    vaStatus = vaCreateImage(
        mVADisplay,
        &imageFormat,
        mVideoFormatInfo.width,
        mVideoFormatInfo.height,
        &vaImage);
    CHECK_VA_STATUS("vaCreateImage");

    vaStatus = vaGetImage(
        mVADisplay,
        mAcquiredBuffer->renderBuffer.surface,
        0,
        0,
        vaImage.width,
        vaImage.height,
        vaImage.image_id);
    CHECK_VA_STATUS("vaGetImage");

    void *pBuf = NULL;
    vaStatus = vaMapBuffer(mVADisplay, vaImage.buf, &pBuf);
    CHECK_VA_STATUS("vaMapBuffer");

    VideoFrameRawData *rawData = NULL;
    if (mAcquiredBuffer->renderBuffer.rawData == NULL) {
        rawData = new VideoFrameRawData;
        if (rawData == NULL) {
            return DECODE_MEMORY_FAIL;
        }
        memset(rawData, 0, sizeof(VideoFrameRawData));
        mAcquiredBuffer->renderBuffer.rawData = rawData;
    } else {
        rawData = mAcquiredBuffer->renderBuffer.rawData;
    }

    // size in NV12 format
    uint32_t cropWidth = mVideoFormatInfo.width - (mVideoFormatInfo.cropLeft + mVideoFormatInfo.cropRight);
    uint32_t cropHeight = mVideoFormatInfo.height - (mVideoFormatInfo.cropBottom + mVideoFormatInfo.cropTop);
    int32_t size = cropWidth  * cropHeight * 3 / 2;

    if (rawData->data != NULL && rawData->size != size) {
        delete [] rawData->data;
        rawData->data = NULL;
        rawData->size = 0;
    }
    if (rawData->data == NULL) {
        rawData->data = new uint8_t [size];
        if (rawData->data == NULL) {
            return DECODE_MEMORY_FAIL;
        }
    }
    rawData->own = true; // allocated by this library
    rawData->width = cropWidth;
    rawData->height = cropHeight;
    rawData->pitch[0] = cropWidth;
    rawData->pitch[1] = cropWidth;
    rawData->pitch[2] = 0;  // interleaved U/V, two planes
    rawData->offset[0] = 0;
    rawData->offset[1] = cropWidth * cropHeight;
    rawData->offset[2] = cropWidth * cropHeight * 3 / 2;
    rawData->size = size;
    rawData->fourcc = 'NV12';
    if (size == (int32_t)vaImage.data_size) {
        memcpy(rawData->data, pBuf, size);
    } else {
        // copy Y data
        uint8_t *src = (uint8_t*)pBuf;
        uint8_t *dst = rawData->data;
        int32_t row = 0;
        for (row = 0; row < cropHeight; row++) {
            memcpy(dst, src, cropWidth);
            dst += cropWidth;
            src += vaImage.pitches[0];
        }
        // copy interleaved V and  U data
        src = (uint8_t*)pBuf + vaImage.offsets[1];
        for (row = 0; row < cropHeight / 2; row++) {
            memcpy(dst, src, cropWidth);
            dst += cropWidth;
            src += vaImage.pitches[1];
        }
    }
    // TODO: image may not get destroyed if error happens.
    if (vaImage.image_id != VA_INVALID_ID) {
        vaDestroyImage(mVADisplay, vaImage.image_id);
    }
    return DECODE_SUCCESS;
}

void VideoDecoderBase::initSurfaceBuffer(bool reset) {
    bool useGraphicBuffer = mConfigBuffer.flag & USE_NATIVE_GRAPHIC_BUFFER;
    if (useGraphicBuffer && reset) {
        pthread_mutex_lock(&mLock);
    }
    for (int32_t i = 0; i < mNumSurfaces; i++) {
        mSurfaceBuffers[i].renderBuffer.display = mVADisplay;
        mSurfaceBuffers[i].renderBuffer.surface = VA_INVALID_SURFACE;  // set in acquireSurfaceBuffer
        mSurfaceBuffers[i].renderBuffer.flag = 0;
        mSurfaceBuffers[i].renderBuffer.scanFormat = VA_FRAME_PICTURE;
        mSurfaceBuffers[i].renderBuffer.timeStamp = 0;
        mSurfaceBuffers[i].referenceFrame = false;
        mSurfaceBuffers[i].asReferernce= false;
        mSurfaceBuffers[i].pictureOrder = 0;
        mSurfaceBuffers[i].next = NULL;
        if (reset == true) {
            mSurfaceBuffers[i].renderBuffer.rawData = NULL;
            mSurfaceBuffers[i].mappedData = NULL;
        }
        if (useGraphicBuffer){
            if (reset) {
               mSurfaceBuffers[i].renderBuffer.graphicBufferHandle = mConfigBuffer.graphicBufferHandler[i];
               mSurfaceBuffers[i].renderBuffer.renderDone = false; //default false
               for (int j = 0; j < mSignalBufferSize; j++) {
                   if(mSignalBufferPre[j] != NULL && mSignalBufferPre[j] == mSurfaceBuffers[i].renderBuffer.graphicBufferHandle) {
                      mSurfaceBuffers[i].renderBuffer.renderDone = true;
                      VTRACE("initSurfaceBuffer set renderDone = true index = %d", i);
                      mSignalBufferPre[j] = NULL;
                      break;
                   }
               }
            }
            else{
               mSurfaceBuffers[i].renderBuffer.renderDone = false;
            }

        } else {
            mSurfaceBuffers[i].renderBuffer.graphicBufferHandle = NULL;
            mSurfaceBuffers[i].renderBuffer.renderDone = true;
        }
        mSurfaceBuffers[i].renderBuffer.acquirePos = i;
    }

    if (useGraphicBuffer && reset) {
        mInitialized = true;
        mSignalBufferSize = 0;
        pthread_mutex_unlock(&mLock);
    }
}

Decode_Status VideoDecoderBase::signalRenderDone(void * graphichandler) {
    if (graphichandler == NULL) {
        return DECODE_SUCCESS;
    }
    pthread_mutex_lock(&mLock);
    int i = 0;
    if (!mInitialized) {
        mSignalBufferPre[mSignalBufferSize++] = graphichandler;
        VTRACE("SignalRenderDoneFlag mInitialized = false graphichandler = %p, mSignalBufferSize = %d", graphichandler, mSignalBufferSize);
        if (mSignalBufferSize > MAX_GRAPHIC_NUM)
            return DECODE_INVALID_DATA;
    } else {
        if (!(mConfigBuffer.flag & USE_NATIVE_GRAPHIC_BUFFER)) {
            return DECODE_SUCCESS;
        }
        for (i = 0; i < mNumSurfaces; i++) {
            if (mSurfaceBuffers[i].renderBuffer.graphicBufferHandle == graphichandler) {
                mSurfaceBuffers[i].renderBuffer.renderDone = true;
                VTRACE("SignalRenderDoneFlag mInitialized = true index = %d", i);
               break;
           }
        }
    }
    pthread_mutex_unlock(&mLock);

    return DECODE_SUCCESS;

}


