/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

#ifndef __MIX_VIDEOCONFIGPARAMSDEC_H__
#define __MIX_VIDEOCONFIGPARAMSDEC_H__

#include <mixvideoconfigparams.h>
#include "mixvideodef.h"


/**
 * MIX_VIDEOCONFIGPARAMSDEC:
 * @obj: object to be type-casted.
 */
#define MIX_VIDEOCONFIGPARAMSDEC(obj) (reinterpret_cast<MixVideoConfigParamsDec*>(obj))

/**
 * MIX_IS_VIDEOCONFIGPARAMSDEC:
 * @obj: an object.
 *
 * Checks if the given object is an instance of #MixParams
 */
#define MIX_IS_VIDEOCONFIGPARAMSDEC(obj) ((NULL != MIX_VIDEOCONFIGPARAMSDEC(obj)) ? TRUE : FALSE)

/**
 * MixVideoConfigParamsDec:
 *
 * MI-X VideoConfig Parameter object
 */
class MixVideoConfigParamsDec : public MixVideoConfigParams {
public:
    MixVideoConfigParamsDec();
    ~MixVideoConfigParamsDec();
    virtual bool copy(MixParams *target) const;
    virtual bool equal(MixParams* obj) const;
    virtual MixParams* dup() const;
public:
    /*< public > */
    //MixVideoConfigParams parent;

    /*< public > */

    /* Frame re-ordering mode */
    MixFrameOrderMode frame_order_mode;

    /* Stream header information, such as
     * codec_data in GStreamer pipelines */
    MixIOVec header;

    /* Mime type */
    char * mime_type;

    /* Frame rate numerator value */
    uint frame_rate_num;

    /* Frame rate denominator value */
    uint frame_rate_denom;

    /* Picture width */
    ulong picture_width;

    /* Picture height */
    ulong picture_height;

    /* Render target format */
    uint raw_format;

    /* Rate control: CBR, VBR, none. Only valid for encoding.
     * This should be set to none for decoding. */
    uint rate_control;

    /* Size of pool of MixBuffers to allocate */
    uint mixbuffer_pool_size;

    /* Extra surfaces for MixVideoFrame objects to be allocated */
    uint extra_surface_allocation;

    /* video range, 0 for short range and 1 for full range, output only */
    uint8 video_range;

    /*
        color matrix, output only. Possible values defined in va.h
        #define VA_SRC_BT601            0x00000010
        #define VA_SRC_BT709            0x00000020
        #define VA_SRC_SMPTE_240     0x00000040
      */
    uint8  color_matrix;

    /* bit rate in bps, output only */
    uint bit_rate;

    /* Pixel aspect ratio numerator value */
    uint par_num;

    /* Pixel aspect ratio  denominator value */
    uint par_denom;

    uint crop_left;
    uint crop_right;
    uint crop_top;
    uint crop_bottom;

