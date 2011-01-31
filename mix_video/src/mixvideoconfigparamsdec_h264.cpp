/* 
INTEL CONFIDENTIAL
Copyright 2009 Intel Corporation All Rights Reserved. 
The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
*/

/**
 * SECTION:mixvideoconfigparamsdec_h264
 * @short_description: MI-X Video H.264 Decode Configuration Parameter
 *
 * MI-X video H.264 decode configuration parameter objects.
 */

#include "mixvideoconfigparamsdec_h264.h"

MixVideoConfigParamsDecH264::MixVideoConfigParamsDecH264()
	:reserved1(NULL)
	,reserved2(NULL)
	,reserved3(NULL)
	,reserved4(NULL) {
}
MixVideoConfigParamsDecH264::~MixVideoConfigParamsDecH264() {
}

gboolean MixVideoConfigParamsDecH264::copy(MixParams *target) const {
	gboolean ret = FALSE;
	MixVideoConfigParamsDecH264 * this_target = MIX_VIDEOCONFIGPARAMSDEC_H264(target);
	if (NULL != this_target)
		ret = MixVideoConfigParamsDec::copy(target);
	return ret;
}

gboolean MixVideoConfigParamsDecH264::equal(MixParams* obj) const {
	gboolean ret = FALSE;
	MixVideoConfigParamsDecH264 * this_obj = MIX_VIDEOCONFIGPARAMSDEC_H264(obj);
	if (NULL != this_obj)
		ret = MixVideoConfigParamsDec::equal(this_obj);
	return ret;
}

MixParams* MixVideoConfigParamsDecH264::dup() const {
	MixParams *ret = new MixVideoConfigParamsDecH264();
	if (NULL != ret) {
		if (FALSE == copy(ret)) {
			ret->Unref();
			ret = NULL;
		}
	}
	return ret;
}

MixVideoConfigParamsDecH264 *
mix_videoconfigparamsdec_h264_new (void) {
	return new MixVideoConfigParamsDecH264();
}

MixVideoConfigParamsDecH264*
mix_videoconfigparamsdec_h264_ref (MixVideoConfigParamsDecH264 * mix) {
	if (NULL != mix)
		mix->Ref();
	return mix;
}

/* TODO: Add getters and setters for properties if any */
