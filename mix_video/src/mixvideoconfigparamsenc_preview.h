/*
INTEL CONFIDENTIAL
Copyright 2009 Intel Corporation All Rights Reserved.
The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
*/

#ifndef __MIX_VIDEOCONFIGPARAMSENC_PREVIEW_H__
#define __MIX_VIDEOCONFIGPARAMSENC_PREVIEW_H__

#include "mixvideoconfigparamsenc.h"
#include "mixvideodef.h"


/**
* MIX_VIDEOCONFIGPARAMSENC_PREVIEW:
* @obj: object to be type-casted.
*/
#define MIX_VIDEOCONFIGPARAMSENC_PREVIEW(obj) (reinterpret_cast<MixVideoConfigParamsEncPreview*>(obj))

/**
* MIX_IS_VIDEOCONFIGPARAMSENC_PREVIEW:
* @obj: an object.
*
* Checks if the given object is an instance of #MixVideoConfigParamsEncPreview
*/
#define MIX_IS_VIDEOCONFIGPARAMSENC_PREVIEW(obj) (NULL != MIX_VIDEOCONFIGPARAMSENC_PREVIEW(obj))



/**
* MixVideoConfigParamsEncPreview:
*
* MI-X VideoConfig Parameter object
*/
class MixVideoConfigParamsEncPreview : public MixVideoConfigParamsEnc {
public:
    MixVideoConfigParamsEncPreview();
    virtual MixParams* dup() const;
};



/**
* mix_videoconfigparamsenc_preview_new:
* @returns: A newly allocated instance of #MixVideoConfigParamsEncPreview
*
* Use this method to create new instance of #MixVideoConfigParamsEncPreview
*/
MixVideoConfigParamsEncPreview* mix_videoconfigparamsenc_preview_new (void);
/**
* mix_videoconfigparamsenc_preview_ref:
* @mix: object to add reference
* @returns: the MixVideoConfigParamsEncPreview instance where reference count has been increased.
*
* Add reference count.
*/
MixVideoConfigParamsEncPreview* mix_videoconfigparamsenc_preview_ref (
    MixVideoConfigParamsEncPreview * mix);

/**
* mix_videoconfigparamsenc_preview_unref:
* @obj: object to unref.
*
* Decrement reference count of the object.
*/
#define mix_videoconfigparamsenc_preview_unref(obj) mix_params_unref(MIX_PARAMS(obj))



#endif /* __MIX_VIDEOCONFIGPARAMSENC_PREVIEW_H__ */

