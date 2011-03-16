/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

#ifndef __MIX_VIDEOCONFIGPARAMS_H__
#define __MIX_VIDEOCONFIGPARAMS_H__

#include <mixparams.h>
#include "mixvideodef.h"

/**
 * MIX_VIDEOCONFIGPARAMS:
 * @obj: object to be type-casted.
 */
#define MIX_VIDEOCONFIGPARAMS(obj) (reinterpret_cast<MixVideoConfigParams*>(obj))


/**
 * MixVideoConfigParams:
 *
 * MI-X VideoConfig Parameter object
 */
class MixVideoConfigParams : public MixParams {
public:
    MixVideoConfigParams();
    virtual ~MixVideoConfigParams();
    virtual bool copy(MixParams *target) const;
    virtual bool equal(MixParams* obj) const;
    virtual MixParams* dup() const;
    /*< public > */
    //MixParams parent;

    /*< private > */
protected:
    void *reserved1;
    void *reserved2;
    void *reserved3;
    void *reserved4;
};


/**
 * mix_videoconfigparams_new:
 * @returns: A newly allocated instance of #MixVideoConfigParams
 *
 * Use this method to create new instance of #MixVideoConfigParams
 */
MixVideoConfigParams *mix_videoconfigparams_new(void);

/**
 * mix_videoconfigparams_ref:
 * @mix: object to add reference
 * @returns: the #MixVideoConfigParams instance where reference count has been increased.
 *
 * Add reference count.
 */
MixVideoConfigParams *mix_videoconfigparams_ref(MixVideoConfigParams * mix);

/**
 * mix_videoconfigparams_unref:
 * @obj: object to unref.
 *
 * Decrement reference count of the object.
 */
#define mix_videoconfigparams_unref(obj) mix_params_unref(MIX_PARAMS(obj))

/* Class Methods */

/* TODO: Add getters and setters for other properties */


#endif /* __MIX_VIDEOCONFIGPARAMS_H__ */
