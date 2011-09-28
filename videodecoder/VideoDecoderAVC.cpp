/* INTEL CONFIDENTIAL
* Copyright (c) 2009 Intel Corporation.  All rights reserved.
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

#include "VideoDecoderAVC.h"
#include "VideoDecoderTrace.h"
#include <string.h>

VideoDecoderAVC::VideoDecoderAVC(const char *mimeType)
    : VideoDecoderBase(mimeType, VBP_H264),
      mToggleDPB(0),
      mErrorConcealment(false){

    invalidateDPB(0);
    invalidateDPB(1);
    mLastPictureFlags = VA_PICTURE_H264_INVALID;
}

VideoDecoderAVC::~VideoDecoderAVC() {
    stop();
}

Decode_Status VideoDecoderAVC::start(VideoConfigBuffer *buffer) {
    Decode_Status status;

    status = VideoDecoderBase::start(buffer);
    CHECK_STATUS("VideoDecoderBase::start");

    // We don't want base class to manage reference.
    VideoDecoderBase::ManageReference(false);
    // output by picture order count
    VideoDecoderBase::setOutputMethod(OUTPUT_BY_POC);

    mErrorConcealment = buffer->flag & WANT_ERROR_CONCEALMENT;
    if (buffer->data == NULL || buffer->size == 0) {
        WTRACE("No config data to start VA.");
        if ((buffer->flag & HAS_SURFACE_NUMBER) && (buffer->flag & HAS_VA_PROFILE)) {
            ITRACE("Used client supplied profile and surface to start VA.");
            return VideoDecoderBase::setupVA(buffer->surfaceNumber, buffer->profile);
        }
        return DECODE_SUCCESS;
    }

    vbp_data_h264 *data = NULL;
    status = VideoDecoderBase::parseBuffer(buffer->data, buffer->size, true, (void**)&data);
    CHECK_STATUS("VideoDecoderBase::parseBuffer");

    status = startVA(data);
    return status;
}

void VideoDecoderAVC::stop(void) {
    // drop the last  frame and ignore return value
    endDecodingFrame(true);
    VideoDecoderBase::stop();
    invalidateDPB(0);
    invalidateDPB(1);
    mToggleDPB = 0;
    mErrorConcealment = false;
    mLastPictureFlags = VA_PICTURE_H264_INVALID;
}

void VideoDecoderAVC::flush(void) {
    // drop the frame and ignore return value
    VideoDecoderBase::flush();
    invalidateDPB(0);
    invalidateDPB(1);
    mToggleDPB = 0;
    mLastPictureFlags = VA_PICTURE_H264_INVALID;
}

Decode_Status VideoDecoderAVC::decode(VideoDecodeBuffer *buffer) {
    Decode_Status status;
    vbp_data_h264 *data = NULL;
    if (buffer == NULL) {
        return DECODE_INVALID_DATA;
    }
    status =  VideoDecoderBase::parseBuffer(
            buffer->data,
            buffer->size,
            false,
            (void**)&data);
    CHECK_STATUS("VideoDecoderBase::parseBuffer");

    if (!mVAStarted) {
         if (data->has_sps && data->has_pps) {
            status = startVA(data);
            CHECK_STATUS("startVA");
        } else {
            WTRACE("Can't start VA as either SPS or PPS is still not available.");
            return DECODE_SUCCESS;
        }
    }

    status = decodeFrame(buffer, data);
    return status;
}

Decode_Status VideoDecoderAVC::decodeFrame(VideoDecodeBuffer *buffer, vbp_data_h264 *data) {
    Decode_Status status;
    if (data->has_sps == 0 || data->has_pps == 0) {
        return DECODE_NO_CONFIG;
    }

    // Don't remove the following codes, it can be enabled for debugging DPB.
#if 0
    for (unsigned int i = 0; i < data->num_pictures; i++) {
        VAPictureH264 &pic = data->pic_data[i].pic_parms->CurrPic;
        VTRACE("%d: decoding frame %.2f, poc top = %d, poc bottom = %d, flags = %d,  reference = %d",
                i,
                buffer->timeStamp/1E6,
                pic.TopFieldOrderCnt,
                pic.BottomFieldOrderCnt,
                pic.flags,
                (pic.flags & VA_PICTURE_H264_SHORT_TERM_REFERENCE) ||
                (pic.flags & VA_PICTURE_H264_LONG_TERM_REFERENCE));
    }
#endif
    if (data->new_sps) {
        status = handleNewSequence(data);
        CHECK_STATUS("handleNewSequence");
    }

    // first pic_data always exists, check if any slice is parsed
    if (data->pic_data[0].num_slices == 0) {
        ITRACE("No slice available for decoding.");
        return DECODE_SUCCESS;
    }

    uint64_t lastPTS = mCurrentPTS;
    mCurrentPTS = buffer->timeStamp;

    //if (lastPTS != mCurrentPTS) {
    if (isNewFrame(data)) {
        // finish decoding the last frame
        status = endDecodingFrame(false);
        CHECK_STATUS("endDecodingFrame");

        // start decoding a new frame
        status = beginDecodingFrame(data);
        CHECK_STATUS("beginDecodingFrame");
    } else {
        status = continueDecodingFrame(data);
        CHECK_STATUS("continueDecodingFrame");
    }

    // HAS_COMPLETE_FRAME is not reliable as it may indicate end of a field
  /*  if (buffer->flag & HAS_COMPLETE_FRAME) {
        // finish decoding current frame
        status = endDecodingFrame(false);
        CHECK_STATUS("endDecodingFrame");
    }*/

    if (mSizeChanged) {
        mSizeChanged = false;
        return DECODE_FORMAT_CHANGE;
    }
    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderAVC::beginDecodingFrame(vbp_data_h264 *data) {
    Decode_Status status;

    status = acquireSurfaceBuffer();
    CHECK_STATUS("acquireSurfaceBuffer");
    VAPictureH264 *picture = &(data->pic_data[0].pic_parms->CurrPic);
    if ((picture->flags  & VA_PICTURE_H264_SHORT_TERM_REFERENCE) ||
        (picture->flags & VA_PICTURE_H264_LONG_TERM_REFERENCE)) {
        mAcquiredBuffer->referenceFrame = true;
    } else {
        mAcquiredBuffer->referenceFrame = false;
    }
    // set asReference in updateDPB

    if (picture->flags & VA_PICTURE_H264_TOP_FIELD) {
        mAcquiredBuffer->renderBuffer.scanFormat = VA_BOTTOM_FIELD | VA_TOP_FIELD;
    } else {
        mAcquiredBuffer->renderBuffer.scanFormat = VA_FRAME_PICTURE;
    }

    // TODO: Set the discontinuity flag
    mAcquiredBuffer->renderBuffer.flag = 0;
    mAcquiredBuffer->renderBuffer.timeStamp = mCurrentPTS;
    mAcquiredBuffer->pictureOrder = getPOC(picture);

    status  = continueDecodingFrame(data);
    // surface buffer is released if decode fails
    return status;
}


