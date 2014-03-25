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
        mVideoParamsVP8.init_qp = 26;
        mVideoParamsVP8.rc_undershoot = 100;
        mVideoParamsVP8.rc_overshoot = 100;
        mVideoParamsVP8.hrd_buf_size = 1000;
        mVideoParamsVP8.hrd_buf_initial_fullness = 500;
        mVideoParamsVP8.hrd_buf_optimal_fullness = 600;
        mVideoParamsVP8.max_frame_size_ratio = 0;

        mVideoConfigVP8.force_kf = 0;
        mVideoConfigVP8.refresh_entropy_probs = 0;
        mVideoConfigVP8.value = 0;
        mVideoConfigVP8.sharpness_level = 2;

        mVideoConfigVP8ReferenceFrame.no_ref_last = 0;
        mVideoConfigVP8ReferenceFrame.no_ref_gf = 0;
        mVideoConfigVP8ReferenceFrame.no_ref_arf = 0;
        mVideoConfigVP8ReferenceFrame.refresh_last = 1;
        mVideoConfigVP8ReferenceFrame.refresh_golden_frame = 1;
        mVideoConfigVP8ReferenceFrame.refresh_alternate_frame = 1;

        mComParams.profile = VAProfileVP8Version0_3;
}

VideoEncoderVP8::~VideoEncoderVP8() {
}

