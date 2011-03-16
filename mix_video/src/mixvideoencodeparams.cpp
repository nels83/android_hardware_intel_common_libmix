/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

/**
 * SECTION:mixvideoencodeparams
 * @short_description: MI-X Video Encode Parameters
 *
 * The #MixVideoEncodeParams object will be created by
 * the MMF/App and provided to #MixVideo in the #MixVideo
 * mix_video_encode() function. Get methods for the
 * properties will be available for the caller to
 * retrieve configuration information. Currently this
 * object is reserved for future use.
 */

#include "mixvideoencodeparams.h"

#define MIX_VIDEOENCODEPARAMS_SETTER_CHECK_INPUT(obj) \
	if(!obj) return MIX_RESULT_NULL_PTR; \
	if(!MIX_IS_VIDEOENCODEPARAMS(obj)) return MIX_RESULT_FAIL; \
 
#define MIX_VIDEOENCODEPARAMS_GETTER_CHECK_INPUT(obj, prop) \
	if(!obj || !prop) return MIX_RESULT_NULL_PTR; \
	if(!MIX_IS_VIDEOENCODEPARAMS(obj)) return MIX_RESULT_FAIL; \
 

MixVideoEncodeParams::MixVideoEncodeParams()
        :timestamp(0)
        ,discontinuity(FALSE)
        ,reserved1(NULL)
        ,reserved2(NULL)
        ,reserved3(NULL)
        ,reserved4(NULL) {
}

MixVideoEncodeParams::~MixVideoEncodeParams() {
}


MixVideoEncodeParams *
mix_videoencodeparams_new(void) {
    return new MixVideoEncodeParams();
}


MixVideoEncodeParams *
mix_videoencodeparams_ref(MixVideoEncodeParams * mix) {
    if (NULL != mix)
        mix->Ref();
    return mix;
}

/**
 * dup:
 * @returns: a newly allocated duplicate of the object.
 *
 * Copy duplicate of the object.
 */
MixParams *MixVideoEncodeParams::dup() const {
    MixParams *ret  = new MixVideoEncodeParams();
    if (NULL != ret) {
        if (FALSE == copy(ret)) {
            ret->Unref();
            ret = NULL;
        }
    }
    return ret;
}

/**
 * copy:
 * @target: copy to target
 * @returns: boolean indicates if copy is successful.
 *
 * Copy instance data from @src to @target.
 */
bool MixVideoEncodeParams::copy(MixParams * target) const {
    bool ret = FALSE;
    MixVideoEncodeParams *this_target = MIX_VIDEOENCODEPARAMS(target);
    if (NULL!= this_target) {
        // chain up base class
        ret = MixParams::copy(this_target);
    }
    return ret;
}

/**
 * equal:
 * @obj: the object to compare
 * @returns: boolean indicates if instance are equal.
 *
 * Copy instance data from @src to @target.
 */
bool   MixVideoEncodeParams::equal(MixParams * obj) const {
    bool ret = FALSE;
    MixVideoEncodeParams * this_obj = MIX_VIDEOENCODEPARAMS(obj);
    if (NULL != this_obj)
        ret = MixParams::equal(this_obj);
    return ret;
}



/* TODO: Add getters and setters for properties. */

MIX_RESULT mix_videoencodeparams_set_timestamp(
    MixVideoEncodeParams * obj, uint64 timestamp) {
    MIX_VIDEOENCODEPARAMS_SETTER_CHECK_INPUT (obj);
    obj->timestamp = timestamp;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoencodeparams_get_timestamp(
    MixVideoEncodeParams * obj, uint64 * timestamp) {
    MIX_VIDEOENCODEPARAMS_GETTER_CHECK_INPUT (obj, timestamp);
    *timestamp = obj->timestamp;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoencodeparams_set_discontinuity(
    MixVideoEncodeParams * obj, bool discontinuity) {
    MIX_VIDEOENCODEPARAMS_SETTER_CHECK_INPUT (obj);
    obj->discontinuity = discontinuity;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoencodeparams_get_discontinuity(
    MixVideoEncodeParams * obj, bool *discontinuity) {
    MIX_VIDEOENCODEPARAMS_GETTER_CHECK_INPUT (obj, discontinuity);
    *discontinuity = obj->discontinuity;
    return MIX_RESULT_SUCCESS;
}

