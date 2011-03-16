/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

/**
 * SECTION:mixvideoconfigparamsdec_vc1
 * @short_description: MI-X Video VC-1 Decode Configuration Parameter
 *
 * MI-X video VC-1 decode configuration parameter objects.
 */


#include "mixvideoconfigparamsdec_vc1.h"

MixVideoConfigParamsDecVC1::MixVideoConfigParamsDecVC1()
        :reserved1(NULL)
        ,reserved2(NULL)
        ,reserved3(NULL)
        ,reserved4(NULL) {
}

MixVideoConfigParamsDecVC1::~MixVideoConfigParamsDecVC1() {
}

bool MixVideoConfigParamsDecVC1::copy(MixParams *target) const {
    bool ret = FALSE;
    MixVideoConfigParamsDecVC1 * this_target = MIX_VIDEOCONFIGPARAMSDEC_VC1(target);
    if (NULL != this_target) {
        ret = MixVideoConfigParamsDec::copy(target);
    }
    return ret;
}

bool MixVideoConfigParamsDecVC1::equal(MixParams* obj) const {
    bool ret = FALSE;
    MixVideoConfigParamsDecVC1 * this_obj = MIX_VIDEOCONFIGPARAMSDEC_VC1(obj);
    if (NULL != this_obj)
        ret = MixVideoConfigParamsDec::equal(this_obj);
    return ret;
}

MixParams* MixVideoConfigParamsDecVC1::dup() const {
    MixParams *ret = new MixVideoConfigParamsDecVC1();
    if (NULL != ret) {
        if (FALSE == copy(ret)) {
            ret->Unref();
            ret = NULL;
        }
    }
    return ret;
}

MixVideoConfigParamsDecVC1 *
mix_videoconfigparamsdec_vc1_new(void) {
    return new MixVideoConfigParamsDecVC1();
}

MixVideoConfigParamsDecVC1 *
mix_videoconfigparamsdec_vc1_ref(
    MixVideoConfigParamsDecVC1 * mix) {
    if (NULL != mix)
        mix->Ref();
    return mix;
}


/* TODO: Add getters and setters for properties if any */
