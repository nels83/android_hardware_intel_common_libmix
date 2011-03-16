/*
INTEL CONFIDENTIAL
Copyright 2009 Intel Corporation All Rights Reserved.
The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
*/

/**
 * SECTION:mixvideoconfigparamsenc_h264
 * @short_description: MI-X Video H.264 Eecode Configuration Parameter
 *
 * MI-X video H.264 eecode configuration parameter objects.
 */


#include "mixvideolog.h"
#include "mixvideoconfigparamsenc_h264.h"

#define MDEBUG

MixVideoConfigParamsEncH264::MixVideoConfigParamsEncH264()
        :basic_unit_size(0)
        ,slice_num(1)
        ,I_slice_num(1)
        ,P_slice_num(1)
        ,disable_deblocking_filter_idc(0)
        ,vui_flag(0)
        ,delimiter_type(MIX_DELIMITER_LENGTHPREFIX)
        ,idr_interval(2)
        ,reserved1(NULL)
        ,reserved2(NULL)
        ,reserved3(NULL)
        ,reserved4(NULL) {
}

MixVideoConfigParamsEncH264::~MixVideoConfigParamsEncH264() {
}

bool MixVideoConfigParamsEncH264::copy(MixParams *target) const {
    bool ret = FALSE;
    MixVideoConfigParamsEncH264 * this_target = MIX_VIDEOCONFIGPARAMSENC_H264(target);
    if (NULL != this_target) {
        this_target->basic_unit_size = basic_unit_size;
        this_target->slice_num = slice_num;
        this_target->I_slice_num = I_slice_num;
        this_target->P_slice_num = P_slice_num;
        this_target->disable_deblocking_filter_idc = disable_deblocking_filter_idc;
        this_target->vui_flag = vui_flag;
        this_target->delimiter_type = delimiter_type;
        this_target->idr_interval = idr_interval;
        ret = MixVideoConfigParamsEnc::copy(target);
    }
    return ret;
}

bool MixVideoConfigParamsEncH264::equal(MixParams* obj) const {
    bool ret = FALSE;
    MixVideoConfigParamsEncH264 * this_obj = MIX_VIDEOCONFIGPARAMSENC_H264(obj);
    if (NULL == this_obj)
        return ret;

    if (this_obj->basic_unit_size != basic_unit_size) {
        goto not_equal;
    }

    if (this_obj->slice_num != slice_num) {
        goto not_equal;
    }

    if (this_obj->I_slice_num != I_slice_num) {
        goto not_equal;
    }

    if (this_obj->P_slice_num !=P_slice_num) {
        goto not_equal;
    }

    if (this_obj->disable_deblocking_filter_idc != disable_deblocking_filter_idc) {
        goto not_equal;
    }

    if (this_obj->vui_flag !=vui_flag) {
        goto not_equal;
    }

    if (this_obj->delimiter_type != delimiter_type) {
        goto not_equal;
    }

    if (this_obj->idr_interval != idr_interval) {
        goto not_equal;
    }

    ret = TRUE;
not_equal:

    if (ret != TRUE) {
        return ret;
    }

    ret = MixVideoConfigParamsEnc::equal(this_obj);
    return ret;
}

MixParams* MixVideoConfigParamsEncH264::dup() const {
    MixParams *ret = new MixVideoConfigParamsEncH264();
    if (NULL != ret) {
        if (FALSE == copy(ret)) {
            ret->Unref();
            ret = NULL;
        }
    }
    return ret;
}

MixVideoConfigParamsEncH264 *
mix_videoconfigparamsenc_h264_new (void) {
    return new MixVideoConfigParamsEncH264();
}



MixVideoConfigParamsEncH264*
mix_videoconfigparamsenc_h264_ref (MixVideoConfigParamsEncH264 * mix) {
    if (NULL != mix)
        mix->Ref();
    return mix;
}


/* TODO: Add getters and setters for properties if any */

#define MIX_VIDEOCONFIGPARAMSENC_H264_SETTER_CHECK_INPUT(obj) \
	if(!obj) return MIX_RESULT_NULL_PTR; \
	if(!MIX_IS_VIDEOCONFIGPARAMSENC_H264(obj)) return MIX_RESULT_FAIL; \
 
#define MIX_VIDEOCONFIGPARAMSENC_H264_GETTER_CHECK_INPUT(obj, prop) \
	if(!obj || !prop) return MIX_RESULT_NULL_PTR; \
	if(!MIX_IS_VIDEOCONFIGPARAMSENC_H264(obj)) return MIX_RESULT_FAIL; \
 

