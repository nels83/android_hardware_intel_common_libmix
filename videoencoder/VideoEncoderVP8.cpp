/*
 INTEL CONFIDENTIAL
 Copyright 2011 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

#include <string.h>
#include <stdlib.h>
#include "VideoEncoderLog.h"
#include "VideoEncoderVP8.h"
#include <va/va_tpi.h>
#include <va/va_enc_vp8.h>

VideoEncoderVP8::VideoEncoderVP8()
    :VideoEncoderBase() {

	mVideoParamsVP8.profile = 0;
	mVideoParamsVP8.error_resilient = 0;
	mVideoParamsVP8.num_token_partitions = 4;
	mVideoParamsVP8.kf_auto = 1;
	mVideoParamsVP8.kf_min_dist = 0;
	mVideoParamsVP8.kf_max_dist = 30;
	mVideoParamsVP8.min_qp = 4;
	mVideoParamsVP8.max_qp = 63;
	mVideoParamsVP8.rc_undershoot = 100;
	mVideoParamsVP8.rc_overshoot = 100;
	mVideoParamsVP8.hrd_buf_size = 6000;
	mVideoParamsVP8.hrd_buf_initial_fullness = 4000;
	mVideoParamsVP8.hrd_buf_optimal_fullness = 5000;

	mVideoConfigVP8.force_kf = 0;
	mVideoConfigVP8.no_ref_last = 0;
	mVideoConfigVP8.no_ref_gf = 0;
	mVideoConfigVP8.no_ref_arf = 0;

	mComParams.profile = VAProfileVP8Version0_3;
}

VideoEncoderVP8::~VideoEncoderVP8() {
}

Encode_Status VideoEncoderVP8::renderSequenceParams() {
    Encode_Status ret = ENCODE_SUCCESS;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAEncSequenceParameterBufferVP8 vp8SeqParam;
    uint32_t frameRateNum = mComParams.frameRate.frameRateNum;
    uint32_t frameRateDenom = mComParams.frameRate.frameRateDenom;
    LOG_V( "Begin\n");

    vp8SeqParam.frame_width = mComParams.resolution.width;
    vp8SeqParam.frame_height = mComParams.resolution.height;
    vp8SeqParam.frame_rate = (unsigned int) (frameRateNum + frameRateDenom /2) / frameRateDenom;
    vp8SeqParam.error_resilient = mVideoParamsVP8.error_resilient;
    vp8SeqParam.kf_auto = mVideoParamsVP8.kf_auto;
    vp8SeqParam.kf_min_dist = mVideoParamsVP8.kf_min_dist;
    vp8SeqParam.kf_max_dist = mVideoParamsVP8.kf_max_dist;
    vp8SeqParam.bits_per_second = mComParams.rcParams.bitRate;
    vp8SeqParam.min_qp = mVideoParamsVP8.min_qp;
    vp8SeqParam.max_qp = mVideoParamsVP8.max_qp;
    vp8SeqParam.rc_undershoot = mVideoParamsVP8.rc_undershoot;
    vp8SeqParam.rc_overshoot = mVideoParamsVP8.rc_overshoot;
    vp8SeqParam.hrd_buf_size = mVideoParamsVP8.hrd_buf_size;
    vp8SeqParam.hrd_buf_initial_fullness = mVideoParamsVP8.hrd_buf_initial_fullness;
    vp8SeqParam.hrd_buf_optimal_fullness = mVideoParamsVP8.hrd_buf_optimal_fullness;
//    memcpy(vp8SeqParam.reference_frames, mVP8InternalFrames, sizeof(mVP8InternalFrames));
    memcpy(vp8SeqParam.reference_frames, mAutoRefSurfaces, sizeof(mAutoRefSurfaces) * mAutoReferenceSurfaceNum);

    vaStatus = vaCreateBuffer(
            mVADisplay, mVAContext,
            VAEncSequenceParameterBufferType,
            sizeof(vp8SeqParam),
            1, &vp8SeqParam,
            &mSeqParamBuf);
    CHECK_VA_STATUS_RETURN("vaCreateBuffer");

    vaStatus = vaRenderPicture(mVADisplay, mVAContext, &mSeqParamBuf, 1);
    CHECK_VA_STATUS_RETURN("vaRenderPicture");

    LOG_V( "End\n");
	return ret;
}

Encode_Status VideoEncoderVP8::renderPictureParams(EncodeTask *task) {
    Encode_Status ret = ENCODE_SUCCESS;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAEncPictureParameterBufferVP8 vp8PicParam;
    LOG_V( "Begin\n");

    vp8PicParam.coded_buf = task->coded_buffer;
    vp8PicParam.pic_flags.bits.force_kf = mVideoConfigVP8.force_kf;
    vp8PicParam.pic_flags.bits.no_ref_last = mVideoConfigVP8.no_ref_last;
    vp8PicParam.pic_flags.bits.no_ref_gf = mVideoConfigVP8.no_ref_gf;
    vp8PicParam.pic_flags.bits.no_ref_arf = mVideoConfigVP8.no_ref_arf;
    vp8PicParam.pic_flags.value = 0;

    vaStatus = vaCreateBuffer(
            mVADisplay, mVAContext,
            VAEncPictureParameterBufferType,
            sizeof(vp8PicParam),
            1, &vp8PicParam,
            &mPicParamBuf);
    CHECK_VA_STATUS_RETURN("vaCreateBuffer");

    vaStatus = vaRenderPicture(mVADisplay, mVAContext, &mPicParamBuf, 1);
    CHECK_VA_STATUS_RETURN("vaRenderPicture");

    LOG_V( "End\n");
	return ret;
}

Encode_Status VideoEncoderVP8::renderSliceParams(EncodeTask *task) {

    VAStatus vaStatus = VA_STATUS_SUCCESS;
    uint32_t sliceHeight;
    uint32_t sliceHeightInMB;

    VAEncSliceParameterBuffer sliceParams;

    LOG_V( "Begin\n\n");

    sliceHeight = mComParams.resolution.height;
    sliceHeight += 15;
    sliceHeight &= (~15);
    sliceHeightInMB = sliceHeight / 16;

    sliceParams.start_row_number = 0;
    sliceParams.slice_height = sliceHeightInMB;
    sliceParams.slice_flags.bits.is_intra = (task->type == FTYPE_I)?1:0;
    sliceParams.slice_flags.bits.disable_deblocking_filter_idc = 0;

    LOG_V("======VP8 slice params======\n");
    LOG_I( "start_row_number = %d\n", (int) sliceParams.start_row_number);
    LOG_I( "sliceHeightInMB = %d\n", (int) sliceParams.slice_height);
    LOG_I( "is_intra = %d\n", (int) sliceParams.slice_flags.bits.is_intra);

    vaStatus = vaCreateBuffer(
            mVADisplay, mVAContext,
            VAEncSliceParameterBufferType,
            sizeof(VAEncSliceParameterBuffer),
            1, &sliceParams,
            &mSliceParamBuf);
    CHECK_VA_STATUS_RETURN("vaCreateBuffer");

    vaStatus = vaRenderPicture(mVADisplay, mVAContext, &mSliceParamBuf, 1);
    CHECK_VA_STATUS_RETURN("vaRenderPicture");

    LOG_V( "end\n");
    return ENCODE_SUCCESS;
}


Encode_Status VideoEncoderVP8::sendEncodeCommand(EncodeTask *task) {

    Encode_Status ret = ENCODE_SUCCESS;
    LOG_V( "Begin\n");

    if (mFrameNum == 0) {
        ret = renderSequenceParams();
        CHECK_ENCODE_STATUS_RETURN("renderSequenceParams");
    }

    ret = renderPictureParams(task);
    CHECK_ENCODE_STATUS_RETURN("renderPictureParams");

    ret = renderSliceParams(task);
    CHECK_ENCODE_STATUS_RETURN("renderSliceParams");

    LOG_V( "End\n");
    return ret;
}


Encode_Status VideoEncoderVP8::derivedSetParams(VideoParamConfigSet *videoEncParams) {

	CHECK_NULL_RETURN_IFFAIL(videoEncParams);
	VideoParamsVP8 *encParamsVP8 = reinterpret_cast <VideoParamsVP8*> (videoEncParams);

	if (encParamsVP8->size != sizeof(VideoParamsVP8)) {
		return ENCODE_INVALID_PARAMS;
	}

	mVideoParamsVP8 = *encParamsVP8;
	return ENCODE_SUCCESS;
}

Encode_Status VideoEncoderVP8::derivedGetParams(VideoParamConfigSet *videoEncParams) {

	CHECK_NULL_RETURN_IFFAIL(videoEncParams);
	VideoParamsVP8 *encParamsVP8 = reinterpret_cast <VideoParamsVP8*> (videoEncParams);

	if (encParamsVP8->size != sizeof(VideoParamsVP8)) {
		return ENCODE_INVALID_PARAMS;
	}

	*encParamsVP8 = mVideoParamsVP8;
	return ENCODE_SUCCESS;
}

Encode_Status VideoEncoderVP8::derivedGetConfig(VideoParamConfigSet *videoEncConfig) {

	CHECK_NULL_RETURN_IFFAIL(videoEncConfig);
	VideoConfigVP8 *encConfigVP8 = reinterpret_cast<VideoConfigVP8*> (videoEncConfig);

	if (encConfigVP8->size != sizeof(VideoConfigVP8)) {
		return ENCODE_INVALID_PARAMS;
	}

	*encConfigVP8 = mVideoConfigVP8;
	return ENCODE_SUCCESS;
}

Encode_Status VideoEncoderVP8::derivedSetConfig(VideoParamConfigSet *videoEncConfig) {

	CHECK_NULL_RETURN_IFFAIL(videoEncConfig);
	VideoConfigVP8 *encConfigVP8 = reinterpret_cast<VideoConfigVP8*> (videoEncConfig);

	if (encConfigVP8->size != sizeof(VideoConfigVP8)) {
		return ENCODE_INVALID_PARAMS;
	}

	mVideoConfigVP8 = *encConfigVP8;
	return ENCODE_SUCCESS;
}
