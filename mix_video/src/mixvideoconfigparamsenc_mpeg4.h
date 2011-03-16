/*
INTEL CONFIDENTIAL
Copyright 2009 Intel Corporation All Rights Reserved.
The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
*/

#ifndef __MIX_VIDEOCONFIGPARAMSENC_MPEG4_H__
#define __MIX_VIDEOCONFIGPARAMSENC_MPEG4_H__

#include "mixvideoconfigparamsenc.h"
#include "mixvideodef.h"


/**
* MIX_VIDEOCONFIGPARAMSENC_MPEG4:
* @obj: object to be type-casted.
*/
#define MIX_VIDEOCONFIGPARAMSENC_MPEG4(obj) (reinterpret_cast<MixVideoConfigParamsEncMPEG4*>(obj))

/**
* MIX_IS_VIDEOCONFIGPARAMSENC_MPEG4:
* @obj: an object.
*
* Checks if the given object is an instance of #MixVideoConfigParamsEncMPEG4
*/
#define MIX_IS_VIDEOCONFIGPARAMSENC_MPEG4(obj) (NULL != MIX_VIDEOCONFIGPARAMSENC_MPEG4(obj))


/**
* MixVideoConfigParamsEncMPEG4:
*
* MI-X VideoConfig Parameter object
*/
class MixVideoConfigParamsEncMPEG4 : public MixVideoConfigParamsEnc {
public:
    MixVideoConfigParamsEncMPEG4();
    virtual ~MixVideoConfigParamsEncMPEG4();
    virtual bool copy(MixParams *target) const;
    virtual bool equal(MixParams* obj) const;
    virtual MixParams* dup() const;

public:
    /* TODO: Add MPEG-4 configuration paramters */
    /* Indicate profile and level.
    * Default value is 3.
    * Can be ignored (refer to encoding
    * specification for more info). */
    uchar  profile_and_level_indication;

    /* Number of ticks between two successive VOPs
    * in display order. Default value is 3.
    * Can be ignored (refer to encoding specification
    * for more info) */
    uint fixed_vop_time_increment;

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
* mix_videoconfigparamsenc_mpeg4_new:
* @returns: A newly allocated instance of #MixVideoConfigParamsEncMPEG4
*
* Use this method to create new instance of #MixVideoConfigParamsEncMPEG4
*/
MixVideoConfigParamsEncMPEG4 *mix_videoconfigparamsenc_mpeg4_new (void);
/**
* mix_videoconfigparamsenc_mpeg4_ref:
* @mix: object to add reference
* @returns: the #MixVideoConfigParamsEncMPEG4 instance where reference count has been increased.
*
* Add reference count.
*/
MixVideoConfigParamsEncMPEG4* mix_videoconfigparamsenc_mpeg4_ref (MixVideoConfigParamsEncMPEG4 * mix);

/**
* mix_videoconfigparamsenc_mpeg4_unref:
* @obj: object to unref.
*
* Decrement reference count of the object.
*/
#define mix_videoconfigparamsenc_mpeg4_unref(obj) mix_params_unref(MIX_PARAMS(obj))

/* Class Methods */

/* TODO: Add getters and setters for other properties */

/**
 * mix_videoconfigparamsenc_mpeg4_set_dlk:
 * @obj: #MixVideoConfigParamsEncMPEG4 object
 * @disable_deblocking_filter_idc: The flag to enable/disable deblocking
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set the The flag to enable/disable deblocking
 */
MIX_RESULT mix_videoconfigparamsenc_mpeg4_set_dlk (
    MixVideoConfigParamsEncMPEG4 * obj, uint disable_deblocking_filter_idc);

/**
 * mix_videoconfigparamsenc_mpeg4_get_dlk:
 * @obj: #MixVideoConfigParamsEncMPEG4 object
 * @disable_deblocking_filter_idc: deblocking flag to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get the The flag to enable/disable deblocking
 */
MIX_RESULT mix_videoconfigparamsenc_mpeg4_get_dlk (
    MixVideoConfigParamsEncMPEG4 * obj, uint * disable_deblocking_filter_idc);

/**
 * mix_videoconfigparamsenc_mpeg4_set_profile_level:
 * @obj: #MixVideoConfigParamsEncMPEG4 object
 * @profile_and_level_indication: Indicate profile and level. Default value is 3.
 *                                Can be ignored (refer to encoding specification
 *                                for more info).
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set profile_and_level_indication
 */
MIX_RESULT mix_videoconfigparamsenc_mpeg4_set_profile_level (
    MixVideoConfigParamsEncMPEG4 * obj, uchar profile_and_level_indication);

/**
 * mix_videoconfigparamsenc_mpeg4_get_profile_level:
 * @obj: #MixVideoConfigParamsEncMPEG4 object
 * @profile_and_level_indication: profile_and_level_indication to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get profile_and_level_indication
 */
MIX_RESULT mix_videoconfigparamsenc_mpeg4_get_profile_level (
    MixVideoConfigParamsEncMPEG4 * obj, uchar * profile_and_level_indication);

/**
 * mix_videoconfigparamsenc_mpeg4_get_profile_level:
 * @obj: #MixVideoConfigParamsEncMPEG4 object
 * @fixed_vop_time_increment: Number of ticks between two successive VOPs in display order.
 *                            Default value is 3. Can be ignored (refer to encoding specification
 *                            for more info)
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set fixed_vop_time_increment
 */
MIX_RESULT mix_videoconfigparamsenc_mpeg4_set_fixed_vti (
    MixVideoConfigParamsEncMPEG4 * obj, uint fixed_vop_time_increment);

/**
 * mix_videoconfigparamsenc_mpeg4_get_fixed_vti:
 * @obj: #MixVideoConfigParamsEncMPEG4 object
 * @fixed_vop_time_increment: fixed_vop_time_increment to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get fixed_vop_time_increment
 */
MIX_RESULT mix_videoconfigparamsenc_mpeg4_get_fixed_vti (
    MixVideoConfigParamsEncMPEG4 * obj, uint * fixed_vop_time_increment);


#endif /* __MIX_VIDEOCONFIGPARAMSENC_MPEG4_H__ */
