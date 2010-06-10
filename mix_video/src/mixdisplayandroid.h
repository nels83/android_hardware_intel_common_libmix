/* 
INTEL CONFIDENTIAL
Copyright 2009 Intel Corporation All Rights Reserved. 
The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
*/

#ifndef __MIX_DISPLAYANDROID_H__
#define __MIX_DISPLAYANDROID_H__

#include "mixdisplay.h"
#include "mixvideodef.h"

//#ifdef ANDROID
//#include <ui/ISurface.h>
//using namespace android;
//#endif

//#ifdef __cplusplus
//extern "C" {
//#endif

#ifdef ANDROID

/**
* MIX_TYPE_DISPLAYANDROID:
* 
* Get type of class.
*/
#define MIX_TYPE_DISPLAYANDROID (mix_displayandroid_get_type ())

/**
* MIX_DISPLAYANDROID:
* @obj: object to be type-casted.
*/
#define MIX_DISPLAYANDROID(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), MIX_TYPE_DISPLAYANDROID, MixDisplayAndroid))

/**
* MIX_IS_DISPLAYANDROID:
* @obj: an object.
* 
* Checks if the given object is an instance of #MixDisplay
*/
#define MIX_IS_DISPLAYANDROID(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MIX_TYPE_DISPLAYANDROID))

/**
* MIX_DISPLAYANDROID_CLASS:
* @klass: class to be type-casted.
*/
#define MIX_DISPLAYANDROID_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), MIX_TYPE_DISPLAYANDROID, MixDisplayAndroidClass))

/**
* MIX_IS_DISPLAYANDROID_CLASS:
* @klass: a class.
* 
* Checks if the given class is #MixDisplayClass
*/
#define MIX_IS_DISPLAYANDROID_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MIX_TYPE_DISPLAYANDROID))

/**
* MIX_DISPLAYANDROID_GET_CLASS:
* @obj: a #MixDisplay object.
* 
* Get the class instance of the object.
*/
#define MIX_DISPLAYANDROID_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), MIX_TYPE_DISPLAYANDROID, MixDisplayAndroidClass))

typedef struct _MixDisplayAndroid MixDisplayAndroid;
typedef struct _MixDisplayAndroidClass MixDisplayAndroidClass;

/**
* MixDisplayAndroid:
*
* MI-X VideoInit Parameter object
*/
struct _MixDisplayAndroid
{
  /*< public > */
  MixDisplay parent;

  /*< public > */

  /* Pointer to a Android specific display  */
  void *display;
  
  /* An Android drawable that is a smart pointer  
   * of ISurface. This field is not used in 
   * mix_video_initialize().
   */ 
  // sp<ISurface> drawable;
};

/**
* MixDisplayAndroidClass:
* 
* MI-X VideoInit object class
*/
struct _MixDisplayAndroidClass
{
  /*< public > */
  MixDisplayClass parent_class;

  /* class members */
};

/**
* mix_displayandroid_get_type:
* @returns: type
* 
* Get the type of object.
*/
GType mix_displayandroid_get_type (void);

/**
* mix_displayandroid_new:
* @returns: A newly allocated instance of #MixDisplayAndroid
* 
* Use this method to create new instance of #MixDisplayAndroid
*/
MixDisplayAndroid *mix_displayandroid_new (void);
/**
* mix_displayandroid_ref:
* @mix: object to add reference
* @returns: the #MixDisplayAndroid instance where reference count has been increased.
* 
* Add reference count.
*/
MixDisplayAndroid *mix_displayandroid_ref (MixDisplayAndroid * mix);

/**
* mix_displayandroid_unref:
* @obj: object to unref.
* 
* Decrement reference count of the object.
*/
#define mix_displayandroid_unref(obj) mix_display_unref(MIX_DISPLAY(obj))

/* Class Methods */


/**
 * mix_displayandroid_set_display:
 * @obj: #MixDisplayAndroid object
 * @display: Pointer to Android specific display
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set Display 
 */
MIX_RESULT mix_displayandroid_set_display (MixDisplayAndroid * obj,
				       void * display);

/**
 * mix_displayandroid_get_display:
 * @obj: #MixDisplayAndroid object
 * @display: Pointer to pointer of Android specific display    
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get Display 
 */
MIX_RESULT mix_displayandroid_get_display (MixDisplayAndroid * obj,
				       void ** dislay);


#endif /* ANDROID */

//#ifdef __cplusplus
//}
//#endif

#endif /* __MIX_DISPLAYANDROID_H__ */

