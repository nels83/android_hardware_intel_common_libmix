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

G_BEGIN_DECLS

/**
* MIX_TYPE_VIDEOCONFIGPARAMSENC_H263:
* 
* Get type of class.
*/
#define MIX_TYPE_VIDEOCONFIGPARAMSENC_H263 (mix_videoconfigparamsenc_h263_get_type ())

/**
* MIX_VIDEOCONFIGPARAMSENC_H263:
* @obj: object to be type-casted.
*/
#define MIX_VIDEOCONFIGPARAMSENC_H263(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), MIX_TYPE_VIDEOCONFIGPARAMSENC_H263, MixVideoConfigParamsEncH263))

/**
* MIX_IS_VIDEOCONFIGPARAMSENC_H263:
* @obj: an object.
* 
* Checks if the given object is an instance of #MixVideoConfigParamsEncH263
*/
#define MIX_IS_VIDEOCONFIGPARAMSENC_H263(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MIX_TYPE_VIDEOCONFIGPARAMSENC_H263))

/**
* MIX_VIDEOCONFIGPARAMSENC_H263_CLASS:
* @klass: class to be type-casted.
*/
#define MIX_VIDEOCONFIGPARAMSENC_H263_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), MIX_TYPE_VIDEOCONFIGPARAMSENC_H263, MixVideoConfigParamsEncH263Class))

/**
* MIX_IS_VIDEOCONFIGPARAMSENC_H263_CLASS:
* @klass: a class.
* 
* Checks if the given class is #MixVideoConfigParamsEncH263Class
*/
#define MIX_IS_VIDEOCONFIGPARAMSENC_H263_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MIX_TYPE_VIDEOCONFIGPARAMSENC_H263))

/**
* MIX_VIDEOCONFIGPARAMSENC_H263_GET_CLASS:
* @obj: a #MixParams object.
* 
* Get the class instance of the object.
*/
#define MIX_VIDEOCONFIGPARAMSENC_H263_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), MIX_TYPE_VIDEOCONFIGPARAMSENC_H263, MixVideoConfigParamsEncH263Class))

typedef struct _MixVideoConfigParamsEncH263 MixVideoConfigParamsEncH263;
typedef struct _MixVideoConfigParamsEncH263Class MixVideoConfigParamsEncH263Class;

/**
* MixVideoConfigParamsEncH263:
*
* MI-X VideoConfig Parameter object
*/
struct _MixVideoConfigParamsEncH263
{
  /*< public > */
  MixVideoConfigParamsEnc parent;

  /*< public > */

  /* TODO: Add H.263 configuration paramters */
  
  /* slice number in one picture */
  guint slice_num;
  
  /* enable/disable deblocking */
  guint disable_deblocking_filter_idc;
  
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
* MixVideoConfigParamsEncH263Class:
* 
* MI-X VideoConfig object class
*/
struct _MixVideoConfigParamsEncH263Class
{
  /*< public > */
  MixVideoConfigParamsEncClass parent_class;

  /* class members */
};

/**
* mix_videoconfigparamsenc_h263_get_type:
* @returns: type
* 
* Get the type of object.
*/
GType mix_videoconfigparamsenc_h263_get_type (void);

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
MixVideoConfigParamsEncH263
  * mix_videoconfigparamsenc_h263_ref (MixVideoConfigParamsEncH263 * mix);

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
MIX_RESULT mix_videoconfigparamsenc_h263_set_dlk (MixVideoConfigParamsEncH263 * obj,
		guint disable_deblocking_filter_idc);

/**
 * mix_videoconfigparamsenc_h263_get_dlk:
 * @obj: #MixVideoConfigParamsEncH263 object
 * @disable_deblocking_filter_idc: deblocking flag to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get the The flag to enable/disable deblocking
 */
MIX_RESULT mix_videoconfigparamsenc_h263_get_dlk (MixVideoConfigParamsEncH263 * obj,
		guint * disable_deblocking_filter_idc);

/**
 * mix_videoconfigparamsenc_h263_set_slice_num:
 * @obj: #MixVideoConfigParamsEncH263 object
 * @slice_num: Number of slice in one picture encoded. 
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set slice_num
 */
MIX_RESULT mix_videoconfigparamsenc_h263_set_slice_num (MixVideoConfigParamsEncH263 * obj,
		guint slice_num);

/**
 * mix_videoconfigparamsenc_h263_get_slice_num:
 * @obj: #MixVideoConfigParamsEncH263 object
 * @slice_num: Number of slice in one picture encoded.
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get slice_num
 */
MIX_RESULT mix_videoconfigparamsenc_h263_get_slice_num (MixVideoConfigParamsEncH263 * obj,
		guint * slice_num);

G_END_DECLS

#endif /* __MIX_VIDEOCONFIGPARAMSENC_H263_H__ */
