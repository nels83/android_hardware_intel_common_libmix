/* 
INTEL CONFIDENTIAL
Copyright 2009 Intel Corporation All Rights Reserved. 
The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
*/

#ifndef __MIX_VIDEOCAPS_H__
#define __MIX_VIDEOCAPS_H__

#include <mixparams.h>
#include "mixvideodef.h"

/**
* MIX_VIDEOCAPS:
* @obj: object to be type-casted.
*/
#define MIX_VIDEOCAPS(obj) (reinterpret_cast<MixVideoCaps*>(obj))

/**
* MIX_IS_VIDEOCAPS:
* @obj: an object.
* 
* Checks if the given object is an instance of #MixParams
*/
#define MIX_IS_VIDEOCAPS(obj) ((NULL != MIX_VIDEOCAPS(obj)) ? TRUE : FALSE)


/**
* MixVideoCaps:
*
* MI-X VideoConfig Parameter object
*/
class MixVideoCaps : public MixParams
{
public:
  MixVideoCaps();
  virtual ~MixVideoCaps();

  virtual gboolean copy(MixParams* target) const;
  virtual MixParams *dup() const;
  virtual gboolean equal(MixParams* obj) const;

public:
  /*< public > */
  //MixParams parent;

  /*< public > */
  gchar *mix_caps;
  gchar *video_hw_caps;

  void *reserved1;
  void *reserved2;
  void *reserved3;
  void *reserved4;
};

/**
* mix_videocaps_new:
* @returns: A newly allocated instance of #MixVideoCaps
* 
* Use this method to create new instance of #MixVideoCaps
*/
MixVideoCaps *mix_videocaps_new (void);
/**
* mix_videocaps_ref:
* @mix: object to add reference
* @returns: the MixVideoCaps instance where reference count has been increased.
* 
* Add reference count.
*/
MixVideoCaps *mix_videocaps_ref (MixVideoCaps * mix);

/**
* mix_videocaps_unref:
* @obj: object to unref.
* 
* Decrement reference count of the object.
*/
#define mix_videocaps_unref(obj) mix_params_unref(MIX_PARAMS(obj))

/* Class Methods */

MIX_RESULT mix_videocaps_set_mix_caps (MixVideoCaps * obj, gchar * mix_caps);
MIX_RESULT mix_videocaps_get_mix_caps (MixVideoCaps * obj,
				       gchar ** mix_caps);

MIX_RESULT mix_videocaps_set_video_hw_caps (MixVideoCaps * obj,
					    gchar * video_hw_caps);
MIX_RESULT mix_videocaps_get_video_hw_caps (MixVideoCaps * obj,
					    gchar ** video_hw_caps);


#endif /* __MIX_VIDEOCAPS_H__ */
