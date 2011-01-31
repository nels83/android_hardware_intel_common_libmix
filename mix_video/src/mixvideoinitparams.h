/* 
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved. 
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
*/

#ifndef __MIX_VIDEOINITPARAMS_H__
#define __MIX_VIDEOINITPARAMS_H__

#include <mixparams.h>
#include "mixdisplay.h"
#include "mixvideodef.h"

/**
 * MIX_VIDEOINITPARAMS:
 * @obj: object to be type-casted.
 */
#define MIX_VIDEOINITPARAMS(obj) (reinterpret_cast<MixVideoInitParams*>(obj))

/**
 * MIX_IS_VIDEOINITPARAMS:
 * @obj: an object.
 * 
 * Checks if the given object is an instance of #MixParams
 */
#define MIX_IS_VIDEOINITPARAMS(obj) ((NULL != MIX_VIDEOINITPARAMS(obj)) ? TRUE : FALSE)

/**
 * MixVideoInitParams:
 *
 * MI-X VideoInit Parameter object
 */
class MixVideoInitParams : public MixParams {
public:
	MixVideoInitParams();
	~MixVideoInitParams();
	virtual gboolean copy(MixParams *target) const;
	virtual gboolean equal(MixParams* obj) const;
	virtual MixParams* dup() const;
public:
	/*< public > */

	/* Pointer to a MixDisplay object 
	* such as MixDisplayX11 */
	MixDisplay *display;

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
 * mix_videoinitparams_new:
 * @returns: A newly allocated instance of #MixVideoInitParams
 * 
 * Use this method to create new instance of #MixVideoInitParams
 */
MixVideoInitParams *mix_videoinitparams_new (void);
/**
 * mix_videoinitparams_ref:
 * @mix: object to add reference
 * @returns: the #MixVideoInitParams instance where reference count has been increased.
 * 
 * Add reference count.
 */
MixVideoInitParams *mix_videoinitparams_ref (MixVideoInitParams * mix);

/**
 * mix_videoinitparams_unref:
 * @obj: object to unref.
 * 
 * Decrement reference count of the object.
 */
#define mix_videoinitparams_unref(obj) mix_params_unref(MIX_PARAMS(obj))

/* Class Methods */


/**
 * mix_videoinitparams_set_display:
 * @obj: #MixVideoInitParams object
 * @display: Pointer to a MixDisplay object such as MixDisplayX11   
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set MixDisplay object 
 */
MIX_RESULT mix_videoinitparams_set_display (
	MixVideoInitParams * obj, MixDisplay * display);

/**
 * mix_videoinitparams_get_display:
 * @obj: #MixVideoInitParams object
 * @dislay: Pointer to pointer of a MixDisplay object such as MixDisplayX11   
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get MixDisplay object 
 */
MIX_RESULT mix_videoinitparams_get_display (
	MixVideoInitParams * obj, MixDisplay ** dislay);



#endif /* __MIX_VIDEOINITPARAMS_H__ */
