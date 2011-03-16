/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

#ifndef __MIX_VIDEOCONFIGPARAMSENC_H__
#define __MIX_VIDEOCONFIGPARAMSENC_H__

#include <mixvideoconfigparams.h>
#include "mixvideodef.h"

/**
 * MIX_VIDEOCONFIGPARAMSENC:
 * @obj: object to be type-casted.
 */
#define MIX_VIDEOCONFIGPARAMSENC(obj) (reinterpret_cast<MixVideoConfigParamsEnc*>(obj))

/**
 * MIX_IS_VIDEOCONFIGPARAMSENC:
 * @obj: an object.
 *
 * Checks if the given object is an instance of #MixParams
 */
#define MIX_IS_VIDEOCONFIGPARAMSENC(obj) (NULL != MIX_VIDEOCONFIGPARAMSENC(obj))



/**
 * MixVideoConfigParamsEnc:
 *
 * MI-X VideoConfig Parameter object
 */
class MixVideoConfigParamsEnc : public MixVideoConfigParams {
public:
    MixVideoConfigParamsEnc();
    virtual ~MixVideoConfigParamsEnc();
    virtual bool copy(MixParams *target) const;
    virtual bool equal(MixParams* obj) const;
    virtual MixParams* dup() const;
public:
    /*< public > */
    //MixIOVec header;

    /* the type of the following members will be changed after MIX API doc is ready */

    /* Encoding profile */
    MixProfile profile;

    uint8 level;

    /* Raw format to be encoded */
    MixRawTargetFormat raw_format;

    /* Rate control mode */
    MixRateControl rate_control;

    /* Bitrate when rate control is used */
    uint bitrate;

    /* Numerator of frame rate */
    uint frame_rate_num;

    /* Denominator of frame rate */
    uint frame_rate_denom;

    /* The initial QP value */
    uint initial_qp;

    /* The minimum QP value */
    uint min_qp;

    /* this is the bit-rate the rate control is targeting, as a percentage of the maximum bit-rate
    * for example if target_percentage is 95 then the rate control will target a bit-rate that is
    * 95% of the maximum bit-rate
    */
    uint target_percentage;

    /* windows size in milliseconds. For example if this is set to 500, then the rate control will guarantee the */
    uint window_size;

    /* Number of frames between key frames (GOP size) */
    uint intra_period;

    /* Width of video frame */
    uint16 picture_width;

    /* Height of the video frame */
    uint16 picture_height;

    /* Mime type, reserved */
    char * mime_type;

    /* Encode target format */
    MixEncodeTargetFormat encode_format;

    /* Size of the pool of MixBuffer objects */
    uint mixbuffer_pool_size;

    /* Are buffers shared between capture and encoding drivers */
    bool share_buf_mode;

    /* Array of frame IDs created by capture library */
    ulong * ci_frame_id;

    /* Size of the array ci_frame_id */
    uint ci_frame_num;

    uint CIR_frame_cnt;

    /* The maximum slice size to be set to video driver (in bits).
     * The encoder hardware will try to make sure the single slice does not exceed this size
     * If not, mix_video_encode() will report a specific error
     */
    uint max_slice_size;

    MixVideoIntraRefreshType refresh_type;

    MixAIRParams air_params;

    MixBufferAllocationMode buffer_mode;
    void * buf_info;

    /* < private > */
    ulong draw;

    /*< public > */

