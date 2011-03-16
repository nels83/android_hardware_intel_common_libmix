/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

/**
 * SECTION:mixvideoinitparams
 * @short_description: MI-X Video Initialization Parameters
 *
 * The MixVideoInitParams object will be created by the MMF/App
 * and provided in the mix_video_initialize() function.
 * The get and set methods for the properties will be available for
 * the caller to set and get information used at initialization time.
 */

#include "mixvideoinitparams.h"

#define SAFE_FREE(p) if(p) { free(p); p = NULL; }

#define MIX_VIDEOINITPARAMS_SETTER_CHECK_INPUT(obj) \
	if(!obj) return MIX_RESULT_NULL_PTR; \
	if(!MIX_IS_VIDEOINITPARAMS(obj)) return MIX_RESULT_FAIL; \
 
#define MIX_VIDEOINITPARAMS_GETTER_CHECK_INPUT(obj, prop) \
	if(!obj || !prop) return MIX_RESULT_NULL_PTR; \
	if(!MIX_IS_VIDEOINITPARAMS(obj)) return MIX_RESULT_FAIL; \
 
MixVideoInitParams::MixVideoInitParams()
        :display(NULL)
        ,reserved1(NULL)
        ,reserved2(NULL)
        ,reserved3(NULL)
        ,reserved4(NULL) {
}
MixVideoInitParams::~MixVideoInitParams() {
    /* unref display */
    if (this->display) {
        mix_display_unref(this->display);
        this->display = NULL;
    }
}

bool MixVideoInitParams::copy(MixParams *target) const {
    bool ret = FALSE;
    MixVideoInitParams * this_target = MIX_VIDEOINITPARAMS(target);
    if (NULL != this_target) {
        /* duplicate display */
        this_target->display = mix_display_dup(this->display);
        // chain up base class
        ret = MixParams::copy(target);
    }
    return ret;
}

bool MixVideoInitParams::equal(MixParams* obj) const {
    bool ret = FALSE;
    MixVideoInitParams * this_obj = MIX_VIDEOINITPARAMS(obj);
    if (NULL != this_obj) {
        /* TODO: add comparison for other properties */
        if ((NULL == this->display && NULL == this_obj->display) ||
                mix_display_equal(this->display, this_obj->display)) {
            ret = MixParams::equal(this_obj);
        }
    }
    return ret;
}

MixParams* MixVideoInitParams::dup() const {
    MixParams *ret = new MixVideoInitParams();
    if (NULL != ret) {
        if (FALSE == copy(ret)) {
            ret->Unref();
            ret = NULL;
        }
    }
    return ret;
}

MixVideoInitParams * mix_videoinitparams_new(void) {
    return new MixVideoInitParams();
}

MixVideoInitParams *
mix_videoinitparams_ref(MixVideoInitParams * mix) {
    if (NULL != mix)
        mix->Ref();
    return mix;
}

MIX_RESULT mix_videoinitparams_set_display(
    MixVideoInitParams * obj, MixDisplay * display) {
    MIX_VIDEOINITPARAMS_SETTER_CHECK_INPUT (obj);
    if (obj->display) {
        mix_display_unref(obj->display);
    }
    obj->display = NULL;
    if (display) {
        /*	obj->display = mix_display_dup(display);
        	if(!obj->display) {
        		return MIX_RESULT_NO_MEMORY;
        	}*/

        obj->display = mix_display_ref(display);
    }
    return MIX_RESULT_SUCCESS;
}

/*
 Caller is responsible to use g_free to free the memory
 */
MIX_RESULT mix_videoinitparams_get_display(
    MixVideoInitParams * obj, MixDisplay ** display) {
    MIX_VIDEOINITPARAMS_GETTER_CHECK_INPUT (obj, display);
    *display = NULL;
    if (obj->display) {
        /*	*display = mix_display_dup(obj->display);
        	if(!*display) {
        		return MIX_RESULT_NO_MEMORY;
        	}*/
        *display = mix_display_ref(obj->display);
    }
    return MIX_RESULT_SUCCESS;
}
