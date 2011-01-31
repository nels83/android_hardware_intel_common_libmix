/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

#ifndef __MIX_VIDEOFORMAT_H264_H__
#define __MIX_VIDEOFORMAT_H264_H__

#include "mixvideoformat.h"
#include "mixvideoframe_private.h"

#define DECODER_ROBUSTNESS


#define MIX_VIDEO_H264_SURFACE_NUM		20


#define MIX_VIDEOFORMAT_H264(obj)		(reinterpret_cast<MixVideoFormat_H264*>(obj))
#define MIX_IS_VIDEOFORMAT_H264(obj)	(NULL != MIX_VIDEOFORMAT_H264(obj))



class MixVideoFormat_H264 : public MixVideoFormat {
public:
	MixVideoFormat_H264();
	virtual ~MixVideoFormat_H264();
	
	virtual MIX_RESULT Initialize(
		MixVideoConfigParamsDec * config_params,
		MixFrameManager * frame_mgr,
		MixBufferPool * input_buf_pool,
		MixSurfacePool ** surface_pool,
		VADisplay va_display);
	virtual MIX_RESULT Decode(
		MixBuffer * bufin[], gint bufincnt, 
		MixVideoDecodeParams * decode_params);
	virtual MIX_RESULT Flush();
	virtual MIX_RESULT EndOfStream();

private:
	// Local Help Func
	MIX_RESULT _update_config_params(vbp_data_h264 *data);
	MIX_RESULT _initialize_va(vbp_data_h264 *data);
	MIX_RESULT _decode_a_buffer(MixBuffer * bufin, guint64 ts,
	gboolean discontinuity, MixVideoDecodeParams * decode_params);
	MIX_RESULT _decode_end(gboolean drop_picture);
	MIX_RESULT _handle_new_sequence(vbp_data_h264 *data);
	MIX_RESULT _decode_begin(vbp_data_h264 *data);
	MIX_RESULT _decode_continue(vbp_data_h264 *data);
	MIX_RESULT _set_frame_type(vbp_data_h264 *data);
	MIX_RESULT _set_frame_structure(vbp_data_h264 *data);
	MIX_RESULT _update_ref_pic_list(VAPictureParameterBufferH264* picture_params,
		VASliceParameterBufferH264* slice_params);
	MIX_RESULT _decode_a_slice(vbp_data_h264 *data,
		int picture_index, int slice_index);
	MIX_RESULT _cleanup_ref_frame(
		VAPictureParameterBufferH264* pic_params, MixVideoFrame * current_frame);
	MIX_RESULT _handle_ref_frames(
		VAPictureParameterBufferH264* pic_params,
		MixVideoFrame * current_frame);

public:
	/*< public > */
	/*< private > */
	GHashTable *dpb_surface_table;
#ifdef DECODER_ROBUSTNESS
	//Can improve which frame is used for this at a later time
	MixVideoFrame  *last_decoded_frame;  //last surface decoded, to be used as reference frame when reference frames are missing
#endif
};


/**
 * mix_videoformat_h264_new:
 * @returns: A newly allocated instance of #MixVideoFormat_H264
 *
 * Use this method to create new instance of #MixVideoFormat_H264
 */
MixVideoFormat_H264 *mix_videoformat_h264_new(void);

/**
 * mix_videoformat_h264_ref:
 * @mix: object to add reference
 * @returns: the MixVideoFormat_H264 instance where reference count has been increased.
 *
 * Add reference count.
 */
MixVideoFormat_H264 *mix_videoformat_h264_ref(MixVideoFormat_H264 * mix);

/**
 * mix_videoformat_h264_unref:
 * @obj: object to unref.
 *
 * Decrement reference count of the object.
 */
MixVideoFormat_H264* mix_videoformat_h264_unref(MixVideoFormat_H264 *mix);


/* Helper functions to manage the DPB table */
gboolean mix_videofmt_h264_check_in_DPB(gpointer key, gpointer value, gpointer user_data);
void mix_videofmt_h264_destroy_DPB_key(gpointer data);
void mix_videofmt_h264_destroy_DPB_value(gpointer data);
guint mix_videofmt_h264_get_poc(VAPictureH264 *pic);

#endif /* __MIX_VIDEOFORMAT_H264_H__ */