Decode_Status VideoDecoderAVC::continueDecodingFrame(vbp_data_h264 *data) {
    Decode_Status status;
    vbp_picture_data_h264 *picData = data->pic_data;

    // TODO: remove these debugging codes
    if (mAcquiredBuffer == NULL || mAcquiredBuffer->renderBuffer.surface == VA_INVALID_SURFACE) {
        ETRACE("mAcquiredBuffer is NULL. Implementation bug.");
        return DECODE_FAIL;
    }
    for (uint32_t picIndex = 0; picIndex < data->num_pictures; picIndex++, picData++) {
        // sanity check
        if (picData == NULL || picData->pic_parms == NULL || picData->slc_data == NULL || picData->num_slices == 0) {
            return DECODE_PARSER_FAIL;
        }

        for (uint32_t sliceIndex = 0; sliceIndex < picData->num_slices; sliceIndex++) {
            status = decodeSlice(data, picIndex, sliceIndex);
            if (status != DECODE_SUCCESS) {
                endDecodingFrame(true);
                // TODO: this is new code
                // remove current frame from DPB as it can't be decoded.
                removeReferenceFromDPB(picData->pic_parms);
                return status;
            }
        }
    }
    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderAVC::decodeSlice(vbp_data_h264 *data, uint32_t picIndex, uint32_t sliceIndex) {
    Decode_Status status;
    VAStatus vaStatus;
    uint32_t bufferIDCount = 0;
    // maximum 4 buffers to render a slice: picture parameter, IQMatrix, slice parameter, slice data
    VABufferID bufferIDs[4];

    vbp_picture_data_h264 *picData = &(data->pic_data[picIndex]);
    vbp_slice_data_h264 *sliceData = &(picData->slc_data[sliceIndex]);
    VAPictureParameterBufferH264 *picParam = picData->pic_parms;
    VASliceParameterBufferH264 *sliceParam = &(sliceData->slc_parms);

    if (sliceParam->first_mb_in_slice == 0 || mDecodingFrame == false) {
        // either condition indicates start of a new frame
        if (sliceParam->first_mb_in_slice != 0) {
            WTRACE("The first slice is lost.");
            // TODO: handle the first slice lost
        }
        if (mDecodingFrame) {
            // interlace content, complete decoding the first field
            vaStatus = vaEndPicture(mVADisplay, mVAContext);
            CHECK_VA_STATUS("vaEndPicture");

            // for interlace content, top field may be valid only after the second field is parsed
            int32_t poc = getPOC(&(picParam->CurrPic));
            if (poc < mAcquiredBuffer->pictureOrder) {
                mAcquiredBuffer->pictureOrder = poc;
            }
        }

        // Check there is no reference frame loss before decoding a frame

        // Update  the reference frames and surface IDs for DPB and current frame
        status = updateDPB(picParam);
        CHECK_STATUS("updateDPB");

        //We have to provide a hacked DPB rather than complete DPB for libva as workaround
        status = updateReferenceFrames(picData);
        CHECK_STATUS("updateReferenceFrames");

        vaStatus = vaBeginPicture(mVADisplay, mVAContext, mAcquiredBuffer->renderBuffer.surface);
        CHECK_VA_STATUS("vaBeginPicture");

        // start decoding a frame
        mDecodingFrame = true;

        vaStatus = vaCreateBuffer(
            mVADisplay,
            mVAContext,
            VAPictureParameterBufferType,
            sizeof(VAPictureParameterBufferH264),
            1,
            picParam,
            &bufferIDs[bufferIDCount]);
        CHECK_VA_STATUS("vaCreatePictureParameterBuffer");
        bufferIDCount++;

        vaStatus = vaCreateBuffer(
            mVADisplay,
            mVAContext,
            VAIQMatrixBufferType,
            sizeof(VAIQMatrixBufferH264),
            1,
            data->IQ_matrix_buf,
            &bufferIDs[bufferIDCount]);
        CHECK_VA_STATUS("vaCreateIQMatrixBuffer");
        bufferIDCount++;
    }

    status = setReference(sliceParam);
    CHECK_STATUS("setReference");

    vaStatus = vaCreateBuffer(
        mVADisplay,
        mVAContext,
        VASliceParameterBufferType,
        sizeof(VASliceParameterBufferH264),
        1,
        sliceParam,
        &bufferIDs[bufferIDCount]);
    CHECK_VA_STATUS("vaCreateSliceParameterBuffer");
    bufferIDCount++;

    vaStatus = vaCreateBuffer(
        mVADisplay,
        mVAContext,
        VASliceDataBufferType,
        sliceData->slice_size, //size
        1,        //num_elements
        sliceData->buffer_addr + sliceData->slice_offset,
        &bufferIDs[bufferIDCount]);
    CHECK_VA_STATUS("vaCreateSliceDataBuffer");
    bufferIDCount++;

    vaStatus = vaRenderPicture(
        mVADisplay,
        mVAContext,
        bufferIDs,
        bufferIDCount);
    CHECK_VA_STATUS("vaRenderPicture");

    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderAVC::setReference(VASliceParameterBufferH264 *sliceParam) {
    int32_t numList = 1;
    // TODO: set numList to 0 if it is I slice
    if (sliceParam->slice_type == 1 || sliceParam->slice_type == 6) {
        // B slice
        numList = 2;
    }

    int32_t activeMinus1 = sliceParam->num_ref_idx_l0_active_minus1;
    VAPictureH264 *ref = sliceParam->RefPicList0;

    for (int32_t i = 0; i < numList; i++) {
        if (activeMinus1 >= REF_LIST_SIZE) {
            ETRACE("Invalid activeMinus1 (%d)", activeMinus1);
            return DECODE_PARSER_FAIL;
        }
        for (int32_t j = 0; j <= activeMinus1; j++, ref++) {
            if (!(ref->flags & VA_PICTURE_H264_INVALID)) {
                ref->picture_id = findSurface(ref);
                if (ref->picture_id == VA_INVALID_SURFACE) {
                    if (mLastReference) {
                        WTRACE("Reference frame %d is missing. Use last reference", getPOC(ref));
                        ref->picture_id = mLastReference->renderBuffer.surface;
                    } else {
                        ETRACE("Reference frame %d is missing. Stop decoding.", getPOC(ref));
                        return DECODE_NO_REFERENCE;
                    }
                }
            }
        }
        activeMinus1 = sliceParam->num_ref_idx_l1_active_minus1;
        ref = sliceParam->RefPicList1;
    }
    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderAVC::updateDPB(VAPictureParameterBufferH264 *picParam) {
    clearAsReference(mToggleDPB);
    // pointer to toggled DPB (new)
    DecodedPictureBuffer *dpb = mDPBs[!mToggleDPB];
    VAPictureH264 *ref = picParam->ReferenceFrames;

    // update current picture ID
    picParam->CurrPic.picture_id = mAcquiredBuffer->renderBuffer.surface;

    // build new DPB
    for (int32_t i = 0; i < MAX_REF_NUMBER; i++, ref++) {
        if (ref->flags & VA_PICTURE_H264_INVALID) {
            continue;
        }
        dpb->poc = getPOC(ref);
        dpb->surfaceBuffer = findSurfaceBuffer(ref);
        if (dpb->surfaceBuffer == NULL) {
            ETRACE("Reference frame %d is missing for current frame %d", dpb->poc, getPOC(&(picParam->CurrPic)));
            if (dpb->poc == getPOC(&(picParam->CurrPic))) {
                WTRACE("updateDPB: Using the current picture for missing reference.");
                dpb->surfaceBuffer = mAcquiredBuffer;
            } else if (mLastReference) {
                WTRACE("updateDPB: Use last reference frame %d for missing reference.", mLastReference->pictureOrder);
                // TODO: this is new code for error resilience
                dpb->surfaceBuffer = mLastReference;
            } else {
                WTRACE("updateDPB: Unable to recover the missing reference frame.");
                // continue buillding DPB without updating dpb pointer.
                continue;
                // continue building DPB as this reference may not be actually used.
                // especially happen after seeking to a non-IDR I frame.
                //return DECODE_NO_REFERENCE;
            }
        }
        if (dpb->surfaceBuffer) {
            // this surface is used as reference
            dpb->surfaceBuffer->asReferernce = true;
        }
        dpb++;
    }

    // add current frame to DPB if it  is a reference frame
    if ((picParam->CurrPic.flags & VA_PICTURE_H264_SHORT_TERM_REFERENCE) ||
        (picParam->CurrPic.flags & VA_PICTURE_H264_LONG_TERM_REFERENCE)) {
        dpb->poc = getPOC(&(picParam->CurrPic));
        dpb->surfaceBuffer = mAcquiredBuffer;
        dpb->surfaceBuffer->asReferernce = true;
    }
    // invalidate the current used DPB
    invalidateDPB(mToggleDPB);
    mToggleDPB = !mToggleDPB;
    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderAVC::updateReferenceFrames(vbp_picture_data_h264 *picData) {
    bool found = false;
    uint32_t flags = 0;
    VAPictureParameterBufferH264 *picParam = picData->pic_parms;
    VASliceParameterBufferH264 *sliceParam = NULL;
    uint8_t activeMinus1 = 0;
    VAPictureH264 *refList = NULL;
    VAPictureH264 *dpb = picParam->ReferenceFrames;
    VAPictureH264 *refFrame = NULL;

    // invalidate DPB in the picture buffer
    memset(picParam->ReferenceFrames, 0xFF, sizeof(picParam->ReferenceFrames));
    picParam->num_ref_frames = 0;

    // update DPB  from the reference list in each slice.
    for (uint32_t slice = 0; slice < picData->num_slices; slice++) {
        sliceParam = &(picData->slc_data[slice].slc_parms);
        for (int32_t list = 0; list < 2; list++) {
            refList = (list == 0) ? sliceParam->RefPicList0 :
                                    sliceParam->RefPicList1;
            activeMinus1 = (list == 0) ? sliceParam->num_ref_idx_l0_active_minus1 :
                                         sliceParam->num_ref_idx_l1_active_minus1;
            if (activeMinus1 >= REF_LIST_SIZE) {
                return DECODE_PARSER_FAIL;
            }
            for (uint8_t item = 0; item < (uint8_t)(activeMinus1 + 1); item++, refList++) {
                if (refList->flags & VA_PICTURE_H264_INVALID) {
                    break;
                }
                found = false;
                refFrame = picParam->ReferenceFrames;
                for (uint8_t frame = 0; frame < picParam->num_ref_frames; frame++, refFrame++) {
                    if (refFrame->TopFieldOrderCnt == refList->TopFieldOrderCnt) {
                        ///check for complementary field
                        flags = refFrame->flags | refList->flags;
                        //If both TOP and BOTTOM are set, we'll clear those flags
                        if ((flags & VA_PICTURE_H264_TOP_FIELD) &&
                            (flags & VA_PICTURE_H264_BOTTOM_FIELD)) {
                            refFrame->flags = VA_PICTURE_H264_SHORT_TERM_REFERENCE;
                        }
                        found = true;  //already in the DPB; will not add this one
                        break;
                    }
                }
                if (found == false) {
                    // add a new reference to the DPB
                    dpb->picture_id = findSurface(refList);
                    if (dpb->picture_id == VA_INVALID_SURFACE) {
                        if (mLastReference != NULL) {
                            dpb->picture_id = mLastReference->renderBuffer.surface;
                        } else {
                            ETRACE("Reference frame %d is missing. Stop updating references frames.", getPOC(refList));
                            return DECODE_NO_REFERENCE;
                        }
                    }
                    dpb->flags = refList->flags;
                    dpb->frame_idx = refList->frame_idx;
                    dpb->TopFieldOrderCnt = refList->TopFieldOrderCnt;
                    dpb->BottomFieldOrderCnt = refList->BottomFieldOrderCnt;
                    dpb++;
                    picParam->num_ref_frames++;
                }
            }
        }
    }
    return DECODE_SUCCESS;
}

void VideoDecoderAVC::removeReferenceFromDPB(VAPictureParameterBufferH264 *picParam) {
    // remove the current frame from DPB as it can't be decoded.
    if ((picParam->CurrPic.flags & VA_PICTURE_H264_SHORT_TERM_REFERENCE) ||
        (picParam->CurrPic.flags & VA_PICTURE_H264_LONG_TERM_REFERENCE)) {
        DecodedPictureBuffer *dpb = mDPBs[mToggleDPB];
        uint32_t poc = getPOC(&(picParam->CurrPic));
        for (int32_t i = 0; i < DPB_SIZE; i++, dpb++) {
            if (poc == dpb->poc) {
                dpb->poc = (uint32_t)-1;
                if (dpb->surfaceBuffer) {
                    dpb->surfaceBuffer->asReferernce = false;
                }
                dpb->surfaceBuffer = NULL;
                break;
            }
        }
    }
}

uint32_t VideoDecoderAVC::getPOC(VAPictureH264 *pic) {
    if (pic->flags & VA_PICTURE_H264_BOTTOM_FIELD) {
        return pic->BottomFieldOrderCnt;
    }
    return pic->TopFieldOrderCnt;
}

VASurfaceID VideoDecoderAVC::findSurface(VAPictureH264 *pic) {
    VideoSurfaceBuffer *p = findSurfaceBuffer(pic);
    if (p == NULL) {
        ETRACE("Could not find surface for poc %d", getPOC(pic));
        return VA_INVALID_SURFACE;
    }
    return p->renderBuffer.surface;
}

VideoSurfaceBuffer* VideoDecoderAVC::findSurfaceBuffer(VAPictureH264 *pic) {
    DecodedPictureBuffer *dpb = mDPBs[mToggleDPB];
    for (int32_t i = 0; i < DPB_SIZE; i++, dpb++) {
        if (dpb->poc == pic->BottomFieldOrderCnt ||
            dpb->poc == pic->TopFieldOrderCnt) {
            // TODO: remove these debugging codes
            if (dpb->surfaceBuffer == NULL) {
                ETRACE("Invalid surface buffer in the DPB for poc %d.", getPOC(pic));
            }
            return dpb->surfaceBuffer;
        }
    }
    ETRACE("Unable to find surface for poc %d", getPOC(pic));
    return NULL;
}

void VideoDecoderAVC::invalidateDPB(int toggle) {
    DecodedPictureBuffer* p = mDPBs[toggle];
    for (int i = 0; i < DPB_SIZE; i++) {
        p->poc = (uint32_t) -1;
        p->surfaceBuffer = NULL;
        p++;
    }
}

void VideoDecoderAVC::clearAsReference(int toggle) {
    DecodedPictureBuffer* p = mDPBs[toggle];
    for (int i = 0; i < DPB_SIZE; i++) {
        if (p->surfaceBuffer) {
            p->surfaceBuffer->asReferernce = false;
        }
        p++;
    }
}

Decode_Status VideoDecoderAVC::startVA(vbp_data_h264 *data) {
    int32_t DPBSize = getDPBSize(data);
    updateFormatInfo(data);

    //Use high profile for all kinds of H.264 profiles (baseline, main and high) except for constrained baseline
    VAProfile vaProfile = VAProfileH264High;

    // TODO: determine when to use VAProfileH264ConstrainedBaseline, set only if we are told to do so
    if ((data->codec_data->profile_idc == 66 || data->codec_data->constraint_set0_flag == 1) &&
        data->codec_data->constraint_set1_flag == 1) {
        if (mErrorConcealment) {
            vaProfile = VAProfileH264ConstrainedBaseline;
        }
    }
   // for 1080p, limit the total surface to 19, according the hardware limitation
   // change the max surface number from 19->10 to workaround memory shortage
    if(mVideoFormatInfo.height == 1088 && DPBSize + AVC_EXTRA_SURFACE_NUMBER > 10) {
        DPBSize = 10 - AVC_EXTRA_SURFACE_NUMBER;
    }
    VideoDecoderBase::setOutputWindowSize(DPBSize);
    return VideoDecoderBase::setupVA(DPBSize + AVC_EXTRA_SURFACE_NUMBER, vaProfile);
}

void VideoDecoderAVC::updateFormatInfo(vbp_data_h264 *data) {
    // new video size
    int width = (data->pic_data[0].pic_parms->picture_width_in_mbs_minus1 + 1) * 16;
    int height = (data->pic_data[0].pic_parms->picture_height_in_mbs_minus1 + 1) * 16;
    ITRACE("updateFormatInfo: current size: %d x %d, new size: %d x %d",
        mVideoFormatInfo.width, mVideoFormatInfo.height, width, height);

    if (mVideoFormatInfo.width != width ||
        mVideoFormatInfo.height!= height) {
        mVideoFormatInfo.width = width;
        mVideoFormatInfo.height = height;
        mSizeChanged = true;
        ITRACE("Video size is changed.");
    }

    if (data->new_sps) {
        mSizeChanged = true;
        ITRACE("New sequence is received. Assuming video size is changed.");
    }

    // video_range has default value of 0.
    mVideoFormatInfo.videoRange = data->codec_data->video_full_range_flag;

    switch (data->codec_data->matrix_coefficients) {
        case 1:
            mVideoFormatInfo.colorMatrix = VA_SRC_BT709;
            break;

        // ITU-R Recommendation BT.470-6 System B, G (MP4), same as
        // SMPTE 170M/BT601
        case 5:
        case 6:
            mVideoFormatInfo.colorMatrix = VA_SRC_BT601;
            break;

        default:
            // unknown color matrix, set to 0 so color space flag will not be set.
            mVideoFormatInfo.colorMatrix = 0;
            break;
    }
    mVideoFormatInfo.aspectX = data->codec_data->sar_width;
    mVideoFormatInfo.aspectY = data->codec_data->sar_height;
    mVideoFormatInfo.bitrate = data->codec_data->bit_rate;
    mVideoFormatInfo.cropLeft = data->codec_data->crop_left;
    mVideoFormatInfo.cropRight = data->codec_data->crop_right;
    mVideoFormatInfo.cropTop = data->codec_data->crop_top;
    mVideoFormatInfo.cropBottom = data->codec_data->crop_bottom;

    ITRACE("Cropping: left = %d, top = %d, right = %d, bottom = %d",
        data->codec_data->crop_left,
        data->codec_data->crop_top,
        data->codec_data->crop_right,
        data->codec_data->crop_bottom);

    mVideoFormatInfo.valid = true;
}

Decode_Status VideoDecoderAVC::handleNewSequence(vbp_data_h264 *data) {
    int width = mVideoFormatInfo.width;
    int height = mVideoFormatInfo.height;

    updateFormatInfo(data);
    if (mSizeChanged == false) {
        return DECODE_SUCCESS;
    }

    if (mVideoFormatInfo.width > mVideoFormatInfo.surfaceWidth ||
        mVideoFormatInfo.height > mVideoFormatInfo.surfaceHeight) {
        ETRACE("New video size %d x %d exceeds surface size %d x %d.",
                mVideoFormatInfo.width, mVideoFormatInfo.height,
                mVideoFormatInfo.surfaceWidth, mVideoFormatInfo.surfaceHeight);
        return DECODE_NEED_RESTART;
    }

    if (width == mVideoFormatInfo.width &&
        height == mVideoFormatInfo.height) {
        ITRACE("New video sequence with the same resolution.");
    } else {
        WTRACE("Video size changed from %d x %d to %d x %d.", width, height,
                mVideoFormatInfo.width, mVideoFormatInfo.height);
        flush();
    }
    return DECODE_SUCCESS;
}

bool VideoDecoderAVC::isNewFrame(vbp_data_h264 *data) {
    if (data->num_pictures == 0) {
        ETRACE("num_pictures == 0");
        return true;
    }

    vbp_picture_data_h264* picData = data->pic_data;
    if (picData->num_slices == 0) {
        ETRACE("num_slices == 0");
        return true;
    }

    bool newFrame = false;
    uint32_t fieldFlags = VA_PICTURE_H264_TOP_FIELD | VA_PICTURE_H264_BOTTOM_FIELD;

    if (picData->slc_data[0].slc_parms.first_mb_in_slice != 0) {
        // not the first slice, assume it is continuation of a partial frame
        // TODO: check if it is new frame boundary as the first slice may get lost in streaming case.
        WTRACE("first_mb_in_slice != 0");
    } else {
        if ((picData->pic_parms->CurrPic.flags & fieldFlags) == fieldFlags) {
            ETRACE("Current picture has both odd field and even field.");
        }
        // current picture is a field or a frame, and buffer conains the first slice, check if the current picture and
        // the last picture form an opposite field pair
        if (((mLastPictureFlags | picData->pic_parms->CurrPic.flags) & fieldFlags) == fieldFlags) {
            // opposite field
            newFrame = false;
            WTRACE("current picture is not at frame boundary.");
            mLastPictureFlags = 0;
        } else {
            newFrame = true;
            mLastPictureFlags = 0;
            for (uint32_t i = 0; i < data->num_pictures; i++) {
                mLastPictureFlags |= data->pic_data[i].pic_parms->CurrPic.flags;
            }
            if ((mLastPictureFlags & fieldFlags) == fieldFlags) {
                // current buffer contains both odd field and even field.
                mLastPictureFlags = 0;
            }
        }
    }
    
    return newFrame;
}

int32_t VideoDecoderAVC::getDPBSize(vbp_data_h264 *data) {
    // 1024 * MaxDPB / ( PicWidthInMbs * FrameHeightInMbs * 384 ), 16
    struct DPBTable {
        int32_t level;
        float maxDPB;
    } dpbTable[] = {
        {9,  148.5},
        {10, 148.5},
        {11, 337.5},
        {12, 891.0},
        {13, 891.0},
        {20, 891.0},
        {21, 1782.0},
        {22, 3037.5},
        {30, 3037.5},
        {31, 6750.0},
        {32, 7680.0},
        {40, 12288.0},
        {41, 12288.0},
        {42, 13056.0},
        {50, 41400.0},
        {51, 69120.0}
    };

    int32_t count = sizeof(dpbTable)/sizeof(DPBTable);
    float maxDPB = 0;
    for (int32_t i = 0; i < count; i++)
    {
        if (dpbTable[i].level == data->codec_data->level_idc) {
            maxDPB = dpbTable[i].maxDPB;
            break;
        }
    }

    int32_t maxDPBSize = maxDPB * 1024 / (
        (data->pic_data[0].pic_parms->picture_width_in_mbs_minus1 + 1) *
        (data->pic_data[0].pic_parms->picture_height_in_mbs_minus1 + 1) *
        384);

    if (maxDPBSize > 16) {
        maxDPBSize = 16;
    } else if (maxDPBSize == 0) {
        maxDPBSize = 3;
    }
    if(maxDPBSize < data->codec_data->num_ref_frames) {
        maxDPBSize = data->codec_data->num_ref_frames;
    }

    // add one extra frame for current frame.
    maxDPBSize += 1;
    ITRACE("maxDPBSize = %d, num_ref_frame = %d", maxDPBSize, data->codec_data->num_ref_frames);
    return maxDPBSize;
}

