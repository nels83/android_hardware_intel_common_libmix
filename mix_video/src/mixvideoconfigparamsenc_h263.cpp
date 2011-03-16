/*
INTEL CONFIDENTIAL
Copyright 2009 Intel Corporation All Rights Reserved.
The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
*/

/**
 * SECTION:mixvideoconfigparamsenc_h263
 * @short_description: MI-X Video H.263 Eecode Configuration Parameter
 *
 * MI-X video H.263 eecode configuration parameter objects.
 */


#include "mixvideolog.h"
#include "mixvideoconfigparamsenc_h263.h"

#define MDEBUG

MixVideoConfigParamsEncH263::MixVideoConfigParamsEncH263()
        :slice_num(1)
        ,disable_deblocking_filter_idc(0)
        ,reserved1(NULL)
        ,reserved2(NULL)
        ,reserved3(NULL)
        ,reserved4(NULL) {
}
MixVideoConfigParamsEncH263:: ~MixVideoConfigParamsEncH263() {
}

bool MixVideoConfigParamsEncH263:: copy(MixParams *target) const {
    bool ret = FALSE;
    MixVideoConfigParamsEncH263 * this_target = MIX_VIDEOCONFIGPARAMSENC_H263(target);
    if (NULL != this_target) {
        //add properties
        this_target->slice_num = slice_num;
        this_target->disable_deblocking_filter_idc = disable_deblocking_filter_idc;
        ret = MixVideoConfigParamsEnc::copy(target);
    }
    return ret;
}

bool MixVideoConfigParamsEncH263:: equal(MixParams* obj) const {
    bool ret = FALSE;
    MixVideoConfigParamsEncH263 * this_first = MIX_VIDEOCONFIGPARAMSENC_H263(obj);

    if (this_first->slice_num !=slice_num) {
        goto not_equal;
    }

    if (this_first->disable_deblocking_filter_idc !=disable_deblocking_filter_idc) {
        goto not_equal;
    }

    ret = TRUE;

not_equal:

    if (ret != TRUE) {
        return ret;
    }
    ret = MixVideoConfigParamsEnc::equal(obj);
    return ret;
}


MixParams*
MixVideoConfigParamsEncH263::dup() const {
    MixParams *ret = new MixVideoConfigParamsEncH263();
    if (NULL != ret) {
        if (FALSE == copy(ret)) {
            ret->Unref();
            ret = NULL;
        }
    }
    return ret;
}

MixVideoConfigParamsEncH263 *
mix_videoconfigparamsenc_h263_new (void) {
    return new MixVideoConfigParamsEncH263();
}


MixVideoConfigParamsEncH263*
mix_videoconfigparamsenc_h263_ref (MixVideoConfigParamsEncH263 * mix) {
    if (NULL != mix)
        mix->Ref();
    return mix;
}


/* TODO: Add getters and setters for properties if any */

#define MIX_VIDEOCONFIGPARAMSENC_H263_SETTER_CHECK_INPUT(obj) \
	if(!obj) return MIX_RESULT_NULL_PTR; \
	if(!MIX_IS_VIDEOCONFIGPARAMSENC_H263(obj)) return MIX_RESULT_FAIL; \
 
#define MIX_VIDEOCONFIGPARAMSENC_H263_GETTER_CHECK_INPUT(obj, prop) \
	if(!obj || !prop) return MIX_RESULT_NULL_PTR; \
	if(!MIX_IS_VIDEOCONFIGPARAMSENC_H263(obj)) return MIX_RESULT_FAIL; \
 

MIX_RESULT mix_videoconfigparamsenc_h263_set_slice_num (
    MixVideoConfigParamsEncH263 * obj, uint slice_num) {
    MIX_VIDEOCONFIGPARAMSENC_H263_SETTER_CHECK_INPUT (obj);
    obj->slice_num = slice_num;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_h263_get_slice_num (
    MixVideoConfigParamsEncH263 * obj, uint * slice_num) {
    MIX_VIDEOCONFIGPARAMSENC_H263_GETTER_CHECK_INPUT (obj, slice_num);
    *slice_num = obj->slice_num;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_h263_set_dlk (
    MixVideoConfigParamsEncH263 * obj, uint disable_deblocking_filter_idc) {
    MIX_VIDEOCONFIGPARAMSENC_H263_SETTER_CHECK_INPUT (obj);
    obj->disable_deblocking_filter_idc = disable_deblocking_filter_idc;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_h263_get_dlk (
    MixVideoConfigParamsEncH263 * obj, uint * disable_deblocking_filter_idc) {
    MIX_VIDEOCONFIGPARAMSENC_H263_GETTER_CHECK_INPUT (obj, disable_deblocking_filter_idc);
    *disable_deblocking_filter_idc = obj->disable_deblocking_filter_idc;
    return MIX_RESULT_SUCCESS;
}

