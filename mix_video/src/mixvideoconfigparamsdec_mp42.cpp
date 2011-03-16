/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

/**
 * SECTION:mixvideoconfigparamsdec_mp42
 * @short_description: MI-X Video MPEG 4:2 Decode Configuration Parameter
 *
 * MI-X video MPEG 4:2 decode configuration parameter objects.
 */


#include "mixvideolog.h"
#include "mixvideoconfigparamsdec_mp42.h"

#define MIX_VIDEOCONFIGPARAMSDEC_MP42_SETTER_CHECK_INPUT(obj) \
	if(!obj) return MIX_RESULT_NULL_PTR; \
	if(!MIX_IS_VIDEOCONFIGPARAMSDEC_MP42(obj)) return MIX_RESULT_FAIL; \
 
#define MIX_VIDEOCONFIGPARAMSDEC_MP42_GETTER_CHECK_INPUT(obj, prop) \
	if(!obj || !prop) return MIX_RESULT_NULL_PTR; \
	if(!MIX_IS_VIDEOCONFIGPARAMSDEC_MP42(obj)) return MIX_RESULT_FAIL; \
 
MixVideoConfigParamsDecMP42::MixVideoConfigParamsDecMP42()
        :mpegversion(0)
        ,divxversion(0)
        ,reserved1(NULL)
        ,reserved2(NULL)
        ,reserved3(NULL)
        ,reserved4(NULL) {
}

MixVideoConfigParamsDecMP42::~MixVideoConfigParamsDecMP42() {
}

bool MixVideoConfigParamsDecMP42::copy(MixParams *target) const {
    bool ret = FALSE;
    MixVideoConfigParamsDecMP42 * this_target = MIX_VIDEOCONFIGPARAMSDEC_MP42(target);
    if (NULL != this_target) {
        this_target->mpegversion = this->mpegversion;
        this_target->divxversion = this->divxversion;
        ret = MixVideoConfigParamsDec::copy(target);
    }
    return ret;
}

bool MixVideoConfigParamsDecMP42::equal(MixParams* obj) const {
    bool ret = FALSE;
    MixVideoConfigParamsDecMP42 * this_obj = MIX_VIDEOCONFIGPARAMSDEC_MP42(obj);
    if (NULL != this_obj)
        ret = MixVideoConfigParamsDec::equal(this_obj);
    return ret;
}

MixParams* MixVideoConfigParamsDecMP42::dup() const {
    MixParams *ret = new MixVideoConfigParamsDecMP42();
    if (NULL != ret) {
        if (FALSE == copy(ret)) {
            ret->Unref();
            ret = NULL;
        }
    }
    return ret;
}


MixVideoConfigParamsDecMP42 *
mix_videoconfigparamsdec_mp42_new(void) {
    return new MixVideoConfigParamsDecMP42();
}

MixVideoConfigParamsDecMP42 *
mix_videoconfigparamsdec_mp42_ref(MixVideoConfigParamsDecMP42 * mix) {
    if (NULL != mix)
        mix->Ref();
    return mix;
}

/* TODO: Add getters and setters for properties if any */
MIX_RESULT mix_videoconfigparamsdec_mp42_set_mpegversion(
    MixVideoConfigParamsDecMP42 *obj, uint version) {
    MIX_VIDEOCONFIGPARAMSDEC_MP42_SETTER_CHECK_INPUT (obj);
    obj->mpegversion = version;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsdec_mp42_get_mpegversion(
    MixVideoConfigParamsDecMP42 *obj, uint *version) {
    MIX_VIDEOCONFIGPARAMSDEC_MP42_GETTER_CHECK_INPUT (obj, version);
    *version = obj->mpegversion;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsdec_mp42_set_divxversion(
    MixVideoConfigParamsDecMP42 *obj, uint version) {
    MIX_VIDEOCONFIGPARAMSDEC_MP42_SETTER_CHECK_INPUT (obj);
    obj->divxversion = version;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsdec_mp42_get_divxversion(
    MixVideoConfigParamsDecMP42 *obj, uint *version) {
    MIX_VIDEOCONFIGPARAMSDEC_MP42_GETTER_CHECK_INPUT (obj, version);
    *version = obj->divxversion;
    return MIX_RESULT_SUCCESS;

}

