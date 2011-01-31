/* 
INTEL CONFIDENTIAL
Copyright 2009 Intel Corporation All Rights Reserved. 
The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
*/

/**
 * SECTION:mixvideoconfigparamsenc_h263
 * @short_description: MI-X Video H.263 Eecode Configuration Parameter
 *
 * MI-X video H.263 eecode configuration parameter objects.
 */


#include "mixvideolog.h"
#include "mixvideoconfigparamsenc_h263.h"

#define MDEBUG


MixVideoConfigParamsEncH263 *
mix_videoconfigparamsenc_h263_new (void)
{
	return new MixVideoConfigParamsEncH263();
}



MixVideoConfigParamsEncH263
  * mix_videoconfigparamsenc_h263_ref (MixVideoConfigParamsEncH263 * mix)
{
  return (MixVideoConfigParamsEncH263 *) mix_params_ref (MIX_PARAMS (mix));
}

/**
* mix_videoconfigparamsenc_h263_dup:
* @obj: a #MixVideoConfigParams object
* @returns: a newly allocated duplicate of the object.
* 
* Copy duplicate of the object.
*/
MixParams *
mix_videoconfigparamsenc_h263_dup (const MixParams * obj)
{
 
  return NULL;
}

/**
* mix_videoconfigparamsenc_h263_copy:
* @target: copy to target
* @src: copy from src
* @returns: boolean indicates if copy is successful.
* 
* Copy instance data from @src to @target.
*/
gboolean
mix_videoconfigparamsenc_h263_copy (MixParams * target, const MixParams * src)
{

  return FALSE;
}

/**
* mix_videoconfigparamsenc_h263:
* @first: first object to compare
* @second: seond object to compare
* @returns: boolean indicates if instance are equal.
* 
* Copy instance data from @src to @target.
*/
gboolean
mix_videoconfigparamsencenc_h263_equal (MixParams * first, MixParams * second)
{
 
  return FALSE;
}



MIX_RESULT mix_videoconfigparamsenc_h263_set_slice_num (MixVideoConfigParamsEncH263 * obj,
		guint slice_num) {

	return MIX_RESULT_SUCCESS;		
}

MIX_RESULT mix_videoconfigparamsenc_h263_get_slice_num (MixVideoConfigParamsEncH263 * obj,
		guint * slice_num) {

	return MIX_RESULT_SUCCESS;		
}

MIX_RESULT mix_videoconfigparamsenc_h263_set_dlk (MixVideoConfigParamsEncH263 * obj,
		guint disable_deblocking_filter_idc) {

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_h263_get_dlk (MixVideoConfigParamsEncH263 * obj,
		guint * disable_deblocking_filter_idc) {

	return MIX_RESULT_SUCCESS;
}

