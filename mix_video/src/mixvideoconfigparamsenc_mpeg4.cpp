/*
INTEL CONFIDENTIAL
Copyright 2009 Intel Corporation All Rights Reserved.
The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
*/

/**
 * SECTION:mixvideoconfigparamsenc_mpeg4
 * @short_description: MI-X Video MPEG 4:2 Eecode Configuration Parameter
 *
 * MI-X video MPEG 4:2 eecode configuration parameter objects.
 */


#include "mixvideolog.h"
#include "mixvideoconfigparamsenc_mpeg4.h"

#define MDEBUG

MixVideoConfigParamsEncMPEG4::MixVideoConfigParamsEncMPEG4()
        :profile_and_level_indication(3)
        ,fixed_vop_time_increment(3)
        ,disable_deblocking_filter_idc(0)
        ,reserved1(NULL)
        ,reserved2(NULL)
        ,reserved3(NULL)
        ,reserved4(NULL) {
}

MixVideoConfigParamsEncMPEG4::~MixVideoConfigParamsEncMPEG4() {
}


bool MixVideoConfigParamsEncMPEG4::copy(MixParams *target) const {
    bool ret = FALSE;
    MixVideoConfigParamsEncMPEG4 *this_target = MIX_VIDEOCONFIGPARAMSENC_MPEG4(target);
    if (NULL != this_target) {
        //add properties
        this_target->profile_and_level_indication= profile_and_level_indication;
        this_target->fixed_vop_time_increment= fixed_vop_time_increment;
        this_target->disable_deblocking_filter_idc = disable_deblocking_filter_idc;

        // Now chainup base class
        ret = MixVideoConfigParamsEnc::copy(target);
    }
    return ret;
}

bool MixVideoConfigParamsEncMPEG4::equal(MixParams* obj) const {
    bool ret = FALSE;
    MixVideoConfigParamsEncMPEG4 *this_obj = MIX_VIDEOCONFIGPARAMSENC_MPEG4(obj);
    if ((NULL != this_obj) &&
            (profile_and_level_indication == this_obj->profile_and_level_indication) &&
            (fixed_vop_time_increment == this_obj->fixed_vop_time_increment) &&
            (disable_deblocking_filter_idc == this_obj->disable_deblocking_filter_idc)) {
        ret = MixVideoConfigParamsEnc::equal(obj);
    }
    return ret;
}

MixParams* MixVideoConfigParamsEncMPEG4::dup() const {
    MixParams *ret = NULL;
    MixVideoConfigParamsEncMPEG4 *duplicate = new MixVideoConfigParamsEncMPEG4();
    if (TRUE == copy(duplicate)) {
        ret = duplicate;
    } else {
        if (NULL != duplicate)
            duplicate->Unref();
    }
    return ret;
}


MixVideoConfigParamsEncMPEG4 *
mix_videoconfigparamsenc_mpeg4_new (void) {
    return new MixVideoConfigParamsEncMPEG4();
}


MixVideoConfigParamsEncMPEG4 *
mix_videoconfigparamsenc_mpeg4_ref (MixVideoConfigParamsEncMPEG4 * mix) {
    if (NULL != mix)
        mix->Ref();
    return mix;
}

/* TODO: Add getters and setters for properties if any */

#define MIX_VIDEOCONFIGPARAMSENC_MPEG4_SETTER_CHECK_INPUT(obj) \
	if(!obj) return MIX_RESULT_NULL_PTR; \
	if(!MIX_IS_VIDEOCONFIGPARAMSENC_MPEG4(obj)) return MIX_RESULT_FAIL; \
 
#define MIX_VIDEOCONFIGPARAMSENC_MPEG4_GETTER_CHECK_INPUT(obj, prop) \
	if(!obj || !prop) return MIX_RESULT_NULL_PTR; \
	if(!MIX_IS_VIDEOCONFIGPARAMSENC_MPEG4(obj)) return MIX_RESULT_FAIL; \
 

MIX_RESULT mix_videoconfigparamsenc_mpeg4_set_profile_level (
    MixVideoConfigParamsEncMPEG4 * obj, uchar profile_and_level_indication) {
    MIX_VIDEOCONFIGPARAMSENC_MPEG4_SETTER_CHECK_INPUT (obj);
    obj->profile_and_level_indication = profile_and_level_indication;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_mpeg4_get_profile_level (
    MixVideoConfigParamsEncMPEG4 * obj, uchar * profile_and_level_indication) {
    MIX_VIDEOCONFIGPARAMSENC_MPEG4_GETTER_CHECK_INPUT (obj, profile_and_level_indication);
    *profile_and_level_indication = obj->profile_and_level_indication;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_mpeg4_set_fixed_vti (
    MixVideoConfigParamsEncMPEG4 * obj, uint fixed_vop_time_increment) {
    MIX_VIDEOCONFIGPARAMSENC_MPEG4_SETTER_CHECK_INPUT (obj);
    obj->fixed_vop_time_increment = fixed_vop_time_increment;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_mpeg4_get_fixed_vti (
    MixVideoConfigParamsEncMPEG4 * obj, uint * fixed_vop_time_increment) {
    MIX_VIDEOCONFIGPARAMSENC_MPEG4_GETTER_CHECK_INPUT (obj, fixed_vop_time_increment);
    *fixed_vop_time_increment = obj->fixed_vop_time_increment;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_mpeg4_set_dlk (
    MixVideoConfigParamsEncMPEG4 * obj, uint disable_deblocking_filter_idc) {
    MIX_VIDEOCONFIGPARAMSENC_MPEG4_SETTER_CHECK_INPUT (obj);
    obj->disable_deblocking_filter_idc = disable_deblocking_filter_idc;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_mpeg4_get_dlk (
    MixVideoConfigParamsEncMPEG4 * obj, uint * disable_deblocking_filter_idc) {
    MIX_VIDEOCONFIGPARAMSENC_MPEG4_GETTER_CHECK_INPUT (obj, disable_deblocking_filter_idc);
    *disable_deblocking_filter_idc = obj->disable_deblocking_filter_idc;
    return MIX_RESULT_SUCCESS;
}

