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

G_BEGIN_DECLS

/**
 * MIX_TYPE_VIDEOCONFIGPARAMSDEC:
 *
 * Get type of class.
 */
#define MIX_TYPE_VIDEOCONFIGPARAMSDEC (mix_videoconfigparamsdec_get_type ())

/**
 * MIX_VIDEOCONFIGPARAMSDEC:
 * @obj: object to be type-casted.
 */
#define MIX_VIDEOCONFIGPARAMSDEC(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), MIX_TYPE_VIDEOCONFIGPARAMSDEC, MixVideoConfigParamsDec))

/**
 * MIX_IS_VIDEOCONFIGPARAMSDEC:
 * @obj: an object.
 *
 * Checks if the given object is an instance of #MixParams
 */
#define MIX_IS_VIDEOCONFIGPARAMSDEC(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MIX_TYPE_VIDEOCONFIGPARAMSDEC))

/**
 * MIX_VIDEOCONFIGPARAMSDEC_CLASS:
 * @klass: class to be type-casted.
 */
#define MIX_VIDEOCONFIGPARAMSDEC_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), MIX_TYPE_VIDEOCONFIGPARAMSDEC, MixVideoConfigParamsDecClass))

/**
 * MIX_IS_VIDEOCONFIGPARAMSDEC_CLASS:
 * @klass: a class.
 *
 * Checks if the given class is #MixParamsClass
 */
#define MIX_IS_VIDEOCONFIGPARAMSDEC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MIX_TYPE_VIDEOCONFIGPARAMSDEC))

/**
 * MIX_VIDEOCONFIGPARAMSDEC_GET_CLASS:
 * @obj: a #MixParams object.
 *
 * Get the class instance of the object.
 */
#define MIX_VIDEOCONFIGPARAMSDEC_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), MIX_TYPE_VIDEOCONFIGPARAMSDEC, MixVideoConfigParamsDecClass))

typedef struct _MixVideoConfigParamsDec MixVideoConfigParamsDec;
typedef struct _MixVideoConfigParamsDecClass MixVideoConfigParamsDecClass;

/**
 * MixVideoConfigParamsDec:
 *
 * MI-X VideoConfig Parameter object
 */
struct _MixVideoConfigParamsDec {
	/*< public > */
	MixVideoConfigParams parent;

	/*< public > */
	
	/* Frame re-ordering mode */
	MixFrameOrderMode frame_order_mode;
	
	/* Stream header information, such as 
	 * codec_data in GStreamer pipelines */ 
	MixIOVec header;

	/* Mime type */
	GString * mime_type;
	
	/* Frame rate numerator value */
	guint frame_rate_num;
	
	/* Frame rate denominator value */	
	guint frame_rate_denom;
	
	/* Picture width */
	gulong picture_width;
	
	/* Picture height */
	gulong picture_height;
	
	/* Render target format */
	guint raw_format;
	
	/* Rate control: CBR, VBR, none. Only valid for encoding.
	 * This should be set to none for decoding. */ 
	guint rate_control;

	/* Size of pool of MixBuffers to allocate */
	guint mixbuffer_pool_size;
	
	/* Extra surfaces for MixVideoFrame objects to be allocated */
	guint extra_surface_allocation;

    /* video range, 0 for short range and 1 for full range, output only */
	guint8 video_range;

    /* 
        color matrix, output only. Possible values defined in va.h
        #define VA_SRC_BT601            0x00000010
        #define VA_SRC_BT709            0x00000020
        #define VA_SRC_SMPTE_240     0x00000040
      */
    guint8  color_matrix;

    /* bit rate in bps, output only */
    guint8 bit_rate;

	/* Pixel aspect ratio numerator value */
	guint par_num;
	
	/* Pixel aspect ratio  denominator value */	
	guint par_denom;
	
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
 * MixVideoConfigParamsDecClass:
 *
 * MI-X VideoConfig object class
 */
struct _MixVideoConfigParamsDecClass {
	/*< public > */
	MixVideoConfigParamsClass parent_class;

	/* class members */
};

/**
 * mix_videoconfigparamsdec_get_type:
 * @returns: type
 *
 * Get the type of object.
 */
GType mix_videoconfigparamsdec_get_type(void);

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
		const gchar * mime_type);

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
		gchar ** mime_type);

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
		guint frame_rate_num, guint frame_rate_denom);

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
		guint * frame_rate_num, guint * frame_rate_denom);

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
		guint picture_width, guint picture_height);

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
		guint * picture_width, guint * picture_height);

/**
 * mix_videoconfigparamsdec_set_raw_format:
 * @obj: #MixVideoConfigParamsDec object
 * @raw_format: Render target format
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set Render target format
 */
