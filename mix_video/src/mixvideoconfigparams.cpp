/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

/**
 * SECTION:mixvideoconfigparams
 * @short_description: MI-X Video Configuration Parameter Base Object
 * @include: mixvideoconfigparams.h
 *
 * <para>
 * A base object of MI-X video configuration parameter objects.
 * </para>
 * <para>
 * The derived MixVideoConfigParams object is created by the MMF/App
 * and provided in the MixVideo mix_video_configure() function. The get and set
 * methods for the properties will be available for the caller to set and get information at
 * configuration time. It will also be created by MixVideo and returned from the
 * mix_video_get_config() function, whereupon the MMF/App can get the get methods to
 * obtain current configuration information.
 * </para>
 * <para>
 * There are decode mode objects (for example, MixVideoConfigParamsDec) and encode
 * mode objects (for example, MixVideoConfigParamsEnc). Each of these types is refined
 * further with media specific objects. The application should create the correct type of
 * object to match the media format of the stream to be handled, e.g. if the media
 * format of the stream to be decoded is H.264, the application would create a
 * MixVideoConfigParamsDecH264 object for the mix_video_configure() call.
 * </para>
 */

#include <string.h>
#include "mixvideolog.h"
#include "mixvideoconfigparams.h"

MixVideoConfigParams::MixVideoConfigParams()
        :reserved1(NULL)
        ,reserved2(NULL)
        ,reserved3(NULL)
        ,reserved4(NULL) {
}
MixVideoConfigParams::~MixVideoConfigParams() {
}
bool MixVideoConfigParams::copy(MixParams *target) const {
    bool ret = FALSE;
    MixVideoConfigParams * this_target = MIX_VIDEOCONFIGPARAMS(target);
    if (NULL != this_target)
        ret = MixParams::copy(target);
    return ret;
}

bool MixVideoConfigParams::equal(MixParams* obj) const {
    bool ret = FALSE;
    MixVideoConfigParams * this_obj = MIX_VIDEOCONFIGPARAMS(obj);
    if (NULL != this_obj)
        ret = MixParams::equal(this_obj);
    return ret;
}

MixParams* MixVideoConfigParams::dup() const {
    MixParams *ret = new MixVideoConfigParams();
    if (NULL != ret) {
        if (FALSE == copy(ret)) {
            ret->Unref();
            ret = NULL;
        }
    }
    return ret;
}


MixVideoConfigParams *
mix_videoconfigparams_new(void) {
    return new MixVideoConfigParams();
}
MixVideoConfigParams *
mix_videoconfigparams_ref(MixVideoConfigParams * mix) {
    if (NULL != mix)
        mix->Ref();
    return mix;
}


