/* 
INTEL CONFIDENTIAL
Copyright 2009 Intel Corporation All Rights Reserved. 
The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
*/

#ifndef __MIX_VIDEOCONFIGPARAMSDEC_H264_H__
#define __MIX_VIDEOCONFIGPARAMSDEC_H264_H__

#include "mixvideoconfigparamsdec.h"
#include "mixvideodef.h"

/**
* MIX_VIDEOCONFIGPARAMSDEC_H264:
* @obj: object to be type-casted.
*/
#define MIX_VIDEOCONFIGPARAMSDEC_H264(obj) (reinterpret_cast<MixVideoConfigParamsDecH264*>(obj))

/**
* MIX_IS_VIDEOCONFIGPARAMSDEC_H264:
* @obj: an object.
* 
* Checks if the given object is an instance of #MixVideoConfigParamsDecH264
*/
#define MIX_IS_VIDEOCONFIGPARAMSDEC_H264(obj) ((NULL != MIX_VIDEOCONFIGPARAMSDEC_H264(obj)) ? TRUE : FALSE)


/**
* MixVideoConfigParamsDecH264:
*
* MI-X VideoConfig Parameter object
*/
class MixVideoConfigParamsDecH264 : public MixVideoConfigParamsDec
{
public:
    MixVideoConfigParamsDecH264();
    ~MixVideoConfigParamsDecH264();
    virtual gboolean copy(MixParams *target) const;
    virtual gboolean equal(MixParams* obj) const;
    virtual MixParams* dup() const;
public:
  /*< public > */

  /* TODO: Add H.264 configuration paramters */
  
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
* mix_videoconfigparamsdec_h264_get_type:
* @returns: type
* 
* Get the type of object.
*/
//GType mix_videoconfigparamsdec_h264_get_type (void);

/**
* mix_videoconfigparamsdec_h264_new:
* @returns: A newly allocated instance of #MixVideoConfigParamsDecH264
* 
* Use this method to create new instance of #MixVideoConfigParamsDecH264
*/
MixVideoConfigParamsDecH264 *mix_videoconfigparamsdec_h264_new (void);
/**
* mix_videoconfigparamsdec_h264_ref:
* @mix: object to add reference
* @returns: #the MixVideoConfigParamsDecH264 instance where reference count has been increased.
* 
* Add reference count.
*/
MixVideoConfigParamsDecH264
  * mix_videoconfigparamsdec_h264_ref (MixVideoConfigParamsDecH264 * mix);

/**
* mix_videoconfigparamsdec_h264_unref:
* @obj: object to unref.
* 
* Decrement reference count of the object.
*/
#define mix_videoconfigparamsdec_h264_unref(obj) mix_params_unref(MIX_PARAMS(obj))

/* Class Methods */

/* TODO: Add getters and setters for other properties */
#endif /* __MIX_VIDEOCONFIGPARAMSDEC_H264_H__ */
