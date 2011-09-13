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

#include "VideoDecoderPAVC.h"
#include "VideoDecoderTrace.h"
#include <string.h>

VideoDecoderPAVC::VideoDecoderPAVC(const char *mimeType)
    : VideoDecoderAVC(mimeType),
      mMetadata(NULL) {
}

VideoDecoderPAVC::~VideoDecoderPAVC() {
}

Decode_Status VideoDecoderPAVC::decode(VideoDecodeBuffer *buffer) {
    // TODO: preprocessing protected content here

    mMetadata = NULL;

    if (buffer->flag & HAS_EXTRADATA) {
        mMetadata = buffer->data + buffer->size;
    }

    return VideoDecoderAVC::decode(buffer);
}


Decode_Status VideoDecoderPAVC::decodeSlice(vbp_data_h264 *data, uint32_t picIndex, uint32_t sliceIndex) {
    if (mMetadata == NULL) {
        // non-protected content playback path
        return VideoDecoderAVC::decodeSlice(data, picIndex, sliceIndex);
    }

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
            mAcquiredBuffer->pictureOrder= picParam->CurrPic.TopFieldOrderCnt;
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

    // find which medata is correlated to current slice
    PAVCMetadata *pMetadata = (PAVCMetadata*)mMetadata;
    uint32_t accumulatedClearNALUSize = 0;
    uint32_t clearNALUSize = 0;
    do {
        clearNALUSize = pMetadata->clearHeaderSize + pMetadata->decryptionDataSize;
        if (clearNALUSize == 0) {
            LOGE("Could not find meta data for current NAL unit.");
            return DECODE_INVALID_DATA;
        }

        if (accumulatedClearNALUSize + clearNALUSize > sliceData->slice_offset) {
            break;
        }
        accumulatedClearNALUSize += clearNALUSize;
        pMetadata++;
    } while (1);

    // add bytes that are encrypted
    sliceParam->slice_data_size += pMetadata->encryptionDataSize;
    sliceData->slice_size = sliceParam->slice_data_size;

    // no need to update:
    // sliceParam->slice_data_offset - 0 always
    // sliceParam->slice_data_bit_offset - relative to  sliceData->slice_offset

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

    // sliceData->slice_offset - accumulatedClearNALUSize is the absolute offset to start codes of current NAL unit
    // offset points to first byte of NAL unit
    uint32_t offset = pMetadata->clearHeaderIMROffset + sliceData->slice_offset - accumulatedClearNALUSize;
    vaStatus = vaCreateBuffer(
        mVADisplay,
        mVAContext,
        //VASliceDataBufferType,
        VAProtectedSliceDataBufferType,
        sliceData->slice_size, //size
        1,        //num_elements
        //sliceData->buffer_addr + sliceData->slice_offset,
        (uint8_t*)offset,
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

