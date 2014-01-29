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
#include <va/va.h>
#include "VideoDecoderBase.h"
#include "VideoDecoderAVC.h"
#include "VideoDecoderTrace.h"
#include "vbp_loader.h"
#include "VideoDecoderAVCSecure.h"

#define MAX_SLICEHEADER_BUFFER_SIZE 4096
#define STARTCODE_PREFIX_LEN        3
#define NALU_TYPE_MASK              0x1F
#define MAX_NALU_HEADER_BUFFER      8192
static const uint8_t startcodePrefix[STARTCODE_PREFIX_LEN] = {0x00, 0x00, 0x01};

/* H264 start code values */
typedef enum _h264_nal_unit_type
{
    h264_NAL_UNIT_TYPE_unspecified = 0,
    h264_NAL_UNIT_TYPE_SLICE,
    h264_NAL_UNIT_TYPE_DPA,
    h264_NAL_UNIT_TYPE_DPB,
    h264_NAL_UNIT_TYPE_DPC,
    h264_NAL_UNIT_TYPE_IDR,
    h264_NAL_UNIT_TYPE_SEI,
    h264_NAL_UNIT_TYPE_SPS,
    h264_NAL_UNIT_TYPE_PPS,
    h264_NAL_UNIT_TYPE_Acc_unit_delimiter,
    h264_NAL_UNIT_TYPE_EOSeq,
    h264_NAL_UNIT_TYPE_EOstream,
    h264_NAL_UNIT_TYPE_filler_data,
    h264_NAL_UNIT_TYPE_SPS_extension,
    h264_NAL_UNIT_TYPE_ACP = 19,
    h264_NAL_UNIT_TYPE_Slice_extension = 20
} h264_nal_unit_type_t;

VideoDecoderAVCSecure::VideoDecoderAVCSecure(const char *mimeType)
    : VideoDecoderAVC(mimeType){
    mFrameSize     = 0;
    mFrameData     = NULL;
    mIsEncryptData = 0;
    mClearData     = NULL;
    setParserType(VBP_H264SECURE);
    mFrameIdx = 0;
}

Decode_Status VideoDecoderAVCSecure::start(VideoConfigBuffer *buffer) {
    VTRACE("VideoDecoderAVCSecure::start");
    Decode_Status status = VideoDecoderAVC::start(buffer);
    if (status != DECODE_SUCCESS) {
        return status;
    }

    mClearData = new uint8_t [MAX_NALU_HEADER_BUFFER];
    if (mClearData == NULL) {
        ETRACE("Failed to allocate memory for mClearData");
        return DECODE_MEMORY_FAIL;
    }

    return status;
}

void VideoDecoderAVCSecure::stop(void) {
    VTRACE("VideoDecoderAVCSecure::stop");
    VideoDecoderAVC::stop();

    if (mClearData) {
        delete [] mClearData;
        mClearData = NULL;
    }
}

