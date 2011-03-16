/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

#ifndef __MIX_VIDEODECODEPARAMS_H__
#define __MIX_VIDEODECODEPARAMS_H__

#include <mixparams.h>
#include "mixvideodef.h"

/**
 * MIX_VIDEODECODEPARAMS:
 * @obj: object to be type-casted.
 */
#define MIX_VIDEODECODEPARAMS(obj) (reinterpret_cast<MixVideoDecodeParams*>(obj))

/**
 * MIX_IS_VIDEODECODEPARAMS:
 * @obj: an object.
 *
 * Checks if the given object is an instance of #MixParams
 */
#define MIX_IS_VIDEODECODEPARAMS(obj) ((NULL != MIX_VIDEODECODEPARAMS(obj)) ? TRUE : FALSE)


/**
 * MixVideoDecodeParams:
 *
 * MI-X VideoDecode Parameter object
 */
class MixVideoDecodeParams : public MixParams {
public:
    MixVideoDecodeParams();
    ~MixVideoDecodeParams();
    virtual bool copy(MixParams *target) const;
    virtual bool equal(MixParams* obj) const;
    virtual MixParams* dup() const;
public:
    /*< public > */
    /* TODO: Add properties */

    /* Presentation timestamp for the video
     * frame data, in milliseconds */
    uint64 timestamp;

    /* Indicates a discontinuity in the stream */
    bool discontinuity;

    /* output only, indicate if stream contains a new sequence */
    bool new_sequence;

    /* Reserved for future use */
    void *reserved1;

    /* Reserved for future use */
    void *reserved2;

    /* Reserved for future use */
    void *reserved3;

    /* Reserved for future use */
    void *reserved4;
};

/**
 * mix_videodecodeparams_new:
 * @returns: A newly allocated instance of #MixVideoDecodeParams
 *
 * Use this method to create new instance of #MixVideoDecodeParams
 */
MixVideoDecodeParams *mix_videodecodeparams_new(void);
/**
 * mix_videodecodeparams_ref:
 * @mix: object to add reference
 * @returns: the #MixVideoDecodeParams instance where reference count has been increased.
 *
 * Add reference count.
 */
MixVideoDecodeParams *mix_videodecodeparams_ref(MixVideoDecodeParams * mix);

/**
 * mix_videodecodeparams_unref:
 * @obj: object to unref.
 *
 * Decrement reference count of the object.
 */
#define mix_videodecodeparams_unref(obj) mix_params_unref(MIX_PARAMS(obj))

/* Class Methods */

/* TODO: Add getters and setters for properties */


/**
 * mix_videodecodeparams_set_timestamp:
 * @obj: #MixVideoDecodeParams object
 * @timestamp: Presentation timestamp for the video frame data, in milliseconds
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set Presentation timestamp
 */
MIX_RESULT mix_videodecodeparams_set_timestamp(MixVideoDecodeParams * obj,
        uint64 timestamp);

/**
 * mix_videodecodeparams_get_timestamp:
 * @obj: #MixVideoDecodeParams object
 * @timestamp: Presentation timestamp for the video frame data, in milliseconds to be returned.
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get Presentation timestamp
 */
MIX_RESULT mix_videodecodeparams_get_timestamp(MixVideoDecodeParams * obj,
        uint64 * timestamp);

/**
 * mix_videodecodeparams_set_discontinuity:
 * @obj: #MixVideoDecodeParams object
 * @discontinuity: Flag to indicate a discontinuity in the stream.
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set discontinuity flag
 */
MIX_RESULT mix_videodecodeparams_set_discontinuity(MixVideoDecodeParams * obj,
        bool discontinuity);


/**
 * mix_videodecodeparams_get_discontinuity:
 * @obj: #MixVideoDecodeParams object
 * @discontinuity: Discontinuity flag to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get discontinuity flag
 */
MIX_RESULT mix_videodecodeparams_get_discontinuity(MixVideoDecodeParams * obj,
        bool *discontinuity);


/**
 * mix_videodecodeparams_set_new_sequence:
 * @obj: #MixVideoDecodeParams object
 * @new_sequence: Flag to indicate if stream contains a new sequence.
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set new_sequence flag
 */
MIX_RESULT mix_videodecodeparams_set_new_sequence(MixVideoDecodeParams * obj,
        bool new_sequence);


/**
 * mix_videodecodeparams_get_new_sequence:
 * @obj: #MixVideoDecodeParams object
 * @new_sequence: new_sequence flag to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get new_sequence flag
 */
MIX_RESULT mix_videodecodeparams_get_new_sequence(MixVideoDecodeParams * obj,
        bool *new_sequence);


#endif /* __MIX_VIDEODECODEPARAMS_H__ */
