/* 
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

#ifndef __MIX_VIDEOCONFIGPARAMSDEC_MP42_H__
#define __MIX_VIDEOCONFIGPARAMSDEC_MP42_H__

#include "mixvideoconfigparamsdec.h"
#include "mixvideodef.h"

/**
 * MIX_VIDEOCONFIGPARAMSDEC_MP42:
 * @obj: object to be type-casted.
 */
#define MIX_VIDEOCONFIGPARAMSDEC_MP42(obj) (reinterpret_cast<MixVideoConfigParamsDecMP42*>(obj))

/**
 * MIX_IS_VIDEOCONFIGPARAMSDEC_MP42:
 * @obj: an object.
 *
 * Checks if the given object is an instance of #MixVideoConfigParamsDecMP42
 */
#define MIX_IS_VIDEOCONFIGPARAMSDEC_MP42(obj) ((NULL != MIX_VIDEOCONFIGPARAMSDEC_MP42(obj)) ? TRUE : FALSE)


/**
 * MixVideoConfigParamsDecMP42:
 *
 * MI-X VideoConfig Parameter object
 */
class MixVideoConfigParamsDecMP42 : public MixVideoConfigParamsDec {
public:
    MixVideoConfigParamsDecMP42();
    ~MixVideoConfigParamsDecMP42();
    virtual gboolean copy(MixParams *target) const;
    virtual gboolean equal(MixParams* obj) const;
    virtual MixParams* dup() const;
public:
	/*< public > */

	/* MPEG version */
	guint mpegversion;
	
	/* DivX version */
	guint divxversion;

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
 * mix_videoconfigparamsdec_mp42_get_type:
 * @returns: type
 *
 * Get the type of object.
 */
//GType mix_videoconfigparamsdec_mp42_get_type(void);

/**
 * mix_videoconfigparamsdec_mp42_new:
 * @returns: A newly allocated instance of #MixVideoConfigParamsDecMP42
 *
 * Use this method to create new instance of #MixVideoConfigParamsDecMP42
 */
MixVideoConfigParamsDecMP42 *mix_videoconfigparamsdec_mp42_new(void);
/**
 * mix_videoconfigparamsdec_mp42_ref:
 * @mix: object to add reference
 * @returns: the #MixVideoConfigParamsDecMP42 instance where reference count has been increased.
 *
 * Add reference count.
 */
MixVideoConfigParamsDecMP42
* mix_videoconfigparamsdec_mp42_ref(MixVideoConfigParamsDecMP42 * mix);

/**
 * mix_videoconfigparamsdec_mp42_unref:
 * @obj: object to unref.
 *
 * Decrement reference count of the object.
 */
#define mix_videoconfigparamsdec_mp42_unref(obj) mix_params_unref(MIX_PARAMS(obj))

/* Class Methods */

/* TODO: Add getters and setters for other properties */


/**
 * mix_videoconfigparamsdec_mp42_set_mpegversion:
 * @obj: #MixVideoConfigParamsDecMP42 object
 * @version: MPEG version
 * @returns:  <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set MPEG version
 */
MIX_RESULT mix_videoconfigparamsdec_mp42_set_mpegversion(
		MixVideoConfigParamsDecMP42 *obj, guint version);

/**
 * mix_videoconfigparamsdec_mp42_get_mpegversion:
 * @obj: #MixVideoConfigParamsDecMP42 object
 * @version: MPEG version to be returned
 * @returns:  <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get MPEG version
 */
MIX_RESULT mix_videoconfigparamsdec_mp42_get_mpegversion(
		MixVideoConfigParamsDecMP42 *obj, guint *version);

/**
 * mix_videoconfigparamsdec_mp42_set_divxversion:
 * @obj: #MixVideoConfigParamsDecMP42 object
 * @version: DivX version
 * @returns:  <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set DivX version
 */
MIX_RESULT mix_videoconfigparamsdec_mp42_set_divxversion(
		MixVideoConfigParamsDecMP42 *obj, guint version);

/**
 * mix_videoconfigparamsdec_mp42_set_divxversion:
 * @obj: #MixVideoConfigParamsDecMP42 object
 * @version: DivX version to be returned
 * @returns:  <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get DivX version
 */
MIX_RESULT mix_videoconfigparamsdec_mp42_get_divxversion(
		MixVideoConfigParamsDecMP42 *obj, guint *version);



#endif /* __MIX_VIDEOCONFIGPARAMSDEC_MP42_H__ */