Decode_Status VideoDecoderAVCSecure::decode(VideoDecodeBuffer *buffer) {
    VTRACE("VideoDecoderAVCSecure::decode");
    Decode_Status status;
    vbp_data_h264 *data = NULL;
    if (buffer == NULL) {
        return DECODE_INVALID_DATA;
    }

    int32_t clear_data_size = 0;
    uint8_t *clear_data = NULL;
    uint8_t naluType = 0;

    int32_t num_nalus;
    int32_t nalu_offset;
    int32_t offset;
    uint8_t *data_src;
    uint8_t *nalu_data;
    uint32_t nalu_size;
//    uint32_t testsize;
//    uint8_t *testdata;
    if (buffer->flag & IS_SECURE_DATA) {
        VTRACE("Decoding protected video ...");
        mIsEncryptData = 1;

        mFrameData = buffer->data;
        mFrameSize = buffer->size;
        VTRACE("mFrameData = %p, mFrameSize = %d", mFrameData, mFrameSize);
#if 0
        testsize = *(uint32_t *)(buffer->data + buffer->size);
        testdata = (uint8_t *)(buffer->data + buffer->size + sizeof(uint32_t));
        for (int i = 0; i < testsize; i++) {
            VTRACE("testdata[%d] = 0x%x", i, testdata[i]);
        }
#endif
        num_nalus  = *(uint32_t *)(buffer->data + buffer->size + sizeof(uint32_t));
        VTRACE("num_nalus = %d", num_nalus);
        offset = 4;
        for (int32_t i = 0; i < num_nalus; i++) {
            VTRACE("%d nalu, offset = %d", i, offset);
            data_src = buffer->data + buffer->size + sizeof(uint32_t) + offset;
            nalu_size = *(uint32_t *)(data_src + 2 * sizeof(uint32_t));
            nalu_size = (nalu_size + 0x03) & (~0x03);

            nalu_data = data_src + 3 *sizeof(uint32_t);
            naluType  = nalu_data[0] & NALU_TYPE_MASK;
            offset += nalu_size + 3 *sizeof(uint32_t);
            VTRACE("naluType = 0x%x", naluType);
            VTRACE("nalu_size = %d, nalu_data = %p", nalu_size, nalu_data);

            if (naluType >= h264_NAL_UNIT_TYPE_SLICE && naluType <= h264_NAL_UNIT_TYPE_IDR) {
                ETRACE("Slice NALU received!");
                return DECODE_INVALID_DATA;
            }

            else if (naluType >= h264_NAL_UNIT_TYPE_SEI && naluType <= h264_NAL_UNIT_TYPE_PPS) {
                memcpy(mClearData + clear_data_size,
                    startcodePrefix,
                    STARTCODE_PREFIX_LEN);
                clear_data_size += STARTCODE_PREFIX_LEN;
                memcpy(mClearData + clear_data_size,
                    nalu_data,
                    nalu_size);
                clear_data_size += nalu_size;
            } else {
                ETRACE("Failure: DECODE_FRAME_DROPPED");
                return DECODE_FRAME_DROPPED;
            }
        }
        clear_data = mClearData;

    } else {
        VTRACE("Decoding clear video ...");
        mIsEncryptData = 0;
        mFrameSize = buffer->size;
        mFrameData = buffer->data;
        clear_data = buffer->data;
        clear_data_size = buffer->size;
    }
    if (clear_data_size > 0) {
        status =  VideoDecoderBase::parseBuffer(
                clear_data,
                clear_data_size,
                false,
                (void**)&data);
        CHECK_STATUS("VideoDecoderBase::parseBuffer");
    } else {
        status =  VideoDecoderBase::queryBuffer((void**)&data);
        CHECK_STATUS("VideoDecoderBase::queryBuffer");
    }

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

Decode_Status VideoDecoderAVCSecure::decodeFrame(VideoDecodeBuffer *buffer, vbp_data_h264 *data) {
    VTRACE("VideoDecoderAVCSecure::decodeFrame");
    Decode_Status status;
    VTRACE("data->has_sps = %d, data->has_pps = %d", data->has_sps, data->has_pps);

#if 0
    // Don't remove the following codes, it can be enabled for debugging DPB.
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

    if (data->new_sps || data->new_pps) {
        status = handleNewSequence(data);
        CHECK_STATUS("handleNewSequence");
    }

    uint64_t lastPTS = mCurrentPTS;
    mCurrentPTS = buffer->timeStamp;

    // start decoding a new frame
    status = acquireSurfaceBuffer();
    CHECK_STATUS("acquireSurfaceBuffer");

    if (mFrameSize > 0) {
        status = parseSliceHeader(buffer, data);
    }
    if (status != DECODE_SUCCESS) {
        endDecodingFrame(true);
        return status;
    }

    status = beginDecodingFrame(data);
    CHECK_STATUS("beginDecodingFrame");

   // finish decoding the last frame
    status = endDecodingFrame(false);
    CHECK_STATUS("endDecodingFrame");

    if (isNewFrame(data, lastPTS == mCurrentPTS) == 0) {
        ETRACE("Can't handle interlaced frames yet");
        return DECODE_FAIL;
    }

    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderAVCSecure::beginDecodingFrame(vbp_data_h264 *data) {
    VTRACE("VideoDecoderAVCSecure::beginDecodingFrame");
    Decode_Status status;
    VAPictureH264 *picture = &(data->pic_data[0].pic_parms->CurrPic);
    if ((picture->flags & VA_PICTURE_H264_SHORT_TERM_REFERENCE) ||
        (picture->flags & VA_PICTURE_H264_LONG_TERM_REFERENCE)) {
        mAcquiredBuffer->referenceFrame = true;
    } else {
        mAcquiredBuffer->referenceFrame = false;
    }

    if (picture->flags & VA_PICTURE_H264_TOP_FIELD) {
        mAcquiredBuffer->renderBuffer.scanFormat = VA_BOTTOM_FIELD | VA_TOP_FIELD;
    } else {
        mAcquiredBuffer->renderBuffer.scanFormat = VA_FRAME_PICTURE;
    }

    mAcquiredBuffer->renderBuffer.flag = 0;
    mAcquiredBuffer->renderBuffer.timeStamp = mCurrentPTS;
    mAcquiredBuffer->pictureOrder = getPOC(picture);

    status  = continueDecodingFrame(data);
    return status;
}

Decode_Status VideoDecoderAVCSecure::continueDecodingFrame(vbp_data_h264 *data) {
    VTRACE("VideoDecoderAVCSecure::continueDecodingFrame");
    Decode_Status status;
    vbp_picture_data_h264 *picData = data->pic_data;

    if (mAcquiredBuffer == NULL || mAcquiredBuffer->renderBuffer.surface == VA_INVALID_SURFACE) {
        ETRACE("mAcquiredBuffer is NULL. Implementation bug.");
        return DECODE_FAIL;
    }
    VTRACE("data->num_pictures = %d", data->num_pictures);
    for (uint32_t picIndex = 0; picIndex < data->num_pictures; picIndex++, picData++) {
        if (picData == NULL || picData->pic_parms == NULL || picData->slc_data == NULL || picData->num_slices == 0) {
            return DECODE_PARSER_FAIL;
        }

        if (picIndex > 0 &&
            (picData->pic_parms->CurrPic.flags & (VA_PICTURE_H264_TOP_FIELD | VA_PICTURE_H264_BOTTOM_FIELD)) == 0) {
            ETRACE("Packed frame is not supported yet!");
            return DECODE_FAIL;
        }
        VTRACE("picData->num_slices = %d", picData->num_slices);
        for (uint32_t sliceIndex = 0; sliceIndex < picData->num_slices; sliceIndex++) {
            status = decodeSlice(data, picIndex, sliceIndex);
            if (status != DECODE_SUCCESS) {
                endDecodingFrame(true);
                // remove current frame from DPB as it can't be decoded.
                removeReferenceFromDPB(picData->pic_parms);
                return status;
            }
        }
    }
    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderAVCSecure::parseSliceHeader(VideoDecodeBuffer *buffer, vbp_data_h264 *data) {
    Decode_Status status;
    VAStatus vaStatus;

    VABufferID sliceheaderbufferID;
    VABufferID pictureparameterparsingbufferID;
    VABufferID mSlicebufferID;

    vaStatus = vaBeginPicture(mVADisplay, mVAContext, mAcquiredBuffer->renderBuffer.surface);
    CHECK_VA_STATUS("vaBeginPicture");

    vaStatus = vaCreateBuffer(
        mVADisplay,
        mVAContext,
        VAParseSliceHeaderGroupBufferType,
        MAX_SLICEHEADER_BUFFER_SIZE,
        1,
        NULL,
        &sliceheaderbufferID);
    CHECK_VA_STATUS("vaCreateSliceHeaderGroupBuffer");

    void *sliceheaderbuf;
    vaStatus = vaMapBuffer(
        mVADisplay,
        sliceheaderbufferID,
        &sliceheaderbuf);
    CHECK_VA_STATUS("vaMapBuffer");

    memset(sliceheaderbuf, 0, MAX_SLICEHEADER_BUFFER_SIZE);

    vaStatus = vaUnmapBuffer(
        mVADisplay,
        sliceheaderbufferID);
    CHECK_VA_STATUS("vaUnmapBuffer");


    vaStatus = vaCreateBuffer(
        mVADisplay,
        mVAContext,
        VASliceDataBufferType,
        mFrameSize, //size
        1,        //num_elements
        mFrameData,
        &mSlicebufferID);
    CHECK_VA_STATUS("vaCreateSliceDataBuffer");

    data->pic_parse_buffer->frame_buf_id = mSlicebufferID;
    data->pic_parse_buffer->slice_headers_buf_id = sliceheaderbufferID;
    data->pic_parse_buffer->frame_size = mFrameSize;
    data->pic_parse_buffer->slice_headers_size = MAX_SLICEHEADER_BUFFER_SIZE;

#if 0

    VTRACE("flags.bits.frame_mbs_only_flag = %d", data->pic_parse_buffer->flags.bits.frame_mbs_only_flag);
    VTRACE("flags.bits.pic_order_present_flag = %d", data->pic_parse_buffer->flags.bits.pic_order_present_flag);
    VTRACE("flags.bits.delta_pic_order_always_zero_flag = %d", data->pic_parse_buffer->flags.bits.delta_pic_order_always_zero_flag);
    VTRACE("flags.bits.redundant_pic_cnt_present_flag = %d", data->pic_parse_buffer->flags.bits.redundant_pic_cnt_present_flag);
    VTRACE("flags.bits.weighted_pred_flag = %d", data->pic_parse_buffer->flags.bits.weighted_pred_flag);
    VTRACE("flags.bits.entropy_coding_mode_flag = %d", data->pic_parse_buffer->flags.bits.entropy_coding_mode_flag);
    VTRACE("flags.bits.deblocking_filter_control_present_flag = %d", data->pic_parse_buffer->flags.bits.deblocking_filter_control_present_flag);
    VTRACE("flags.bits.weighted_bipred_idc = %d", data->pic_parse_buffer->flags.bits.weighted_bipred_idc);

    VTRACE("pic_parse_buffer->expected_pic_parameter_set_id = %d", data->pic_parse_buffer->expected_pic_parameter_set_id);
    VTRACE("pic_parse_buffer->num_slice_groups_minus1 = %d", data->pic_parse_buffer->num_slice_groups_minus1);
    VTRACE("pic_parse_buffer->chroma_format_idc = %d", data->pic_parse_buffer->chroma_format_idc);
    VTRACE("pic_parse_buffer->log2_max_pic_order_cnt_lsb_minus4 = %d", data->pic_parse_buffer->log2_max_pic_order_cnt_lsb_minus4);
    VTRACE("pic_parse_buffer->pic_order_cnt_type = %d", data->pic_parse_buffer->pic_order_cnt_type);
    VTRACE("pic_parse_buffer->residual_colour_transform_flag = %d", data->pic_parse_buffer->residual_colour_transform_flag);
    VTRACE("pic_parse_buffer->num_ref_idc_l0_active_minus1 = %d", data->pic_parse_buffer->num_ref_idc_l0_active_minus1);
    VTRACE("pic_parse_buffer->num_ref_idc_l1_active_minus1 = %d", data->pic_parse_buffer->num_ref_idc_l1_active_minus1);
#endif

    vaStatus = vaCreateBuffer(
        mVADisplay,
        mVAContext,
        VAParsePictureParameterBufferType,
        sizeof(VAParsePictureParameterBuffer),
        1,
        data->pic_parse_buffer,
        &pictureparameterparsingbufferID);
    CHECK_VA_STATUS("vaCreatePictureParameterParsingBuffer");

    vaStatus = vaRenderPicture(
        mVADisplay,
        mVAContext,
        &pictureparameterparsingbufferID,
        1);
    CHECK_VA_STATUS("vaRenderPicture");

    vaStatus = vaMapBuffer(
        mVADisplay,
        sliceheaderbufferID,
        &sliceheaderbuf);
    CHECK_VA_STATUS("vaMapBuffer");

    status = updateSliceParameter(data,sliceheaderbuf);
    CHECK_STATUS("processSliceHeader");

    vaStatus = vaUnmapBuffer(
        mVADisplay,
        sliceheaderbufferID);
    CHECK_VA_STATUS("vaUnmapBuffer");

    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderAVCSecure::updateSliceParameter(vbp_data_h264 *data, void *sliceheaderbuf) {
    VTRACE("VideoDecoderAVCSecure::updateSliceParameter");
    Decode_Status status;
    status =  VideoDecoderBase::updateBuffer(
            (uint8_t *)sliceheaderbuf,
            MAX_SLICEHEADER_BUFFER_SIZE,
            (void**)&data);
    CHECK_STATUS("updateBuffer");
    return DECODE_SUCCESS;
}

Decode_Status VideoDecoderAVCSecure::decodeSlice(vbp_data_h264 *data, uint32_t picIndex, uint32_t sliceIndex) {
    Decode_Status status;
    VAStatus vaStatus;
    uint32_t bufferIDCount = 0;
    // maximum 3 buffers to render a slice: picture parameter, IQMatrix, slice parameter
    VABufferID bufferIDs[3];

    vbp_picture_data_h264 *picData = &(data->pic_data[picIndex]);
    vbp_slice_data_h264 *sliceData = &(picData->slc_data[sliceIndex]);
    VAPictureParameterBufferH264 *picParam = picData->pic_parms;
    VASliceParameterBufferH264 *sliceParam = &(sliceData->slc_parms);

    if (sliceParam->first_mb_in_slice == 0 || mDecodingFrame == false) {
        // either condition indicates start of a new frame
        if (sliceParam->first_mb_in_slice != 0) {
            WTRACE("The first slice is lost.");
        }
        VTRACE("Current frameidx = %d", mFrameIdx++);
        // Update  the reference frames and surface IDs for DPB and current frame
        status = updateDPB(picParam);
        CHECK_STATUS("updateDPB");

        //We have to provide a hacked DPB rather than complete DPB for libva as workaround
        status = updateReferenceFrames(picData);
        CHECK_STATUS("updateReferenceFrames");

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

    sliceParam->slice_data_size = mFrameSize;
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

    vaStatus = vaRenderPicture(
        mVADisplay,
        mVAContext,
        bufferIDs,
        bufferIDCount);
    CHECK_VA_STATUS("vaRenderPicture");

    VABufferID slicebufferID;

    vaStatus = vaCreateBuffer(
        mVADisplay,
        mVAContext,
        VASliceDataBufferType,
        mFrameSize, //size
        1,        //num_elements
        mFrameData,
        &slicebufferID);
    CHECK_VA_STATUS("vaCreateSliceDataBuffer");

    vaStatus = vaRenderPicture(
        mVADisplay,
        mVAContext,
        &slicebufferID,
        1);
    CHECK_VA_STATUS("vaRenderPicture");

    return DECODE_SUCCESS;

}
