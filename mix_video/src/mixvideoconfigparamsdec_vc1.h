/* 
INTEL CONFIDENTIAL
Copyright 2009 Intel Corporation All Rights Reserved. 
The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
*/

#ifndef __MIX_VIDEOCONFIGPARAMSDEC_VC1_H__
#define __MIX_VIDEOCONFIGPARAMSDEC_VC1_H__

#include "mixvideoconfigparamsdec.h"
#include "mixvideodef.h"

/**
* MIX_VIDEOCONFIGPARAMSDEC_VC1:
* @obj: object to be type-casted.
*/
#define MIX_VIDEOCONFIGPARAMSDEC_VC1(obj) (reinterpret_cast<MixVideoConfigParamsDecVC1*>(obj))

/**
* MIX_IS_VIDEOCONFIGPARAMSDEC_VC1:
* @obj: an object.
* 
* Checks if the given object is an instance of #MixVideoConfigParamsDecVC1
*/
#define MIX_IS_VIDEOCONFIGPARAMSDEC_VC1(obj) ((NULL != MIX_VIDEOCONFIGPARAMSDEC_VC1(obj)) ? TRUE : FALSE)

/**
* MixVideoConfigParamsDecVC1:
*
* MI-X VideoConfig Parameter object
*/
class MixVideoConfigParamsDecVC1 : public MixVideoConfigParamsDec
{
public:
    MixVideoConfigParamsDecVC1();
    ~MixVideoConfigParamsDecVC1();
    virtual gboolean copy(MixParams *target) const;
    virtual gboolean equal(MixParams* obj) const;
    virtual MixParams* dup() const;
public:
  /*< public > */

  /* TODO: Add VC1 configuration paramters */
  /* TODO: wmv_version and fourcc type might be changed later */
  
  /* WMV version */
  guint wmv_version;
  
  /* FourCC code */
  guint fourcc;

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
* mix_videoconfigparamsdec_vc1_new:
* @returns: A newly allocated instance of #MixVideoConfigParamsDecVC1
* 
* Use this method to create new instance of #MixVideoConfigParamsDecVC1
*/
MixVideoConfigParamsDecVC1 *mix_videoconfigparamsdec_vc1_new (void);
/**
* mix_videoconfigparamsdec_vc1_ref:
* @mix: object to add reference
* @returns: the #MixVideoConfigParamsDecVC1 instance where reference count has been increased.
* 
* Add reference count.
*/
MixVideoConfigParamsDecVC1
  * mix_videoconfigparamsdec_vc1_ref (MixVideoConfigParamsDecVC1 * mix);

/**
* mix_videoconfigparamsdec_vc1_unref:
* @obj: object to unref.
* 
* Decrement reference count of the object.
*/
#define mix_videoconfigparamsdec_vc1_unref(obj) mix_params_unref(MIX_PARAMS(obj))

/* Class Methods */

/* TODO: Add getters and setters for other properties */
#endif /* __MIX_VIDEOCONFIGPARAMSDECDEC_VC1_H__ */
