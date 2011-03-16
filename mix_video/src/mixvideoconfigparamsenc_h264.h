/*
INTEL CONFIDENTIAL
Copyright 2009 Intel Corporation All Rights Reserved.
The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
*/

#ifndef __MIX_VIDEOCONFIGPARAMSENC_H264_H__
#define __MIX_VIDEOCONFIGPARAMSENC_H264_H__

#include "mixvideoconfigparamsenc.h"
#include "mixvideodef.h"


/**
* MIX_TYPE_VIDEOCONFIGPARAMSENC_H264:
*
* Get type of class.
*/
#define MIX_VIDEOCONFIGPARAMSENC_H264(obj) (reinterpret_cast<MixVideoConfigParamsEncH264*>(obj))


/**
* MIX_IS_VIDEOCONFIGPARAMSENC_H264:
* @obj: an object.
*
* Checks if the given object is an instance of #MixVideoConfigParamsEncH264
*/
#define MIX_IS_VIDEOCONFIGPARAMSENC_H264(obj) ((NULL != MIX_VIDEOCONFIGPARAMSENC_H264(obj)) ? TRUE : FALSE)


/**
* MixVideoConfigParamsEncH264:
*
* MI-X VideoConfig Parameter object
*/
class MixVideoConfigParamsEncH264 : public MixVideoConfigParamsEnc {
public:
    MixVideoConfigParamsEncH264();
    virtual ~MixVideoConfigParamsEncH264();
    virtual bool copy(MixParams *target) const;
    virtual bool equal(MixParams* obj) const;
    virtual MixParams* dup() const;

public:

    /* TODO: Add H.264 configuration paramters */

    /* The basic unit size used by rate control */
    uint basic_unit_size;

    /* Number of slices in one frame */
    uint slice_num;

    /* Number of slices in one I frame */
    uint I_slice_num;

    /* Number of slices in one P frame */
    uint P_slice_num;

    /* enable/disable deblocking */
    uint8 disable_deblocking_filter_idc;

    /* enable/disable vui */
    uint8 vui_flag;

    /* delimiter_type */
    MixDelimiterType delimiter_type;