MIX_RESULT mix_videoconfigparamsenc_h264_set_bus (
    MixVideoConfigParamsEncH264 * obj, uint basic_unit_size) {
    MIX_VIDEOCONFIGPARAMSENC_H264_SETTER_CHECK_INPUT (obj);
    obj->basic_unit_size = basic_unit_size;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_h264_get_bus (
    MixVideoConfigParamsEncH264 * obj, uint * basic_unit_size) {
    MIX_VIDEOCONFIGPARAMSENC_H264_GETTER_CHECK_INPUT (obj, basic_unit_size);
    *basic_unit_size = obj->basic_unit_size;
    return MIX_RESULT_SUCCESS;
}


MIX_RESULT mix_videoconfigparamsenc_h264_set_dlk (
    MixVideoConfigParamsEncH264 * obj, uint disable_deblocking_filter_idc) {
    MIX_VIDEOCONFIGPARAMSENC_H264_SETTER_CHECK_INPUT (obj);
    obj->disable_deblocking_filter_idc = disable_deblocking_filter_idc;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_h264_get_dlk (
    MixVideoConfigParamsEncH264 * obj, uint * disable_deblocking_filter_idc) {
    MIX_VIDEOCONFIGPARAMSENC_H264_GETTER_CHECK_INPUT (obj, disable_deblocking_filter_idc);
    *disable_deblocking_filter_idc = obj->disable_deblocking_filter_idc;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_h264_set_vui_flag (
    MixVideoConfigParamsEncH264 * obj, uint8 vui_flag) {
    MIX_VIDEOCONFIGPARAMSENC_H264_SETTER_CHECK_INPUT (obj);
    obj->vui_flag = vui_flag;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_h264_get_vui_flag (
    MixVideoConfigParamsEncH264 * obj, uint8 * vui_flag) {
    MIX_VIDEOCONFIGPARAMSENC_H264_GETTER_CHECK_INPUT (obj, vui_flag);
    *vui_flag = obj->vui_flag;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_h264_set_slice_num(
    MixVideoConfigParamsEncH264 * obj, uint slice_num) {
    MIX_VIDEOCONFIGPARAMSENC_H264_SETTER_CHECK_INPUT (obj);
    obj->slice_num = slice_num;
    obj->I_slice_num = slice_num;
    obj->P_slice_num = slice_num;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_h264_get_slice_num(
    MixVideoConfigParamsEncH264 * obj, uint * slice_num) {
    MIX_VIDEOCONFIGPARAMSENC_H264_GETTER_CHECK_INPUT (obj, slice_num);
    *slice_num = obj->slice_num;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_h264_set_I_slice_num(
    MixVideoConfigParamsEncH264 * obj, uint I_slice_num) {
    MIX_VIDEOCONFIGPARAMSENC_H264_SETTER_CHECK_INPUT (obj);
    obj->I_slice_num = I_slice_num;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_h264_get_I_slice_num(
    MixVideoConfigParamsEncH264 * obj, uint * I_slice_num) {
    MIX_VIDEOCONFIGPARAMSENC_H264_GETTER_CHECK_INPUT (obj, I_slice_num);
    *I_slice_num = obj->I_slice_num;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_h264_set_P_slice_num(
    MixVideoConfigParamsEncH264 * obj, uint P_slice_num) {
    MIX_VIDEOCONFIGPARAMSENC_H264_SETTER_CHECK_INPUT (obj);
    obj->P_slice_num = P_slice_num;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_h264_get_P_slice_num(
    MixVideoConfigParamsEncH264 * obj, uint * P_slice_num) {
    MIX_VIDEOCONFIGPARAMSENC_H264_GETTER_CHECK_INPUT (obj, P_slice_num);
    *P_slice_num = obj->P_slice_num;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_h264_set_delimiter_type (
    MixVideoConfigParamsEncH264 * obj, MixDelimiterType delimiter_type) {
    MIX_VIDEOCONFIGPARAMSENC_H264_SETTER_CHECK_INPUT (obj);
    obj->delimiter_type = delimiter_type;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_h264_get_delimiter_type (
    MixVideoConfigParamsEncH264 * obj, MixDelimiterType * delimiter_type) {
    MIX_VIDEOCONFIGPARAMSENC_H264_GETTER_CHECK_INPUT (obj, delimiter_type);
    *delimiter_type = obj->delimiter_type;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_h264_set_IDR_interval (
    MixVideoConfigParamsEncH264 * obj, uint idr_interval) {
    MIX_VIDEOCONFIGPARAMSENC_H264_SETTER_CHECK_INPUT (obj);
    obj->idr_interval = idr_interval;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_h264_get_IDR_interval (
    MixVideoConfigParamsEncH264 * obj, uint * idr_interval) {
    MIX_VIDEOCONFIGPARAMSENC_H264_GETTER_CHECK_INPUT (obj, idr_interval);
    *idr_interval = obj->idr_interval;
    return MIX_RESULT_SUCCESS;
}