    /* Error concealment enabled/disabled */
    bool error_concealment;

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
 * mix_videoconfigparamsdec_get_type:
 * @returns: type
 *
 * Get the type of object.
 */
//GType mix_videoconfigparamsdec_get_type(void);

/**
 * mix_videoconfigparamsdec_new:
 * @returns: A newly allocated instance of #MixVideoConfigParamsDec
 *
 * Use this method to create new instance of #MixVideoConfigParamsDec
 */
MixVideoConfigParamsDec *mix_videoconfigparamsdec_new(void);
/**
 * mix_videoconfigparamsdec_ref:
 * @mix: object to add reference
 * @returns: the #MixVideoConfigParamsDec instance where reference count has been increased.
 *
 * Add reference count.
 */
MixVideoConfigParamsDec *mix_videoconfigparamsdec_ref(MixVideoConfigParamsDec * mix);

/**
 * mix_videoconfigparamsdec_unref:
 * @obj: object to unref.
 *
 * Decrement reference count of the object.
 */
#define mix_videoconfigparamsdec_unref(obj) mix_params_unref(MIX_PARAMS(obj))

/* Class Methods */


/**
 * mix_videoconfigparamsdec_set_frame_order_mode:
 * @obj: #MixVideoConfigParamsDec object
 * @frame_order_mode: Frame re-ordering mode
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set frame order mode.
 */
MIX_RESULT mix_videoconfigparamsdec_set_frame_order_mode(
    MixVideoConfigParamsDec * obj, MixFrameOrderMode frame_order_mode);

/**
 * mix_videoconfigparamsdec_get_frame_order_mode:
 * @obj: #MixVideoConfigParamsDec object
 * @frame_order_mode: pointer to frame re-ordering mode
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get frame order mode.
 */
MIX_RESULT mix_videoconfigparamsdec_get_frame_order_mode(
    MixVideoConfigParamsDec * obj, MixFrameOrderMode * frame_order_mode);

/**
 * mix_videoconfigparamsdec_set_header:
 * @obj: #MixVideoConfigParamsDec object
 * @header: Stream header information, such as codec_data in GStreamer pipelines
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set stream header information.
 */
MIX_RESULT mix_videoconfigparamsdec_set_header(MixVideoConfigParamsDec * obj,
        MixIOVec *header);

/**
 * mix_videoconfigparamsdec_get_header:
 * @obj: #MixVideoConfigParamsDec object
 * @header: Pointer to pointer of Stream header information
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get stream header information.
 * <note>
 * Caller is responsible to g_free (*header)->data field and *header
 * </note>
 */
MIX_RESULT mix_videoconfigparamsdec_get_header(MixVideoConfigParamsDec * obj,
        MixIOVec ** header);

/**
 * mix_videoconfigparamsdec_set_mime_type:
 * @obj: #MixVideoConfigParamsDec object
 * @mime_type: mime type
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set stream mime type
 */
MIX_RESULT mix_videoconfigparamsdec_set_mime_type(MixVideoConfigParamsDec * obj,
        const char * mime_type);

/**
 * mix_videoconfigparamsdec_get_mime_type:
 * @obj: #MixVideoConfigParamsDec object
 * @mime_type: Pointer to pointer of type gchar
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get mime type
 * <note>
 * Caller is responsible to g_free *mime_type
 * </note>
 */
MIX_RESULT mix_videoconfigparamsdec_get_mime_type(MixVideoConfigParamsDec * obj,
        char ** mime_type);

/**
 * mix_videoconfigparamsdec_set_frame_rate:
 * @obj: #MixVideoConfigParamsDec object
 * @frame_rate_num: Frame rate numerator value
 * @frame_rate_denom: Frame rate denominator value *
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set frame rate
 */
MIX_RESULT mix_videoconfigparamsdec_set_frame_rate(MixVideoConfigParamsDec * obj,
        uint frame_rate_num, uint frame_rate_denom);

/**
 * mix_videoconfigparamsdec_get_frame_rate:
 * @obj: #MixVideoConfigParamsDec object
 * @frame_rate_num: Frame rate numerator value to be returned
 * @frame_rate_denom: Frame rate denominator value to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get frame rate
 */
MIX_RESULT mix_videoconfigparamsdec_get_frame_rate(MixVideoConfigParamsDec * obj,
        uint * frame_rate_num, uint * frame_rate_denom);

/**
 * mix_videoconfigparamsdec_set_picture_res:
 * @obj: #MixVideoConfigParamsDec object
 * @picture_width: Picture width
 * @picture_height: Picture height
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set video resolution
 */
MIX_RESULT mix_videoconfigparamsdec_set_picture_res(MixVideoConfigParamsDec * obj,
        uint picture_width, uint picture_height);

/**
 * mix_videoconfigparamsdec_get_picture_res:
 * @obj: #MixVideoConfigParamsDec object
 * @picture_width: Picture width to be returned
 * @picture_height: Picture height to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get video resolution
 */
MIX_RESULT mix_videoconfigparamsdec_get_picture_res(MixVideoConfigParamsDec * obj,
        uint * picture_width, uint * picture_height);

/**
 * mix_videoconfigparamsdec_set_raw_format:
 * @obj: #MixVideoConfigParamsDec object
 * @raw_format: Render target format
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set Render target format
 */
MIX_RESULT mix_videoconfigparamsdec_set_raw_format(MixVideoConfigParamsDec * obj,
        uint raw_format);

/**
 * mix_videoconfigparamsdec_get_raw_format:
 * @obj: #MixVideoConfigParamsDec object
 * @raw_format: Render target format to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get Render target format
 */
MIX_RESULT mix_videoconfigparamsdec_get_raw_format(MixVideoConfigParamsDec * obj,
        uint *raw_format);

/**
 * mix_videoconfigparamsdec_set_rate_control:
 * @obj: #MixVideoConfigParamsDec object
 * @rate_control: Rate control: CBR, VBR, none. Only valid for encoding.
 *                This should be set to none for decoding.
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set rate control
 */
MIX_RESULT mix_videoconfigparamsdec_set_rate_control(MixVideoConfigParamsDec * obj,
        uint rate_control);

/**
 * mix_videoconfigparamsdec_get_rate_control:
 * @obj: #MixVideoConfigParamsDec object
 * @rate_control: Rate control to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get rate control
 */
MIX_RESULT mix_videoconfigparamsdec_get_rate_control(MixVideoConfigParamsDec * obj,
        uint *rate_control);

/**
 * mix_videoconfigparamsdec_set_buffer_pool_size:
 * @obj: #MixVideoConfigParamsDec object
 * @bufpoolsize: Size of pool of #MixBuffers to allocate
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set buffer pool size
 */
MIX_RESULT mix_videoconfigparamsdec_set_buffer_pool_size(MixVideoConfigParamsDec * obj,
        uint bufpoolsize);

/**
 * mix_videoconfigparamsdec_get_buffer_pool_size:
 * @obj: #MixVideoConfigParamsDec object
 * @bufpoolsize: Size of pool of #MixBuffers to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get buffer pool size
 */
MIX_RESULT mix_videoconfigparamsdec_get_buffer_pool_size(MixVideoConfigParamsDec * obj,
        uint *bufpoolsize);

/**
 * mix_videoconfigparamsdec_set_extra_surface_allocation:
 * @obj: #MixVideoConfigParamsDec object
 * @extra_surface_allocation: Extra surfaces for #MixVideoFrame objects to be allocated
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set extra surface allocation
 */
MIX_RESULT mix_videoconfigparamsdec_set_extra_surface_allocation(MixVideoConfigParamsDec * obj,
        uint extra_surface_allocation);

/**
 * mix_videoconfigparamsdec_get_extra_surface_allocation:
 * @obj: #MixVideoConfigParamsDec object
 * @extra_surface_allocation: Extra surfaces for #MixVideoFrame objects to be retuned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get extra surface allocation
 */
MIX_RESULT mix_videoconfigparamsdec_get_extra_surface_allocation(MixVideoConfigParamsDec * obj,
        uint *extra_surface_allocation);


/**
 * mix_videoconfigparamsdec_set_video_range:
 * @obj: #MixVideoConfigParamsDec object
 * @video_range: 1 for full video range, 0 for short video range.
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set video range
 */
MIX_RESULT mix_videoconfigparamsdec_set_video_range(MixVideoConfigParamsDec * obj,
        uint8 video_range);

/**
 * mix_videoconfigparamsdec_get_video_range:
 * @obj: #MixVideoConfigParamsDec object
 * @video_range: video range to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get video range
 */
MIX_RESULT mix_videoconfigparamsdec_get_video_range(MixVideoConfigParamsDec * obj,
        uint8 *video_range);


/**
 * mix_videoconfigparamsdec_set_color_matrix:
 * @obj: #MixVideoConfigParamsDec object
 * @color_matrix: BT601 or BT709, defined in va.h. 0 for any other including unspecified color matrix.
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set color matrix
 */
MIX_RESULT mix_videoconfigparamsdec_set_color_matrix(MixVideoConfigParamsDec * obj,
        uint8 color_matrix);

/**
 * mix_videoconfigparamsdec_get_color_matrix:
 * @obj: #MixVideoConfigParamsDec object
 * @color_matrix: color matrix to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get color matrix
 */
MIX_RESULT mix_videoconfigparamsdec_get_color_matrix(MixVideoConfigParamsDec * obj,
        uint8 *color_matrix);


/**
 * mix_videoconfigparamsdec_set_bit_rate:
 * @obj: #MixVideoConfigParamsDec object
 * @bit_rate: bit rate in bit per second.
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set bit rate
 */
MIX_RESULT mix_videoconfigparamsdec_set_bit_rate(MixVideoConfigParamsDec * obj,
        uint bit_rate);

/**
 * mix_videoconfigparamsdec_get_bit_rate:
 * @obj: #MixVideoConfigParamsDec object
 * @bit_rate: bit rate to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get bit rate
 */
MIX_RESULT mix_videoconfigparamsdec_get_bit_rate(MixVideoConfigParamsDec * obj,
        uint *bit_rate);



/**
 * mix_videoconfigparamsdec_set_pixel_aspect_ratio:
 * @obj: #MixVideoConfigParamsDec object
 * @par_num: Pixel aspect ratio numerator value
 * @par_denom: Pixel aspect ratio denominator value *
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set pixel aspect ratio
 */
MIX_RESULT mix_videoconfigparamsdec_set_pixel_aspect_ratio(MixVideoConfigParamsDec * obj,
        uint par_num, uint par_denom);

/**
 * mix_videoconfigparamsdec_get_pixel_aspect_ratio:
 * @obj: #MixVideoConfigParamsDec object
 * @par_num: Pixel aspect ratio  numerator value to be returned
 * @par_denom: Pixel aspect ratio denominator value to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get pixel aspect ratio
 */
MIX_RESULT mix_videoconfigparamsdec_get_pixel_aspect_ratio(MixVideoConfigParamsDec * obj,
        uint * par_num, uint * par_denom);

/**
 * mix_videoconfigparamsdec_set_cropping_info:
 * @obj: #MixVideoConfigParamsDec object
 * @crop_left: left cropping value
 * @crop_right: right cropping value
 * @crop_top: top cropping value
 * @crop_bottom: bottom cropping value
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set cropping information
 */
MIX_RESULT mix_videoconfigparamsdec_set_cropping_info(MixVideoConfigParamsDec * obj,
        uint crop_left, uint crop_right, uint crop_top, uint crop_bottom);

/**
 * mix_videoconfigparamsdec_get_cropping_info:
 * @obj: #MixVideoConfigParamsDec object
 * @crop_left: left cropping value
 * @crop_right: right cropping value
 * @crop_top: top cropping value
 * @crop_bottom: bottom cropping value
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get cropping information
 */
MIX_RESULT mix_videoconfigparamsdec_get_cropping_info(MixVideoConfigParamsDec * obj,
        uint *crop_left, uint *crop_right, uint *crop_top, uint *crop_bottom);


/**
 * mix_videoconfigparamsdec_set_error_concealment:
 * @obj: #MixVideoConfigParamsDec object
 * @error_concealment: A flag to indicate whether error concealment is enabled for decoder
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set the flag that indicates whether error concealment is enabled
 */
MIX_RESULT mix_videoconfigparamsdec_set_error_concealment (MixVideoConfigParamsDec * obj,
        bool error_concealment);

/**
 * mix_videoconfigparamsdec_get_error_concealment:
 * @obj: #MixVideoConfigParamsDec object
 * @error_concealment: the flag to be returned that indicates error concealment is enabled for decoder
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get the flag that indicates whether error concealment is enabled
 */
MIX_RESULT mix_videoconfigparamsdec_get_error_concealment(MixVideoConfigParamsDec * obj,
        bool *error_concealment);


/* TODO: Add getters and setters for other properties */

#endif /* __MIX_VIDEOCONFIGPARAMSDEC_H__ */