Encode_Status VideoEncoderVP8::renderSequenceParams() {
    Encode_Status ret = ENCODE_SUCCESS;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAEncSequenceParameterBufferVP8 vp8SeqParam;

    LOG_V( "Begin\n");

    memset(&(vp8SeqParam),0x00, sizeof(VAEncSequenceParameterBufferVP8));
    vp8SeqParam.frame_width = mComParams.resolution.width;
    vp8SeqParam.frame_height = mComParams.resolution.height;
    vp8SeqParam.error_resilient = mVideoParamsVP8.error_resilient;
    vp8SeqParam.kf_auto = mVideoParamsVP8.kf_auto;
    vp8SeqParam.kf_min_dist = mVideoParamsVP8.kf_min_dist;
    vp8SeqParam.kf_max_dist = mVideoParamsVP8.kf_max_dist;
    vp8SeqParam.bits_per_second = mComParams.rcParams.bitRate;
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

    memset(&(vp8PicParam),0x00, sizeof(VAEncPictureParameterBufferVP8));
    vp8PicParam.coded_buf = task->coded_buffer;
    vp8PicParam.pic_flags.value = 0;
    vp8PicParam.ref_flags.bits.force_kf = mVideoConfigVP8.force_kf; //0;
    if(!vp8PicParam.ref_flags.bits.force_kf) {
        vp8PicParam.ref_flags.bits.no_ref_last = mVideoConfigVP8ReferenceFrame.no_ref_last;
        vp8PicParam.ref_flags.bits.no_ref_arf = mVideoConfigVP8ReferenceFrame.no_ref_arf;
        vp8PicParam.ref_flags.bits.no_ref_gf = mVideoConfigVP8ReferenceFrame.no_ref_gf;
    }
    vp8PicParam.pic_flags.bits.refresh_entropy_probs = 0;
    vp8PicParam.sharpness_level = 2;
    vp8PicParam.pic_flags.bits.num_token_partitions = 2;
    vp8PicParam.pic_flags.bits.refresh_last = mVideoConfigVP8ReferenceFrame.refresh_last;
    vp8PicParam.pic_flags.bits.refresh_golden_frame = mVideoConfigVP8ReferenceFrame.refresh_golden_frame;
    vp8PicParam.pic_flags.bits.refresh_alternate_frame = mVideoConfigVP8ReferenceFrame.refresh_alternate_frame;

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

Encode_Status VideoEncoderVP8::renderRCParams(void)
{
    VABufferID rc_param_buf;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAEncMiscParameterBuffer *misc_param;
    VAEncMiscParameterRateControl *misc_rate_ctrl;

    vaStatus = vaCreateBuffer(mVADisplay, mVAContext,
                              VAEncMiscParameterBufferType,
                              sizeof(VAEncMiscParameterBuffer) + sizeof(VAEncMiscParameterRateControl),
                              1,NULL,&rc_param_buf);
    CHECK_VA_STATUS_RETURN("vaCreateBuffer");

    vaMapBuffer(mVADisplay, rc_param_buf,(void **)&misc_param);

    misc_param->type = VAEncMiscParameterTypeRateControl;
    misc_rate_ctrl = (VAEncMiscParameterRateControl *)misc_param->data;
    memset(misc_rate_ctrl, 0, sizeof(*misc_rate_ctrl));
    misc_rate_ctrl->bits_per_second = mComParams.rcParams.bitRate;
    misc_rate_ctrl->target_percentage = 100;
    misc_rate_ctrl->window_size = 1000;
    misc_rate_ctrl->initial_qp = mVideoParamsVP8.init_qp;
    misc_rate_ctrl->min_qp = mVideoParamsVP8.min_qp;
    misc_rate_ctrl->basic_unit_size = 0;
    misc_rate_ctrl->max_qp = mVideoParamsVP8.max_qp;

    vaUnmapBuffer(mVADisplay, rc_param_buf);

    vaStatus = vaRenderPicture(mVADisplay,mVAContext, &rc_param_buf, 1);
    CHECK_VA_STATUS_RETURN("vaRenderPicture");;
    return 0;
}

Encode_Status VideoEncoderVP8::renderFrameRateParams(void)
{
    VABufferID framerate_param_buf;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAEncMiscParameterBuffer *misc_param;
    VAEncMiscParameterFrameRate * misc_framerate;
    uint32_t frameRateNum = mComParams.frameRate.frameRateNum;
    uint32_t frameRateDenom = mComParams.frameRate.frameRateDenom;

    vaStatus = vaCreateBuffer(mVADisplay, mVAContext,
                              VAEncMiscParameterBufferType,
                              sizeof(VAEncMiscParameterBuffer) + sizeof(VAEncMiscParameterFrameRate),
                              1,NULL,&framerate_param_buf);
    CHECK_VA_STATUS_RETURN("vaCreateBuffer");

    vaMapBuffer(mVADisplay, framerate_param_buf,(void **)&misc_param);
    misc_param->type = VAEncMiscParameterTypeFrameRate;
    misc_framerate = (VAEncMiscParameterFrameRate *)misc_param->data;
    memset(misc_framerate, 0, sizeof(*misc_framerate));
    misc_framerate->framerate = (unsigned int) (frameRateNum + frameRateDenom /2) / frameRateDenom;

    vaUnmapBuffer(mVADisplay, framerate_param_buf);

    vaStatus = vaRenderPicture(mVADisplay,mVAContext, &framerate_param_buf, 1);
    CHECK_VA_STATUS_RETURN("vaRenderPicture");;

    return 0;
}

Encode_Status VideoEncoderVP8::renderHRDParams(void)
{
    VABufferID hrd_param_buf;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAEncMiscParameterBuffer *misc_param;
    VAEncMiscParameterHRD * misc_hrd;
    vaStatus = vaCreateBuffer(mVADisplay, mVAContext,
                              VAEncMiscParameterBufferType,
                              sizeof(VAEncMiscParameterBuffer) + sizeof(VAEncMiscParameterHRD),
                              1,NULL,&hrd_param_buf);
    CHECK_VA_STATUS_RETURN("vaCreateBuffer");

    vaMapBuffer(mVADisplay, hrd_param_buf,(void **)&misc_param);
    misc_param->type = VAEncMiscParameterTypeHRD;
    misc_hrd = (VAEncMiscParameterHRD *)misc_param->data;
    memset(misc_hrd, 0, sizeof(*misc_hrd));
    misc_hrd->buffer_size = 1000;
    misc_hrd->initial_buffer_fullness = 500;
    misc_hrd->optimal_buffer_fullness = 600;
    vaUnmapBuffer(mVADisplay, hrd_param_buf);

    vaStatus = vaRenderPicture(mVADisplay,mVAContext, &hrd_param_buf, 1);
    CHECK_VA_STATUS_RETURN("vaRenderPicture");;

    return 0;
}

Encode_Status VideoEncoderVP8::renderMaxFrameSizeParams(void)
{
    VABufferID max_frame_size_param_buf;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAEncMiscParameterBuffer *misc_param;
    VAEncMiscParameterBufferMaxFrameSize * misc_maxframesize;
    unsigned int frameRateNum = mComParams.frameRate.frameRateNum;
    unsigned int frameRateDenom = mComParams.frameRate.frameRateDenom;
    unsigned int frameRate = (unsigned int)(frameRateNum + frameRateDenom /2);
    unsigned int bitRate = mComParams.rcParams.bitRate;

    vaStatus = vaCreateBuffer(mVADisplay, mVAContext,
                              VAEncMiscParameterBufferType,
                              sizeof(VAEncMiscParameterBuffer) + sizeof(VAEncMiscParameterHRD),
                              1,NULL,&max_frame_size_param_buf);
    CHECK_VA_STATUS_RETURN("vaCreateBuffer");

    vaMapBuffer(mVADisplay, max_frame_size_param_buf,(void **)&misc_param);
    misc_param->type = VAEncMiscParameterTypeMaxFrameSize;
    misc_maxframesize = (VAEncMiscParameterBufferMaxFrameSize *)misc_param->data;
    memset(misc_maxframesize, 0, sizeof(*misc_maxframesize));
    misc_maxframesize->max_frame_size = (unsigned int)((bitRate/frameRate) * mVideoParamsVP8.max_frame_size_ratio);
    vaUnmapBuffer(mVADisplay, max_frame_size_param_buf);

    vaStatus = vaRenderPicture(mVADisplay,mVAContext, &max_frame_size_param_buf, 1);
    CHECK_VA_STATUS_RETURN("vaRenderPicture");;

    return 0;
}

Encode_Status VideoEncoderVP8::renderMultiTemporalBitRateFrameRate(void)
{
    VABufferID rc_param_buf;
    VABufferID framerate_param_buf;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    VAEncMiscParameterBuffer *misc_param;
    VAEncMiscParameterRateControl *misc_rate_ctrl;
    VAEncMiscParameterFrameRate *misc_framerate;

    int i;

    vaStatus = vaCreateBuffer(mVADisplay, mVAContext,
			      VAEncMiscParameterBufferType,
                              sizeof(VAEncMiscParameterBuffer) + sizeof(VAEncMiscParameterRateControl),
                              1,NULL,&rc_param_buf);

    CHECK_VA_STATUS_RETURN("vaCreateBuffer");

    for(i=0;i<mComParams.numberOfLayer;i++)
    {
        vaMapBuffer(mVADisplay, rc_param_buf,(void **)&misc_param);

	misc_param->type = VAEncMiscParameterTypeRateControl;
        misc_rate_ctrl = (VAEncMiscParameterRateControl *)misc_param->data;
        memset(misc_rate_ctrl, 0, sizeof(*misc_rate_ctrl));
//        misc_rate_ctrl->bits_per_second = mVideoConfigVP8TemporalBitRateFrameRate[i].bitRate;
        misc_rate_ctrl->rc_flags.bits.temporal_id = 0;
        misc_rate_ctrl->target_percentage = 100;
        misc_rate_ctrl->window_size = 1000;
        misc_rate_ctrl->initial_qp = mVideoParamsVP8.init_qp;
        misc_rate_ctrl->min_qp = mVideoParamsVP8.min_qp;
        misc_rate_ctrl->basic_unit_size = 0;
        misc_rate_ctrl->max_qp = mVideoParamsVP8.max_qp;
        vaUnmapBuffer(mVADisplay, rc_param_buf);

        vaStatus = vaRenderPicture(mVADisplay,mVAContext, &rc_param_buf, 1);
    }

    vaStatus = vaCreateBuffer(mVADisplay, mVAContext,
                              VAEncMiscParameterBufferType,
                              sizeof(VAEncMiscParameterBuffer) + sizeof(VAEncMiscParameterFrameRate),
                              1,NULL,&framerate_param_buf);
    CHECK_VA_STATUS_RETURN("vaCreateBuffer");

    for(i=0;i<mComParams.numberOfLayer;i++)
    {
	vaMapBuffer(mVADisplay, framerate_param_buf,(void **)&misc_param);
	misc_param->type = VAEncMiscParameterTypeFrameRate;
	misc_framerate = (VAEncMiscParameterFrameRate *)misc_param->data;
	memset(misc_framerate, 0, sizeof(*misc_framerate));
	misc_framerate->framerate_flags.bits.temporal_id = i;
//	misc_framerate->framerate = mVideoConfigVP8TemporalBitRateFrameRate[i].frameRate;

        vaUnmapBuffer(mVADisplay, framerate_param_buf);

	vaStatus = vaRenderPicture(mVADisplay,mVAContext, &framerate_param_buf, 1);
    }

    CHECK_VA_STATUS_RETURN("vaRenderPicture");;

    return 0;
}

Encode_Status VideoEncoderVP8::sendEncodeCommand(EncodeTask *task) {

    Encode_Status ret = ENCODE_SUCCESS;
    LOG_V( "Begin\n");

    if (mFrameNum == 0) {
        ret = renderFrameRateParams();
        ret = renderRCParams();
        ret = renderHRDParams();
        ret = renderSequenceParams();
        ret = renderMaxFrameSizeParams();
        CHECK_ENCODE_STATUS_RETURN("renderSequenceParams");
    }

    if (mRenderBitRate){
        ret = renderRCParams();
        CHECK_ENCODE_STATUS_RETURN("renderRCParams");

        mRenderBitRate = false;
    }

    if (mRenderFrameRate) {
        ret = renderFrameRateParams();
        CHECK_ENCODE_STATUS_RETURN("renderFrameRateParams");

        mRenderFrameRate = false;
    }

    if (mRenderMaxFrameSize) {
        ret = renderMaxFrameSizeParams();
        CHECK_ENCODE_STATUS_RETURN("renderMaxFrameSizeParams");

        mRenderMaxFrameSize = false;
    }

    if (mRenderMultiTemporal) {
        ret = renderMultiTemporalBitRateFrameRate();
        CHECK_ENCODE_STATUS_RETURN("renderMultiTemporalBitRateFrameRate");

        mRenderMultiTemporal = false;
    }

    ret = renderPictureParams(task);
    CHECK_ENCODE_STATUS_RETURN("renderPictureParams");

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

		int layer_id;
        CHECK_NULL_RETURN_IFFAIL(videoEncConfig);

        switch (videoEncConfig->type)
        {
                case VideoConfigTypeVP8:{
                        VideoConfigVP8 *encConfigVP8 =
                                reinterpret_cast<VideoConfigVP8*> (videoEncConfig);

                        if (encConfigVP8->size != sizeof(VideoConfigVP8)) {
                                return ENCODE_INVALID_PARAMS;
                        }

                        *encConfigVP8 = mVideoConfigVP8;
                }
                break;

                case VideoConfigTypeVP8ReferenceFrame:{

                        VideoConfigVP8ReferenceFrame *encConfigVP8ReferenceFrame =
                                reinterpret_cast<VideoConfigVP8ReferenceFrame*> (videoEncConfig);

                        if (encConfigVP8ReferenceFrame->size != sizeof(VideoConfigVP8ReferenceFrame)) {
                                return ENCODE_INVALID_PARAMS;
                        }

                        *encConfigVP8ReferenceFrame = mVideoConfigVP8ReferenceFrame;

                }
                break;

                case VideoConfigTypeVP8MaxFrameSizeRatio :{

                        VideoConfigVP8MaxFrameSizeRatio *encConfigVP8MaxFrameSizeRatio =
                                reinterpret_cast<VideoConfigVP8MaxFrameSizeRatio*> (videoEncConfig);

                        if (encConfigVP8MaxFrameSizeRatio->size != sizeof(VideoConfigVP8MaxFrameSizeRatio)) {
                                return ENCODE_INVALID_PARAMS;
                        }

                        encConfigVP8MaxFrameSizeRatio->max_frame_size_ratio = mVideoParamsVP8.max_frame_size_ratio;
                }
                break;

                default: {
                   LOG_E ("Invalid Config Type");
                   break;
                }
       }

       return ENCODE_SUCCESS;
}

Encode_Status VideoEncoderVP8::derivedSetConfig(VideoParamConfigSet *videoEncConfig) {

        int layer_id;
        CHECK_NULL_RETURN_IFFAIL(videoEncConfig);

        //LOGE ("%s begin",__func__);

        switch (videoEncConfig->type)
        {
                case VideoConfigTypeVP8:{
                        VideoConfigVP8 *encConfigVP8 =
                                reinterpret_cast<VideoConfigVP8*> (videoEncConfig);

                        if (encConfigVP8->size != sizeof(VideoConfigVP8)) {
                                return ENCODE_INVALID_PARAMS;
                        }

                        mVideoConfigVP8 = *encConfigVP8;
                }
                break;

                case VideoConfigTypeVP8ReferenceFrame:{
                        VideoConfigVP8ReferenceFrame *encConfigVP8ReferenceFrame =
                                reinterpret_cast<VideoConfigVP8ReferenceFrame*> (videoEncConfig);

                        if (encConfigVP8ReferenceFrame->size != sizeof(VideoConfigVP8ReferenceFrame)) {
                                return ENCODE_INVALID_PARAMS;
                        }

                        mVideoConfigVP8ReferenceFrame = *encConfigVP8ReferenceFrame;

                }
                break;

                case VideoConfigTypeVP8MaxFrameSizeRatio:{
                        VideoConfigVP8MaxFrameSizeRatio *encConfigVP8MaxFrameSizeRatio =
                                reinterpret_cast<VideoConfigVP8MaxFrameSizeRatio*> (videoEncConfig);

                        if (encConfigVP8MaxFrameSizeRatio->size != sizeof(VideoConfigVP8MaxFrameSizeRatio)) {
                                return ENCODE_INVALID_PARAMS;
                        }

                        mVideoParamsVP8.max_frame_size_ratio = encConfigVP8MaxFrameSizeRatio->max_frame_size_ratio;
                        mRenderMaxFrameSize = true;
		}
                break;

                default: {
            LOG_E ("Invalid Config Type");
            break;
                }
        }
        return ENCODE_SUCCESS;
}
