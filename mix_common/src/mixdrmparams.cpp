/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
*/

/**
 * SECTION:mixdrmparams
 * @short_description: Drm parameters
 *
 * A data object which stores drm specific parameters.
 */

#include "mixdrmparams.h"


MixDrmParams::MixDrmParams() {
}

MixDrmParams::~MixDrmParams() {
}

MixDrmParams *mix_drmparams_new(void) {
    return new MixDrmParams();
}

MixDrmParams *mix_drmparams_ref(MixDrmParams *mix) {
    return (MixDrmParams*)mix_params_ref(MIX_PARAMS(mix));
}

MixParams * MixDrmParams::dup () const {
    MixParams* dup = new MixDrmParams();
    if (NULL != dup) {
        if (FALSE == copy(dup)) {
            dup->Unref();
            dup = NULL;
        }
    }
    return dup;
}



