/*
INTEL CONFIDENTIAL
Copyright 2009 Intel Corporation All Rights Reserved.
The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
*/

/**
* SECTION:mixvideocaps
* @short_description: VideoConfig parameters
*
* A data object which stores videoconfig specific parameters.
*/

#include <string.h>
#include "mixvideocaps.h"
#include <stdlib.h>


#ifdef ANDROID
#define mix_strcmp strcmp
#else
#define mix_strcmp g_strcmp0
#endif

#define MIX_VIDEOCAPS_SETTER_CHECK_INPUT(obj) \
	if(!obj) return MIX_RESULT_NULL_PTR; \
	if(!MIX_IS_VIDEOCAPS(obj)) return MIX_RESULT_FAIL; \
 
#define MIX_VIDEOCAPS_GETTER_CHECK_INPUT(obj, prop) \
	if(!obj || !prop) return MIX_RESULT_NULL_PTR; \
	if(!MIX_IS_VIDEOCAPS(obj)) return MIX_RESULT_FAIL; \
 

#define SAFE_FREE(p) if(p) {free(p); p = NULL; }

MixVideoCaps::MixVideoCaps()
        :mix_caps(NULL)
        ,video_hw_caps(NULL)
        ,reserved1(NULL)
        ,reserved2(NULL)
        ,reserved3(NULL)
        ,reserved4(NULL) {
}

MixVideoCaps::~MixVideoCaps() {
    SAFE_FREE (this->mix_caps);
    SAFE_FREE (this->video_hw_caps);
}

/**
* mix_videocaps_dup:
* @obj: a #MixVideoCaps object
* @returns: a newly allocated duplicate of the object.
*
* Copy duplicate of the object.
*/
MixParams* MixVideoCaps::dup() const {
    MixParams *ret = new MixVideoCaps();
    if (NULL != ret) {
        if (FALSE == copy(ret)) {
            ret->Unref();
            ret = NULL;
        }
    }
    return ret;
}

/**
* mix_videocaps_copy:
* @target: copy to target
* @src: copy from src
* @returns: boolean indicates if copy is successful.
*
* Copy instance data from @src to @target.
*/
bool MixVideoCaps::copy (MixParams * target) const {
    bool ret = FALSE;
    MixVideoCaps * this_target = MIX_VIDEOCAPS(target);
    if (NULL != this_target) {
        // Free the existing properties
        SAFE_FREE (this_target->mix_caps);
        SAFE_FREE (this_target->video_hw_caps);
        // Duplicate string
        this_target->mix_caps = strdup (this->mix_caps);
        this_target->video_hw_caps = strdup (this->video_hw_caps);

        // chain up base class
        ret = MixParams::copy(target);
    }
    return ret;
}


bool MixVideoCaps::equal(MixParams* obj) const {
    bool ret = FALSE;
    MixVideoCaps * this_obj = MIX_VIDEOCAPS(obj);
    if (NULL != this_obj) {
        if ((mix_strcmp (this->mix_caps, this_obj->mix_caps) == 0) &&
                (mix_strcmp (this->video_hw_caps, this_obj->video_hw_caps) == 0)) {
            ret = MixParams::equal(this_obj);
        }
    }
    return ret;
}



MixVideoCaps *
mix_videocaps_new (void) {
    return new MixVideoCaps();
}

MixVideoCaps *
mix_videocaps_ref (MixVideoCaps * mix) {
    if (NULL != mix)
        mix->Ref();
    return mix;
}


/* TODO: Add getters and setters for other properties. The following is just an exmaple, not implemented yet. */
MIX_RESULT mix_videocaps_set_mix_caps (
    MixVideoCaps * obj, char * mix_caps) {
    MIX_VIDEOCAPS_SETTER_CHECK_INPUT (obj);
    SAFE_FREE (obj->mix_caps);
    obj->mix_caps = strdup (mix_caps);
    if (NULL == obj->mix_caps && NULL != mix_caps) {
        return MIX_RESULT_NO_MEMORY;
    }
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videocaps_get_mix_caps (
    MixVideoCaps * obj, char ** mix_caps) {
    MIX_VIDEOCAPS_GETTER_CHECK_INPUT (obj, mix_caps);
    *mix_caps = strdup (obj->mix_caps);
    if (NULL == *mix_caps && NULL != obj->mix_caps) {
        return MIX_RESULT_NO_MEMORY;
    }
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videocaps_set_video_hw_caps (
    MixVideoCaps * obj, char * video_hw_caps) {
    MIX_VIDEOCAPS_SETTER_CHECK_INPUT (obj);
    SAFE_FREE (obj->video_hw_caps);
    obj->video_hw_caps = strdup (video_hw_caps);
    if (NULL != video_hw_caps && NULL == obj->video_hw_caps) {
        return MIX_RESULT_NO_MEMORY;
    }
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videocaps_get_video_hw_caps (
    MixVideoCaps * obj, char ** video_hw_caps) {
    MIX_VIDEOCAPS_GETTER_CHECK_INPUT (obj, video_hw_caps);
    *video_hw_caps = strdup (obj->video_hw_caps);
    if (NULL == *video_hw_caps && NULL != obj->video_hw_caps) {
        return MIX_RESULT_NO_MEMORY;
    }
    return MIX_RESULT_SUCCESS;
}
