/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

#ifndef __MIX_VIDEOENCODEPARAMS_H__
#define __MIX_VIDEOENCODEPARAMS_H__

#include <mixparams.h>
#include "mixvideodef.h"


/**
 * MIX_VIDEOENCODEPARAMS:
 * @obj: object to be type-casted.
 */
#define MIX_VIDEOENCODEPARAMS(obj) (reinterpret_cast<MixVideoEncodeParams*>(obj))

/**
 * MIX_IS_VIDEOENCODEPARAMS:
 * @obj: an object.
 *
 * Checks if the given object is an instance of #MixParams
 */
#define MIX_IS_VIDEOENCODEPARAMS(obj) ((NULL !=MIX_VIDEOENCODEPARAMS(obj)) ? TRUE : FALSE)


/**
 * MixVideoEncodeParams:
 *
 * MI-X VideoDecode Parameter object
 */
class MixVideoEncodeParams :public MixParams {
public:
    MixVideoEncodeParams();
    virtual ~MixVideoEncodeParams();
    virtual bool copy(MixParams * target) const;
    virtual MixParams *dup() const;
    virtual bool equal(MixParams *  obj) const;

public:
    /* TODO: Add properties */
    /* < private > */
    uint64 timestamp;
    bool discontinuity;

    /* < public > */

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
 * mix_videoencodeparams_new:
 * @returns: A newly allocated instance of #MixVideoEncodeParams
 *
 * Use this method to create new instance of #MixVideoEncodeParams
 */
MixVideoEncodeParams *mix_videoencodeparams_new(void);
/**
 * mix_videoencodeparams_ref:
 * @mix: object to add reference
 * @returns: the MixVideoEncodeParams instance where reference count has been increased.
 *
 * Add reference count.
 */
MixVideoEncodeParams *mix_videoencodeparams_ref(MixVideoEncodeParams * mix);

/**
 * mix_videoencodeparams_unref:
 * @obj: object to unref.
 *
 * Decrement reference count of the object.
 */
#define mix_videoencodeparams_unref(obj) mix_params_unref(MIX_PARAMS(obj))

/* Class Methods */

/* TODO: Add getters and setters for properties */
MIX_RESULT mix_videoencodeparams_set_timestamp(
    MixVideoEncodeParams * obj, uint64 timestamp);
MIX_RESULT mix_videoencodeparams_get_timestamp(
    MixVideoEncodeParams * obj, uint64 * timestamp);
MIX_RESULT mix_videoencodeparams_set_discontinuity(
    MixVideoEncodeParams * obj, bool discontinuity);
MIX_RESULT mix_videoencodeparams_get_discontinuity(
    MixVideoEncodeParams * obj, bool *discontinuity);

#endif /* __MIX_VIDEOENCODEPARAMS_H__ */

