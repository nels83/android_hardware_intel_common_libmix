/*
INTEL CONFIDENTIAL
Copyright 2009 Intel Corporation All Rights Reserved.
The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
*/

/**
* SECTION:mixvideoconfigparamsenc_preview
* @short_description: VideoConfig parameters
*
* A data object which stores videoconfig specific parameters.
*/

#include "mixvideolog.h"
#include "mixvideoconfigparamsenc_preview.h"

#define MDEBUG

MixVideoConfigParamsEncPreview::MixVideoConfigParamsEncPreview() {
}

MixParams* MixVideoConfigParamsEncPreview::dup() const {
    MixParams *ret = NULL;
    MixVideoConfigParamsEncPreview *duplicate = new MixVideoConfigParamsEncPreview();
    if (TRUE == copy(duplicate)) {
        ret = duplicate;
    } else {
        if (NULL != duplicate)
            duplicate->Unref();
    }
    return ret;
}

MixVideoConfigParamsEncPreview *
mix_videoconfigparamsenc_preview_new (void) {
    return new MixVideoConfigParamsEncPreview();
}

MixVideoConfigParamsEncPreview*
mix_videoconfigparamsenc_preview_ref (MixVideoConfigParamsEncPreview * mix) {
    if (NULL != mix)
        mix->Ref();
    return mix;

}