    /* Indicates whether MixVideoFrames suitable for displaying
     * need to be enqueued for retrieval using mix_video_get_frame() */
    bool need_display;

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
 * mix_videoconfigparamsenc_new:
 * @returns: A newly allocated instance of #MixVideoConfigParamsEnc
 *
 * Use this method to create new instance of #MixVideoConfigParamsEnc
 */
MixVideoConfigParamsEnc *mix_videoconfigparamsenc_new(void);

/**
 * mix_videoconfigparamsenc_ref:
 * @mix: object to add reference
 * @returns: the #MixVideoConfigParamsEnc instance where reference count has been increased.
 *
 * Add reference count.
 */
MixVideoConfigParamsEnc *mix_videoconfigparamsenc_ref(MixVideoConfigParamsEnc * mix);

/**
 * mix_videoconfigparamsenc_unref:
 * @obj: object to unref.
 *
 * Decrement reference count of the object.
 */
#define mix_videoconfigparamsenc_unref(obj) mix_params_unref(MIX_PARAMS(obj))

/* Class Methods */

/**
 * mix_videoconfigparamsenc_set_mime_type:
 * @obj: #MixVideoConfigParamsEnc object
 * @mime_type: Mime type
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set mime type
 */
MIX_RESULT mix_videoconfigparamsenc_set_mime_type(
    MixVideoConfigParamsEnc * obj, const char * mime_type);

/**
 * mix_videoconfigparamsenc_get_mime_type:
 * @obj: #MixVideoConfigParamsEnc object
 * @mime_type: Mime type to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get mime type
 *
 * <note>
 * Caller is responsible to g_free *mime_type
 * </note>
 */
MIX_RESULT mix_videoconfigparamsenc_get_mime_type(
    MixVideoConfigParamsEnc * obj, char ** mime_type);


/**
 * mix_videoconfigparamsenc_set_frame_rate:
 * @obj: #MixVideoConfigParamsEnc object
 * @frame_rate_num: Numerator of frame rate
 * @frame_rate_denom: Denominator of frame rate
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set frame rate
 */
MIX_RESULT mix_videoconfigparamsenc_set_frame_rate(
    MixVideoConfigParamsEnc * obj, uint frame_rate_num, uint frame_rate_denom);

/**
 * mix_videoconfigparamsenc_get_frame_rate:
 * @obj: #MixVideoConfigParamsEnc object
 * @frame_rate_num: Numerator of frame rate to be returned
 * @frame_rate_denom: Denominator of frame rate to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get frame rate
 */
MIX_RESULT mix_videoconfigparamsenc_get_frame_rate(
    MixVideoConfigParamsEnc * obj, uint * frame_rate_num, uint * frame_rate_denom);

/**
 * mix_videoconfigparamsenc_set_picture_res:
 * @obj: #MixVideoConfigParamsEnc object
 * @picture_width: Width of video frame
 * @picture_height: Height of the video frame
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set width and height of video frame
 */
MIX_RESULT mix_videoconfigparamsenc_set_picture_res(
    MixVideoConfigParamsEnc * obj, uint picture_width, uint picture_height);

/**
 * mix_videoconfigparamsenc_get_picture_res:
 * @obj: #MixVideoConfigParamsEnc object
 * @picture_width: Width of video frame to be returned
 * @picture_height: Height of the video frame to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get width and height of video frame
 */
MIX_RESULT mix_videoconfigparamsenc_get_picture_res(
    MixVideoConfigParamsEnc * obj, uint * picture_width, uint * picture_height);

/**
 * mix_videoconfigparamsenc_set_encode_format:
 * @obj: #MixVideoConfigParamsEnc object
 * @encode_format: Encode target format
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set Encode target format
 */
MIX_RESULT mix_videoconfigparamsenc_set_encode_format (
    MixVideoConfigParamsEnc * obj, MixEncodeTargetFormat encode_format);

/**
 * mix_videoconfigparamsenc_get_encode_format:
 * @obj: #MixVideoConfigParamsEnc object
 * @encode_format: Encode target format to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get Encode target format
 */
MIX_RESULT mix_videoconfigparamsenc_get_encode_format (
    MixVideoConfigParamsEnc * obj, MixEncodeTargetFormat * encode_format);

/**
 * mix_videoconfigparamsenc_set_bit_rate:
 * @obj: #MixVideoConfigParamsEnc object
 * @bps: bitrate
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set bitrate
 */
MIX_RESULT mix_videoconfigparamsenc_set_bit_rate (
    MixVideoConfigParamsEnc * obj, uint bps);

/**
 * mix_videoconfigparamsenc_get_bit_rate:
 * @obj: #MixVideoConfigParamsEnc object
 * @bps: bitrate to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get bitrate
 */
MIX_RESULT mix_videoconfigparamsenc_get_bit_rate (
    MixVideoConfigParamsEnc * obj, uint *bps);

/**
 * mix_videoconfigparamsenc_set_init_qp:
 * @obj: #MixVideoConfigParamsEnc object
 * @initial_qp: The initial QP value
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set The initial QP value
 */
MIX_RESULT mix_videoconfigparamsenc_set_init_qp (
    MixVideoConfigParamsEnc * obj, uint initial_qp);

/**
 * mix_videoconfigparamsenc_get_init_qp:
 * @obj: #MixVideoConfigParamsEnc object
 * @initial_qp: The initial QP value to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get The initial QP value
 */
MIX_RESULT mix_videoconfigparamsenc_get_init_qp (
    MixVideoConfigParamsEnc * obj, uint *initial_qp);

/**
 * mix_videoconfigparamsenc_set_min_qp:
 * @obj: #MixVideoConfigParamsEnc object
 * @min_qp: The minimum QP value
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set The minimum QP value
 */
MIX_RESULT mix_videoconfigparamsenc_set_min_qp (
    MixVideoConfigParamsEnc * obj, uint min_qp);

/**
 * mix_videoconfigparamsenc_get_min_qp:
 * @obj: #MixVideoConfigParamsEnc object
 * @min_qp: The minimum QP value to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get The minimum QP value
 */
MIX_RESULT mix_videoconfigparamsenc_get_min_qp(
    MixVideoConfigParamsEnc * obj, uint *min_qp);


/**
 * mix_videoconfigparamsenc_set_target_percentage:
 * @obj: #MixVideoConfigParamsEnc object
 * @target_percentage: The target percentage value
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set The  target percentage value
 */
MIX_RESULT mix_videoconfigparamsenc_set_target_percentage (
    MixVideoConfigParamsEnc * obj, uint target_percentage);

/**
 * mix_videoconfigparamsenc_get_target_percentage:
 * @obj: #MixVideoConfigParamsEnc object
 * @target_percentage: The target percentage value to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get The target percentage value
 */
MIX_RESULT mix_videoconfigparamsenc_get_target_percentage(
    MixVideoConfigParamsEnc * obj, uint *target_percentage);

/**
 * mix_videoconfigparamsenc_set_window_size:
 * @obj: #MixVideoConfigParamsEnc object
 * @window_size: The window size for rate control
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set The window size value
 */
MIX_RESULT mix_videoconfigparamsenc_set_window_size (
    MixVideoConfigParamsEnc * obj, uint window_size);

/**
 * mix_videoconfigparamsenc_get_window_size:
 * @obj: #MixVideoConfigParamsEnc object
 * @window_size: The window size for rate control
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get The window size value
 */
MIX_RESULT mix_videoconfigparamsenc_get_window_size (
    MixVideoConfigParamsEnc * obj, uint *window_size);

/**
 * mix_videoconfigparamsenc_set_intra_period:
 * @obj: #MixVideoConfigParamsEnc object
 * @intra_period: Number of frames between key frames (GOP size)
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set Number of frames between key frames (GOP size)
 */
MIX_RESULT mix_videoconfigparamsenc_set_intra_period (
    MixVideoConfigParamsEnc * obj, uint intra_period);

/**
 * mix_videoconfigparamsenc_get_intra_period:
 * @obj: #MixVideoConfigParamsEnc object
 * @intra_period: Number of frames between key frames (GOP size) to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get Number of frames between key frames (GOP size)
 */
MIX_RESULT mix_videoconfigparamsenc_get_intra_period (
    MixVideoConfigParamsEnc * obj, uint *intra_period);

/**
 * mix_videoconfigparamsenc_set_buffer_pool_size:
 * @obj: #MixVideoConfigParamsEnc object
 * @bufpoolsize: Size of the pool of #MixBuffer objects
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set Size of the pool of #MixBuffer objects
 */
MIX_RESULT mix_videoconfigparamsenc_set_buffer_pool_size(
    MixVideoConfigParamsEnc * obj, uint bufpoolsize);

/**
 * mix_videoconfigparamsenc_set_buffer_pool_size:
 * @obj: #MixVideoConfigParamsEnc object
 * @bufpoolsize: Size of the pool of #MixBuffer objects to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get Size of the pool of #MixBuffer objects
 */
MIX_RESULT mix_videoconfigparamsenc_get_buffer_pool_size(
    MixVideoConfigParamsEnc * obj, uint *bufpoolsize);

/**
 * mix_videoconfigparamsenc_set_share_buf_mode:
 * @obj: #MixVideoConfigParamsEnc object
 * @share_buf_mod: A flag to indicate whether buffers are shared
 * between capture and encoding drivers or not
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set the flag that indicates whether buffers are shared between capture and encoding drivers or not
 */
MIX_RESULT mix_videoconfigparamsenc_set_share_buf_mode (
    MixVideoConfigParamsEnc * obj, bool share_buf_mod);

/**
 * mix_videoconfigparamsenc_get_share_buf_mode:
 * @obj: #MixVideoConfigParamsEnc object
 * @share_buf_mod: the flag to be returned that indicates whether buffers
 * are shared between capture and encoding drivers or not
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get the flag that indicates whether buffers are shared between capture and encoding drivers or not
 */
MIX_RESULT mix_videoconfigparamsenc_get_share_buf_mode(
    MixVideoConfigParamsEnc * obj, bool *share_buf_mod);

/**
 * mix_videoconfigparamsenc_set_ci_frame_info:
 * @obj: #MixVideoConfigParamsEnc object
 * @ci_frame_id: Array of frame IDs created by capture library *
 * @ci_frame_num: Size of the array ci_frame_id
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set CI frame information
 */
MIX_RESULT mix_videoconfigparamsenc_set_ci_frame_info(
    MixVideoConfigParamsEnc * obj, ulong * ci_frame_id, uint ci_frame_num);

/**
 * mix_videoconfigparamsenc_get_ci_frame_info:
 * @obj: #MixVideoConfigParamsEnc object
 * @ci_frame_id: Array of frame IDs created by capture library to be returned
 * @ci_frame_num: Size of the array ci_frame_id to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get CI frame information
 * <note>
 * Caller is responsible to g_free *ci_frame_id
 * </note>
 */
MIX_RESULT mix_videoconfigparamsenc_get_ci_frame_info (
    MixVideoConfigParamsEnc * obj, ulong * *ci_frame_id, uint *ci_frame_num);


/**
 * mix_videoconfigparamsenc_set_drawable:
 * @obj: #MixVideoConfigParamsEnc object
 * @draw: drawable
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set drawable
 */
MIX_RESULT mix_videoconfigparamsenc_set_drawable (
    MixVideoConfigParamsEnc * obj, ulong draw);

/**
 * mix_videoconfigparamsenc_get_drawable:
 * @obj: #MixVideoConfigParamsEnc object
 * @draw: drawable to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get drawable
 */
MIX_RESULT mix_videoconfigparamsenc_get_drawable (
    MixVideoConfigParamsEnc * obj, ulong *draw);

/**
 * mix_videoconfigparamsenc_set_need_display:
 * @obj: #MixVideoConfigParamsEnc object
 * @need_display: Flag to indicates whether MixVideoFrames suitable for displaying
 * need to be enqueued for retrieval using mix_video_get_frame()
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set the flag used to indicate whether MixVideoFrames suitable for displaying
 * need to be enqueued for retrieval using mix_video_get_frame()
 */
MIX_RESULT mix_videoconfigparamsenc_set_need_display (
    MixVideoConfigParamsEnc * obj, bool need_display);


/**
 * mix_videoconfigparamsenc_get_need_display:
 * @obj: #MixVideoConfigParamsEnc object
 * @need_display: A flag to be returned to indicates whether MixVideoFrames suitable for displaying
 * need to be enqueued for retrieval using mix_video_get_frame()
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get the flag used to indicate whether MixVideoFrames suitable for displaying
 * need to be enqueued for retrieval using mix_video_get_frame()
 */
MIX_RESULT mix_videoconfigparamsenc_get_need_display(
    MixVideoConfigParamsEnc * obj, bool *need_display);

/**
 * mix_videoconfigparamsenc_set_rate_control:
 * @obj: #MixVideoConfigParamsEnc object
 * @rcmode: Rate control mode
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set Rate control mode
 */
MIX_RESULT mix_videoconfigparamsenc_set_rate_control(
    MixVideoConfigParamsEnc * obj, MixRateControl rcmode);

/**
 * mix_videoconfigparamsenc_set_rate_control:
 * @obj: #MixVideoConfigParamsEnc object
 * @rcmode: Rate control mode to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get Rate control mode
 */
MIX_RESULT mix_videoconfigparamsenc_get_rate_control(
    MixVideoConfigParamsEnc * obj, MixRateControl * rcmode);

/**
 * mix_videoconfigparamsenc_set_raw_format:
 * @obj: #MixVideoConfigParamsEnc object
 * @raw_format: Raw format to be encoded
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set Raw format to be encoded
 */
MIX_RESULT mix_videoconfigparamsenc_set_raw_format (
    MixVideoConfigParamsEnc * obj, MixRawTargetFormat raw_format);

/**
 * mix_videoconfigparamsenc_get_raw_format:
 * @obj: #MixVideoConfigParamsEnc object
 * @raw_format: Raw format to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get Raw format
 */
MIX_RESULT mix_videoconfigparamsenc_get_raw_format (
    MixVideoConfigParamsEnc * obj, MixRawTargetFormat * raw_format);

/**
 * mix_videoconfigparamsenc_set_profile:
 * @obj: #MixVideoConfigParamsEnc object
 * @profile: Encoding profile
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set Encoding profile
 */
MIX_RESULT mix_videoconfigparamsenc_set_profile (
    MixVideoConfigParamsEnc * obj, MixProfile profile);

/**
 * mix_videoconfigparamsenc_get_profile:
 * @obj: #MixVideoConfigParamsEnc object
 * @profile: Encoding profile to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get Encoding profile
 */
MIX_RESULT mix_videoconfigparamsenc_get_profile (
    MixVideoConfigParamsEnc * obj, MixProfile * profile);


/**
 * mix_videoconfigparamsenc_set_level:
 * @obj: #MixVideoConfigParamsEnc object
 * @level: Encoding level
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set Encoding level
 */
MIX_RESULT mix_videoconfigparamsenc_set_level (
    MixVideoConfigParamsEnc * obj, uint8 level);


/**
 * mix_videoconfigparamsenc_get_level:
 * @obj: #MixVideoConfigParamsEnc object
 * @level: Encoding level to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get Encoding level
 */

MIX_RESULT mix_videoconfigparamsenc_get_level (
    MixVideoConfigParamsEnc * obj, uint8 * level);


/**
 * mix_videoconfigparamsenc_set_CIR_frame_cnt:
 * @obj: #MixVideoConfigParamsEnc object
 * @CIR_frame_cnt: Encoding CIR frame count
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set Encoding CIR frame count
 */
MIX_RESULT mix_videoconfigparamsenc_set_CIR_frame_cnt (
    MixVideoConfigParamsEnc * obj, uint CIR_frame_cnt);

/**
 * mix_videoconfigparamsenc_set_CIR_frame_cnt:
 * @obj: #MixVideoConfigParamsEnc object
 * @CIR_frame_cnt: Encoding CIR frame count to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get Encoding CIR frame count
 */

MIX_RESULT mix_videoconfigparamsenc_get_CIR_frame_cnt (
    MixVideoConfigParamsEnc * obj, uint * CIR_frame_cnt);


/**
 * mix_videoconfigparamsenc_set_max_slice_size:
 * @obj: #MixVideoConfigParamsEnc object
 * @max_slice_size: Maximum encoded slice size
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set Maximum encoded slice size
 */
MIX_RESULT mix_videoconfigparamsenc_set_max_slice_size (
    MixVideoConfigParamsEnc * obj, uint max_slice_size);

/**
 * mix_videoconfigparamsenc_get_max_slice_size:
 * @obj: #MixVideoConfigParamsEnc object
 * @max_slice_size: Maximum encoded slice size
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get Maximum encoded slice size
 */

MIX_RESULT mix_videoconfigparamsenc_get_max_slice_size (
    MixVideoConfigParamsEnc * obj, uint * max_slice_size);


/**
 * mix_videoconfigparamsenc_set_refresh_type:
 * @obj: #MixVideoConfigParamsEnc object
 * @refresh_type: The intra refresh type (CIR, AIR etc)
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set Intra Refresh Type
 */
MIX_RESULT mix_videoconfigparamsenc_set_refresh_type (
    MixVideoConfigParamsEnc * obj, MixVideoIntraRefreshType refresh_type);

/**
 * mix_videoconfigparamsenc_get_refresh_type:
 * @obj: #MixVideoConfigParamsEnc object
 * @refresh_type: The intra refresh type (CIR, AIR etc)
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get Intra Refresh Type
 */

MIX_RESULT mix_videoconfigparamsenc_get_refresh_type (
    MixVideoConfigParamsEnc * obj, MixVideoIntraRefreshType * refresh_type);

/**
 * mix_videoconfigparamsenc_set_AIR_params:
 * @obj: #MixVideoConfigParamsEnc object
 * @air_params: AIR Parameters, including air_MBs, air_threshold and air_auto
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set AIR parameters
 */
MIX_RESULT mix_videoconfigparamsenc_set_AIR_params (
    MixVideoConfigParamsEnc * obj, MixAIRParams air_params);

/**
 * mix_videoconfigparamsenc_get_AIR_params:
 * @obj: #MixVideoConfigParamsEnc object
 * @air_params: AIR Parameters, including air_MBs, air_threshold and air_auto
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get AIR parameters
 */

MIX_RESULT mix_videoconfigparamsenc_get_AIR_params (
    MixVideoConfigParamsEnc * obj, MixAIRParams * air_params);

/**
 * mix_videoconfigparamsenc_set_buffer_mode:
 * @obj: #MixVideoConfigParamsEnc object
 * @buffer_mode: Buffer allocation mode
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set buffer allocation mode
 */
MIX_RESULT mix_videoconfigparamsenc_set_buffer_mode (
    MixVideoConfigParamsEnc * obj, MixBufferAllocationMode buffer_mode);

/**
 * mix_videoconfigparamsenc_get_buffer_mode:
 * @obj: #MixVideoConfigParamsEnc object
 * @buffer_mode: Buffer allocation mode
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get buffer allocation mode
 */
MIX_RESULT mix_videoconfigparamsenc_get_buffer_mode (
    MixVideoConfigParamsEnc * obj, MixBufferAllocationMode * buffer_mode);


/**
 * mix_videoconfigparamsenc_set_upstream_buffer_info:
 * @obj: #MixVideoConfigParamsEnc object
 * @buffer_mode: Buffer allocation mode
 * @buf_info: Buffer information
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set buffer information according to the buffer mode
 */

MIX_RESULT mix_videoconfigparamsenc_set_upstream_buffer_info (
    MixVideoConfigParamsEnc * obj, MixBufferAllocationMode buffer_mode, void * buf_info);

/**
 * mix_videoconfigparamsenc_get_upstream_buffer_info:
 * @obj: #MixVideoConfigParamsEnc object
 * @buffer_mode: Buffer allocation mode
 * @buf_info: Buffer information
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get buffer information according to the buffer mode
 */
MIX_RESULT mix_videoconfigparamsenc_get_upstream_buffer_info (
    MixVideoConfigParamsEnc * obj, MixBufferAllocationMode buffer_mode, void ** buf_info);

/* TODO: Add getters and setters for other properties */
#endif /* __MIX_VIDEOCONFIGPARAMSENC_H__ */