    uint idr_interval;

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
* mix_videoconfigparamsenc_h264_new:
* @returns: A newly allocated instance of #MixVideoConfigParamsEncH264
*
* Use this method to create new instance of #MixVideoConfigParamsEncH264
*/
MixVideoConfigParamsEncH264 *mix_videoconfigparamsenc_h264_new (void);
/**
* mix_videoconfigparamsenc_h264_ref:
* @mix: object to add reference
* @returns: the #MixVideoConfigParamsEncH264 instance where reference count has been increased.
*
* Add reference count.
*/
MixVideoConfigParamsEncH264*
mix_videoconfigparamsenc_h264_ref (MixVideoConfigParamsEncH264 * mix);

/**
* mix_videoconfigparamsenc_h264_unref:
* @obj: object to unref.
*
* Decrement reference count of the object.
*/
#define mix_videoconfigparamsenc_h264_unref(obj) mix_params_unref(MIX_PARAMS(obj))

/* Class Methods */

/* TODO: Add getters and setters for other properties */


/**
 * mix_videoconfigparamsenc_h264_set_bus:
 * @obj: #MixVideoConfigParamsEncH264 object
 * @basic_unit_size: The basic unit size used by rate control
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set The basic unit size used by rate control
 */
MIX_RESULT mix_videoconfigparamsenc_h264_set_bus (
    MixVideoConfigParamsEncH264 * obj, uint basic_unit_size);

/**
 * mix_videoconfigparamsenc_h264_get_bus:
 * @obj: #MixVideoConfigParamsEncH264 object
 * @basic_unit_size: The basic unit size to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get The basic unit size used by rate control
 */
MIX_RESULT mix_videoconfigparamsenc_h264_get_bus (
    MixVideoConfigParamsEncH264 * obj, uint * basic_unit_size);

/**
 * mix_videoconfigparamsenc_h264_set_dlk:
 * @obj: #MixVideoConfigParamsEncH264 object
 * @disable_deblocking_filter_idc: The flag to enable/disable deblocking
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set the The flag to enable/disable deblocking
 */
MIX_RESULT mix_videoconfigparamsenc_h264_set_dlk (
    MixVideoConfigParamsEncH264 * obj, uint disable_deblocking_filter_idc);

/**
 * mix_videoconfigparamsenc_h264_get_dlk:
 * @obj: #MixVideoConfigParamsEncH264 object
 * @disable_deblocking_filter_idc: deblocking flag to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get the The flag to enable/disable deblocking
 */
MIX_RESULT mix_videoconfigparamsenc_h264_get_dlk (
    MixVideoConfigParamsEncH264 * obj, uint * disable_deblocking_filter_idc);

/**
 * mix_videoconfigparamsenc_h264_set_vui_flag:
 * @obj: #MixVideoConfigParamsEncH264 object
 * @vui_flag: The flag to enable/disable vui
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set the The flag to enable/disable vui
 */
MIX_RESULT mix_videoconfigparamsenc_h264_set_vui_flag (
    MixVideoConfigParamsEncH264 * obj, uint8 vui_flag);

/**
 * mix_videoconfigparamsenc_h264_get_vui_flag
 * @obj: #MixVideoConfigParamsEncH264 object
 * @vui_flag: vui_flag to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get the The flag to enable/disable vui_flag
 */
MIX_RESULT mix_videoconfigparamsenc_h264_get_vui_flag (
    MixVideoConfigParamsEncH264 * obj, uint8 * vui_flag);


/**
 * mix_videoconfigparamsenc_h264_set_slice_num:
 * @obj: #MixVideoConfigParamsEncH264 object
 * @slice_num: Number of slices in one frame
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set the Number of slices in one frame
 */
MIX_RESULT mix_videoconfigparamsenc_h264_set_slice_num(
    MixVideoConfigParamsEncH264 * obj, uint slice_num);

/**
 * mix_videoconfigparamsenc_h264_get_slice_num:
 * @obj: #MixVideoConfigParamsEncH264 object
 * @slice_num: Number of slices in one frame to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get the Number of slices in one frame
 */
MIX_RESULT mix_videoconfigparamsenc_h264_get_slice_num(
    MixVideoConfigParamsEncH264 * obj, uint * slice_num);


/**
 * mix_videoconfigparamsenc_h264_set_I_slice_num:
 * @obj: #MixVideoConfigParamsEncH264 object
 * @I_slice_num: Number of slices in one I frame
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set the Number of slices in one I frame
 */
MIX_RESULT mix_videoconfigparamsenc_h264_set_I_slice_num(
    MixVideoConfigParamsEncH264 * obj, uint I_slice_num);

/**
 * mix_videoconfigparamsenc_h264_get_I_slice_num:
 * @obj: #MixVideoConfigParamsEncH264 object
 * @I_slice_num: Number of slices in one I frame to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get the Number of slices in one I frame
 */
MIX_RESULT mix_videoconfigparamsenc_h264_get_I_slice_num(
    MixVideoConfigParamsEncH264 * obj, uint * I_slice_num);

/**
 * mix_videoconfigparamsenc_h264_set_P_slice_num:
 * @obj: #MixVideoConfigParamsEncH264 object
 * @P_slice_num: Number of slices in one P frame
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set the Number of slices in one P frame
 */
MIX_RESULT mix_videoconfigparamsenc_h264_set_P_slice_num(
    MixVideoConfigParamsEncH264 * obj, uint P_slice_num);

/**
 * mix_videoconfigparamsenc_h264_get_P_slice_num:
 * @obj: #MixVideoConfigParamsEncH264 object
 * @P_slice_num: Number of slices in one P frame to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get the Number of slices in one P frame
 */
MIX_RESULT mix_videoconfigparamsenc_h264_get_P_slice_num(
    MixVideoConfigParamsEncH264 * obj, uint * P_slice_num);

/**
 * mix_videoconfigparamsenc_h264_set_delimiter_type:
 * @obj: #MixVideoConfigParamsEncH264 object
 * @delimiter_type: Delimiter type
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set Delimiter type
 */
MIX_RESULT mix_videoconfigparamsenc_h264_set_delimiter_type (
    MixVideoConfigParamsEncH264 * obj, MixDelimiterType delimiter_type);

/**
 * mix_videoconfigparamsenc_h264_get_delimiter_type:
 * @obj: #MixVideoConfigParamsEncH264 object
 * @delimiter_type: Delimiter type to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get Delimiter type
 */
MIX_RESULT mix_videoconfigparamsenc_h264_get_delimiter_type (
    MixVideoConfigParamsEncH264 * obj, MixDelimiterType * delimiter_type);


/**
 * mix_videoconfigparamsenc_h264_set_IDR_interval:
 * @obj: #MixVideoConfigParamsEncH264 object
 * @idr_interval: IDR interval
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set IDR interval
 */
MIX_RESULT mix_videoconfigparamsenc_h264_set_IDR_interval (
    MixVideoConfigParamsEncH264 * obj, uint idr_interval);


/**
 * mix_videoconfigparamsenc_h264_get_IDR_interval:
 * @obj: #MixVideoConfigParamsEncH264 object
 * @idr_interval: IDR interval to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get IDR interval
 */
MIX_RESULT mix_videoconfigparamsenc_h264_get_IDR_interval (
    MixVideoConfigParamsEncH264 * obj, uint * idr_interval);



#endif /* __MIX_VIDEOCONFIGPARAMSENC_H264_H__ */

