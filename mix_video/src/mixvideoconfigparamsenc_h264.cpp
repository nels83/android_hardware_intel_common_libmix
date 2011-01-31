/*
INTEL CONFIDENTIAL
Copyright 2009 Intel Corporation All Rights Reserved.
The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
*/

/**
 * SECTION:mixvideoconfigparamsenc_h264
 * @short_description: MI-X Video H.264 Eecode Configuration Parameter
 *
 * MI-X video H.264 eecode configuration parameter objects.
 */


#include "mixvideolog.h"
#include "mixvideoconfigparamsenc_h264.h"

#define MDEBUG





MixVideoConfigParamsEncH264 *
mix_videoconfigparamsenc_h264_new (void)
{
 

  return new MixVideoConfigParamsEncH264();
}


MixVideoConfigParamsEncH264
  * mix_videoconfigparamsenc_h264_ref (MixVideoConfigParamsEncH264 * mix)
{
  return (MixVideoConfigParamsEncH264 *) mix_params_ref (MIX_PARAMS (mix));
}

/**
* mix_videoconfigparamsenc_h264_dup:
* @obj: a #MixVideoConfigParams object
* @returns: a newly allocated duplicate of the object.
*
* Copy duplicate of the object.
*/
MixParams *
mix_videoconfigparamsenc_h264_dup (const MixParams * obj)
{
 
  return NULL;
}

/**
* mix_videoconfigparamsenc_h264_copy:
* @target: copy to target
* @src: copy from src
* @returns: boolean indicates if copy is successful.
*
* Copy instance data from @src to @target.
*/
gboolean
mix_videoconfigparamsenc_h264_copy (MixParams * target, const MixParams * src)
{

  return FALSE;
}

/**
* mix_videoconfigparamsenc_h264:
* @first: first object to compare
* @second: seond object to compare
* @returns: boolean indicates if instance are equal.
*
* Copy instance data from @src to @target.
*/
gboolean
mix_videoconfigparamsencenc_h264_equal (MixParams * first, MixParams * second)
{
  gboolean ret = FALSE;

  return ret;
}



MIX_RESULT mix_videoconfigparamsenc_h264_set_bus (MixVideoConfigParamsEncH264 * obj,
		guint basic_unit_size) {

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_h264_get_bus (MixVideoConfigParamsEncH264 * obj,
		guint * basic_unit_size) {

	return MIX_RESULT_SUCCESS;
}


MIX_RESULT mix_videoconfigparamsenc_h264_set_dlk (MixVideoConfigParamsEncH264 * obj,
		guint disable_deblocking_filter_idc) {

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_h264_get_dlk (MixVideoConfigParamsEncH264 * obj,
		guint * disable_deblocking_filter_idc) {

	return MIX_RESULT_SUCCESS;
}


MIX_RESULT mix_videoconfigparamsenc_h264_set_slice_num(MixVideoConfigParamsEncH264 * obj,
		guint slice_num) {

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_h264_get_slice_num(MixVideoConfigParamsEncH264 * obj,
		guint * slice_num) {

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_h264_set_I_slice_num(MixVideoConfigParamsEncH264 * obj,
		guint I_slice_num) {

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_h264_get_I_slice_num(MixVideoConfigParamsEncH264 * obj,
		guint * I_slice_num) {

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_h264_set_P_slice_num(MixVideoConfigParamsEncH264 * obj,
		guint P_slice_num) {

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_h264_get_P_slice_num(MixVideoConfigParamsEncH264 * obj,
		guint * P_slice_num) {

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_h264_set_delimiter_type (MixVideoConfigParamsEncH264 * obj,
		MixDelimiterType delimiter_type) {

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_h264_get_delimiter_type (MixVideoConfigParamsEncH264 * obj,
		MixDelimiterType * delimiter_type) {

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_h264_set_IDR_interval (MixVideoConfigParamsEncH264 * obj,
		guint idr_interval) {

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_h264_get_IDR_interval (MixVideoConfigParamsEncH264 * obj,
		guint * idr_interval) {

	return MIX_RESULT_SUCCESS;
}
