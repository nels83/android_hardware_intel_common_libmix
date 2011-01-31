/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

/**
 * SECTION:mixvideodecodeparams
 * @short_description: MI-X Video Decode Paramters
 *
 * The #MixVideoDecodeParams object will be created by the MMF/App 
 * and provided to MixVideo in the MixVideo mix_video_decode() function.
 */

#include "mixvideodecodeparams.h"

#define MIX_VIDEODECODEPARAMS_SETTER_CHECK_INPUT(obj) \
	if(!obj) return MIX_RESULT_NULL_PTR; \
	if(!MIX_IS_VIDEODECODEPARAMS(obj)) return MIX_RESULT_FAIL; \

#define MIX_VIDEODECODEPARAMS_GETTER_CHECK_INPUT(obj, prop) \
	if(!obj || !prop) return MIX_RESULT_NULL_PTR; \
	if(!MIX_IS_VIDEODECODEPARAMS(obj)) return MIX_RESULT_FAIL; \


MixVideoDecodeParams::MixVideoDecodeParams()
	:timestamp(0)
	,discontinuity(FALSE)
	,new_sequence(FALSE)
	,reserved1(NULL)
	,reserved2(NULL)
	,reserved3(NULL)
	,reserved4(NULL) {
}

MixVideoDecodeParams::~MixVideoDecodeParams() {
}

gboolean MixVideoDecodeParams::copy(MixParams *target) const {
	gboolean ret = FALSE;
	MixVideoDecodeParams * this_target = MIX_VIDEODECODEPARAMS(target);
	if (NULL != this_target) {
		// chain up base class
		ret = MixParams::copy(target);
	}
	return ret;
}

gboolean MixVideoDecodeParams::equal(MixParams* obj) const {
	gboolean ret = FALSE;
	MixVideoDecodeParams * this_obj = MIX_VIDEODECODEPARAMS(obj);
	if (NULL != this_obj)
		ret = MixParams::equal(this_obj);
	return ret;
}

MixParams* MixVideoDecodeParams::dup() const {
	MixParams *ret = new MixVideoDecodeParams();
	if (NULL != ret) {
		if (FALSE == copy(ret)) {
			ret->Unref();
			ret = NULL;
		}
	}
	return ret;
}

MixVideoDecodeParams * mix_videodecodeparams_new(void) {
	return new MixVideoDecodeParams();
}

MixVideoDecodeParams *
mix_videodecodeparams_ref(MixVideoDecodeParams * mix) {
	if (NULL != mix)
		mix->Ref();
	return mix;
}


/* TODO: Add getters and setters for properties. */

MIX_RESULT mix_videodecodeparams_set_timestamp(
	MixVideoDecodeParams * obj, guint64 timestamp) {
	MIX_VIDEODECODEPARAMS_SETTER_CHECK_INPUT (obj);
	obj->timestamp = timestamp;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videodecodeparams_get_timestamp(
	MixVideoDecodeParams * obj, guint64 * timestamp) {
	MIX_VIDEODECODEPARAMS_GETTER_CHECK_INPUT (obj, timestamp);
	*timestamp = obj->timestamp;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videodecodeparams_set_discontinuity(
	MixVideoDecodeParams * obj, gboolean discontinuity) {
	MIX_VIDEODECODEPARAMS_SETTER_CHECK_INPUT (obj);
	obj->discontinuity = discontinuity;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videodecodeparams_get_discontinuity(
	MixVideoDecodeParams * obj, gboolean *discontinuity) {
	MIX_VIDEODECODEPARAMS_GETTER_CHECK_INPUT (obj, discontinuity);
	*discontinuity = obj->discontinuity;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videodecodeparams_set_new_sequence(
	MixVideoDecodeParams * obj, gboolean new_sequence) {
	MIX_VIDEODECODEPARAMS_SETTER_CHECK_INPUT (obj);
	obj->new_sequence = new_sequence;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videodecodeparams_get_new_sequence(
	MixVideoDecodeParams * obj, gboolean *new_sequence) {
	MIX_VIDEODECODEPARAMS_GETTER_CHECK_INPUT (obj, new_sequence);
	*new_sequence = obj->new_sequence;
	return MIX_RESULT_SUCCESS;
}

