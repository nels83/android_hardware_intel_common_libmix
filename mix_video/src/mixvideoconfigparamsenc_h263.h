/*
INTEL CONFIDENTIAL
Copyright 2009 Intel Corporation All Rights Reserved.
The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
*/

#ifndef __MIX_VIDEOCONFIGPARAMSENC_H263_H__
#define __MIX_VIDEOCONFIGPARAMSENC_H263_H__

#include "mixvideoconfigparamsenc.h"
#include "mixvideodef.h"


/**
* MIX_VIDEOCONFIGPARAMSENC_H263:
* @obj: object to be type-casted.
*/
#define MIX_VIDEOCONFIGPARAMSENC_H263(obj) (reinterpret_cast<MixVideoConfigParamsEncH263*>(obj))

/**
* MIX_IS_VIDEOCONFIGPARAMSENC_H263:
* @obj: an object.
*
* Checks if the given object is an instance of #MixVideoConfigParamsEncH263
*/
#define MIX_IS_VIDEOCONFIGPARAMSENC_H263(obj) ((NULL != MIX_VIDEOCONFIGPARAMSENC_H263(obj)) ? TRUE : FALSE)


/**
* MixVideoConfigParamsEncH263:
*
* MI-X VideoConfig Parameter object
*/
class MixVideoConfigParamsEncH263 : public MixVideoConfigParamsEnc {
public:
    MixVideoConfigParamsEncH263();
    virtual ~MixVideoConfigParamsEncH263();
    virtual bool copy(MixParams *target) const;
    virtual bool equal(MixParams* obj) const;
    virtual MixParams* dup() const;

public:
    /*< public > */

    /* TODO: Add H.263 configuration paramters */

    /* slice number in one picture */
    uint slice_num;

    /* enable/disable deblocking */
    uint disable_deblocking_filter_idc;

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
* mix_videoconfigparamsenc_h263_new:
* @returns: A newly allocated instance of #MixVideoConfigParamsEncH263
*
* Use this method to create new instance of #MixVideoConfigParamsEncH263
*/
MixVideoConfigParamsEncH263 *mix_videoconfigparamsenc_h263_new (void);
/**
* mix_videoconfigparamsenc_h263_ref:
* @mix: object to add reference
* @returns: the #MixVideoConfigParamsEncH263 instance where reference count has been increased.
*
* Add reference count.
*/
MixVideoConfigParamsEncH263*
mix_videoconfigparamsenc_h263_ref (MixVideoConfigParamsEncH263 * mix);

/**
* mix_videoconfigparamsenc_h263_unref:
* @obj: object to unref.
*
* Decrement reference count of the object.
*/
#define mix_videoconfigparamsenc_h263_unref(obj) mix_params_unref(MIX_PARAMS(obj))

/* Class Methods */

/* TODO: Add getters and setters for other properties */

/**
 * mix_videoconfigparamsenc_h263_set_dlk:
 * @obj: #MixVideoConfigParamsEncH263 object
 * @disable_deblocking_filter_idc: The flag to enable/disable deblocking
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set the The flag to enable/disable deblocking
 */
MIX_RESULT mix_videoconfigparamsenc_h263_set_dlk (
    MixVideoConfigParamsEncH263 * obj, uint disable_deblocking_filter_idc);

/**
 * mix_videoconfigparamsenc_h263_get_dlk:
 * @obj: #MixVideoConfigParamsEncH263 object
 * @disable_deblocking_filter_idc: deblocking flag to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get the The flag to enable/disable deblocking
 */
MIX_RESULT mix_videoconfigparamsenc_h263_get_dlk (
    MixVideoConfigParamsEncH263 * obj, uint * disable_deblocking_filter_idc);

/**
 * mix_videoconfigparamsenc_h263_set_slice_num:
 * @obj: #MixVideoConfigParamsEncH263 object
 * @slice_num: Number of slice in one picture encoded.
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set slice_num
 */
MIX_RESULT mix_videoconfigparamsenc_h263_set_slice_num (
    MixVideoConfigParamsEncH263 * obj, uint slice_num);

/**
 * mix_videoconfigparamsenc_h263_get_slice_num:
 * @obj: #MixVideoConfigParamsEncH263 object
 * @slice_num: Number of slice in one picture encoded.
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get slice_num
 */
MIX_RESULT mix_videoconfigparamsenc_h263_get_slice_num (
    MixVideoConfigParamsEncH263 * obj, uint * slice_num);



#endif /* __MIX_VIDEOCONFIGPARAMSENC_H263_H__ */