MIX_RESULT mix_videoconfigparamsdec_set_raw_format(MixVideoConfigParamsDec * obj,
		guint raw_format);

/**
 * mix_videoconfigparamsdec_get_raw_format:
 * @obj: #MixVideoConfigParamsDec object
 * @raw_format: Render target format to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get Render target format
 */
MIX_RESULT mix_videoconfigparamsdec_get_raw_format(MixVideoConfigParamsDec * obj,
		guint *raw_format);

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
		guint rate_control);

/**
 * mix_videoconfigparamsdec_get_rate_control:
 * @obj: #MixVideoConfigParamsDec object
 * @rate_control: Rate control to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get rate control
 */
MIX_RESULT mix_videoconfigparamsdec_get_rate_control(MixVideoConfigParamsDec * obj,
		guint *rate_control);

/**
 * mix_videoconfigparamsdec_set_buffer_pool_size:
 * @obj: #MixVideoConfigParamsDec object
 * @bufpoolsize: Size of pool of #MixBuffers to allocate
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set buffer pool size
 */
MIX_RESULT mix_videoconfigparamsdec_set_buffer_pool_size(MixVideoConfigParamsDec * obj,
		guint bufpoolsize);

/**
 * mix_videoconfigparamsdec_get_buffer_pool_size:
 * @obj: #MixVideoConfigParamsDec object
 * @bufpoolsize: Size of pool of #MixBuffers to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get buffer pool size
 */
MIX_RESULT mix_videoconfigparamsdec_get_buffer_pool_size(MixVideoConfigParamsDec * obj,
		guint *bufpoolsize);

/**
 * mix_videoconfigparamsdec_set_extra_surface_allocation:
 * @obj: #MixVideoConfigParamsDec object
 * @extra_surface_allocation: Extra surfaces for #MixVideoFrame objects to be allocated
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set extra surface allocation
 */
MIX_RESULT mix_videoconfigparamsdec_set_extra_surface_allocation(MixVideoConfigParamsDec * obj,
		guint extra_surface_allocation);

/**
 * mix_videoconfigparamsdec_get_extra_surface_allocation:
 * @obj: #MixVideoConfigParamsDec object
 * @extra_surface_allocation: Extra surfaces for #MixVideoFrame objects to be retuned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get extra surface allocation
 */
MIX_RESULT mix_videoconfigparamsdec_get_extra_surface_allocation(MixVideoConfigParamsDec * obj,
		guint *extra_surface_allocation);


/**
 * mix_videoconfigparamsdec_set_video_range:
 * @obj: #MixVideoConfigParamsDec object
 * @video_range: 1 for full video range, 0 for short video range.
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set video range
 */
MIX_RESULT mix_videoconfigparamsdec_set_video_range(MixVideoConfigParamsDec * obj,
		guint8 video_range);

/**
 * mix_videoconfigparamsdec_get_video_range:
 * @obj: #MixVideoConfigParamsDec object
 * @video_range: video range to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get video range
 */
MIX_RESULT mix_videoconfigparamsdec_get_video_range(MixVideoConfigParamsDec * obj,
		guint8 *video_range);


/**
 * mix_videoconfigparamsdec_set_color_matrix:
 * @obj: #MixVideoConfigParamsDec object
 * @color_matrix: BT601 or BT709, defined in va.h. 0 for any other including unspecified color matrix.
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set color matrix
 */
MIX_RESULT mix_videoconfigparamsdec_set_color_matrix(MixVideoConfigParamsDec * obj,
		guint8 color_matrix);

/**
 * mix_videoconfigparamsdec_get_color_matrix:
 * @obj: #MixVideoConfigParamsDec object
 * @color_matrix: color matrix to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get color matrix
 */
MIX_RESULT mix_videoconfigparamsdec_get_color_matrix(MixVideoConfigParamsDec * obj,
		guint8 *color_matrix);


/**
 * mix_videoconfigparamsdec_set_bit_rate:
 * @obj: #MixVideoConfigParamsDec object
 * @bit_rate: bit rate in bit per second.
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set bit rate
 */
MIX_RESULT mix_videoconfigparamsdec_set_bit_rate(MixVideoConfigParamsDec * obj,
		guint bit_rate);

/**
 * mix_videoconfigparamsdec_get_bit_rate:
 * @obj: #MixVideoConfigParamsDec object
 * @bit_rate: bit rate to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get bit rate
 */
MIX_RESULT mix_videoconfigparamsdec_get_bit_rate(MixVideoConfigParamsDec * obj,
		guint *bit_rate);		



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
		guint par_num, guint par_denom);

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
		guint * par_num, guint * par_denom);
		

/* TODO: Add getters and setters for other properties */

G_END_DECLS

#endif /* __MIX_VIDEOCONFIGPARAMSDEC_H__ */
