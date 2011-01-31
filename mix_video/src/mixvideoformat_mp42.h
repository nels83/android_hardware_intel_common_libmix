/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

#ifndef __MIX_VIDEOFORMAT_MP42_H__
#define __MIX_VIDEOFORMAT_MP42_H__

#include "mixvideoformat.h"
#include "mixvideoframe_private.h"

//Note: this is only a max limit.  Real number of surfaces allocated is calculated in mix_videoformat_mp42_initialize()
#define MIX_VIDEO_MP42_SURFACE_NUM	8

/*
 * Type macros.
 */
#define MIX_VIDEOFORMAT_MP42(obj)		(reinterpret_cast<MixVideoFormat_MP42*>(obj))
#define MIX_IS_VIDEOFORMAT_MP42(obj)	(NULL != MIX_VIDEOFORMAT_MP42(obj))

class MixVideoFormat_MP42 : public MixVideoFormat {
public:
	MixVideoFormat_MP42();
	virtual ~MixVideoFormat_MP42();

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
	MIX_RESULT _handle_ref_frames(
		enum _picture_type frame_type, MixVideoFrame * current_frame);
	MIX_RESULT _release_input_buffers(guint64 timestamp);
	MIX_RESULT _update_config_params(vbp_data_mp42 *data);
	MIX_RESULT _initialize_va(vbp_data_mp42 *data);
	MIX_RESULT _decode_a_slice(
		vbp_data_mp42* data, vbp_picture_data_mp42* pic_data);
	MIX_RESULT _decode_end(gboolean drop_picture);
	MIX_RESULT _decode_continue(vbp_data_mp42 *data);
	MIX_RESULT _decode_begin(vbp_data_mp42* data);
	MIX_RESULT _decode_a_buffer(
		MixBuffer * bufin, guint64 ts, gboolean discontinuity);

public:
	/*< public > */
	MixVideoFrame * reference_frames[2];
	MixVideoFrame * last_frame;
	gint last_vop_coding_type;
	guint last_vop_time_increment;

	/* indicate if future n-vop is a placeholder of a packed frame */
	gboolean next_nvop_for_PB_frame;

	/* indicate if iq_matrix_buffer is sent to driver */
	gboolean iq_matrix_buf_sent;
};


/**
 * mix_videoformat_mp42_new:
 * @returns: A newly allocated instance of #MixVideoFormat_MP42
 *
 * Use this method to create new instance of #MixVideoFormat_MP42
 */
MixVideoFormat_MP42 *mix_videoformat_mp42_new(void);

/**
 * mix_videoformat_mp42_ref:
 * @mix: object to add reference
 * @returns: the MixVideoFormat_MP42 instance where reference count has been increased.
 *
 * Add reference count.
 */
MixVideoFormat_MP42 *mix_videoformat_mp42_ref(MixVideoFormat_MP42 * mix);

/**
 * mix_videoformat_mp42_unref:
 * @obj: object to unref.
 *
 * Decrement reference count of the object.
 */
MixVideoFormat_MP42 *mix_videoformat_mp42_unref(MixVideoFormat_MP42 * mix);

#endif /* __MIX_VIDEOFORMAT_MP42_H__ */
