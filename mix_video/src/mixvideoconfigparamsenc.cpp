/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

/**
 * SECTION:mixvideoconfigparamsenc
 * @short_description: MI-X Video Encode Configuration Parameter Base Object
 *
 * A base object of MI-X video encode configuration parameter objects.
 */


#include <string.h>
#include "mixvideolog.h"
#include "mixvideoconfigparamsenc.h"


#define MDEBUG

MixVideoConfigParamsEnc *
mix_videoconfigparamsenc_new(void) {
    return new MixVideoConfigParamsEnc();
}



MixVideoConfigParamsEnc *
mix_videoconfigparamsenc_ref(MixVideoConfigParamsEnc * mix) {
    return (MixVideoConfigParamsEnc *) mix_params_ref(MIX_PARAMS(mix));
}

/**
 * mix_videoconfigparamsenc_dup:
 * @obj: a #MixVideoConfigParamsEnc object
 * @returns: a newly allocated duplicate of the object.
 *
 * Copy duplicate of the object.
 */
MixParams *
mix_videoconfigparamsenc_dup(const MixParams * obj) {
	return NULL;
}

/**
 * mix_videoconfigparamsenc_copy:
 * @target: copy to target
 * @src: copy from src
 * @returns: boolean indicates if copy is successful.
 *
 * Copy instance data from @src to @target.
 */
gboolean mix_videoconfigparamsenc_copy(MixParams * target, const MixParams * src) {

    return FALSE;
}


/**
 * mix_videoconfigparamsenc_:
 * @first: first object to compare
 * @second: seond object to compare
 * @returns: boolean indicates if instance are equal.
 *
 * Copy instance data from @src to @target.
 */
gboolean mix_videoconfigparamsenc_equal(MixParams * first, MixParams * second) {

	gboolean ret = FALSE;

	MixVideoConfigParamsEnc *this_first, *this_second;

	if (NULL != first && NULL != second) {

		// Deep compare
		// Cast the base object to this child object
		this_first = MIX_VIDEOCONFIGPARAMSENC(first);
		this_second = MIX_VIDEOCONFIGPARAMSENC(second);

		/* check the equalitiy of the primitive type properties */
		if (this_first->bitrate != this_second->bitrate) {
			goto not_equal;
		}

		if (this_first->frame_rate_num != this_second->frame_rate_num) {
			goto not_equal;
		}

		if (this_first->frame_rate_denom != this_second->frame_rate_denom) {
			goto not_equal;
		}

		if (this_first->initial_qp != this_second->initial_qp) {
			goto not_equal;
		}

		if (this_first->min_qp != this_second->min_qp) {
			goto not_equal;
		}

		if (this_first->target_percentage != this_second->target_percentage) {
			goto not_equal;
		}

		if (this_first->window_size != this_second->window_size) {
			goto not_equal;
		}

		if (this_first->max_slice_size != this_second->max_slice_size) {
			goto not_equal;
		}

		if (this_first->intra_period != this_second->intra_period) {
			goto not_equal;
		}

		if (this_first->picture_width != this_second->picture_width
				&& this_first->picture_height != this_second->picture_height) {
			goto not_equal;
		}

		if (this_first->encode_format != this_second->encode_format) {
			goto not_equal;
		}

		if (this_first->mixbuffer_pool_size != this_second->mixbuffer_pool_size) {
			goto not_equal;
		}

		if (this_first->share_buf_mode != this_second->share_buf_mode) {
			goto not_equal;
		}

		if (this_first->ci_frame_id != this_second->ci_frame_id) {
			goto not_equal;
		}

		if (this_first->ci_frame_num != this_second->ci_frame_num) {
			goto not_equal;
		}

		if (this_first->draw != this_second->draw) {
			goto not_equal;
		}

		if (this_first->need_display!= this_second->need_display) {
			goto not_equal;
		}

        if (this_first->rate_control != this_second->rate_control) {
            goto not_equal;
        }

        if (this_first->raw_format != this_second->raw_format) {
            goto not_equal;
        }

        if (this_first->profile != this_second->profile) {
            goto not_equal;
        }

        if (this_first->level != this_second->level) {
            goto not_equal;
		}

        if (this_first->CIR_frame_cnt != this_second->CIR_frame_cnt) {
            goto not_equal;
        }

        if (this_first->refresh_type != this_second->refresh_type) {
            goto not_equal;
        }

        if (this_first->air_params.air_MBs != this_second->air_params.air_MBs) {
            goto not_equal;
        }

        if (this_first->air_params.air_threshold != this_second->air_params.air_threshold) {
            goto not_equal;
        }

        if (this_first->air_params.air_auto != this_second->air_params.air_auto) {
            goto not_equal;
        }

		/* check the equalitiy of the none-primitive type properties */

		/* compare mime_type */

		if (this_first->mime_type && this_second->mime_type) {
			if (g_string_equal(this_first->mime_type, this_second->mime_type)
					!= TRUE) {
				goto not_equal;
			}
		} else if (!(!this_first->mime_type && !this_second->mime_type)) {
			goto not_equal;
		}

		ret = TRUE;

		not_equal:

		if (ret != TRUE) {
			return ret;
		}

		/* chaining up. */
		return TRUE;
	}

	return ret;
}


/* TODO: Add getters and setters for other properties. The following is incomplete */


MIX_RESULT mix_videoconfigparamsenc_set_mime_type(MixVideoConfigParamsEnc * obj,
		const gchar * mime_type) {


	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_mime_type(MixVideoConfigParamsEnc * obj,
		gchar ** mime_type) {


	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_set_frame_rate(MixVideoConfigParamsEnc * obj,
		guint frame_rate_num, guint frame_rate_denom) {

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_frame_rate(MixVideoConfigParamsEnc * obj,
		guint * frame_rate_num, guint * frame_rate_denom) {

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_set_picture_res(MixVideoConfigParamsEnc * obj,
		guint picture_width, guint picture_height) {

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_picture_res(MixVideoConfigParamsEnc * obj,
        guint * picture_width, guint * picture_height) {

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_set_encode_format(MixVideoConfigParamsEnc * obj,
		MixEncodeTargetFormat encode_format) {

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_encode_format (MixVideoConfigParamsEnc * obj,
		MixEncodeTargetFormat* encode_format) {

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_set_bit_rate (MixVideoConfigParamsEnc * obj,
        guint bitrate) {

	return MIX_RESULT_SUCCESS;

}

MIX_RESULT mix_videoconfigparamsenc_get_bit_rate (MixVideoConfigParamsEnc * obj,
        guint *bitrate) {

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_set_init_qp (MixVideoConfigParamsEnc * obj,
        guint initial_qp) {

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_init_qp (MixVideoConfigParamsEnc * obj,
        guint *initial_qp) {

	return MIX_RESULT_SUCCESS;

}

MIX_RESULT mix_videoconfigparamsenc_set_min_qp (MixVideoConfigParamsEnc * obj,
        guint min_qp) {
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_min_qp(MixVideoConfigParamsEnc * obj,
        guint *min_qp) {
    return MIX_RESULT_NOT_SUPPORTED;
}


MIX_RESULT mix_videoconfigparamsenc_set_target_percentage (MixVideoConfigParamsEnc * obj,
        guint target_percentage) {

	return MIX_RESULT_SUCCESS;
	}


MIX_RESULT mix_videoconfigparamsenc_get_target_percentage(MixVideoConfigParamsEnc * obj,
        guint *target_percentage) {


    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_set_window_size (MixVideoConfigParamsEnc * obj,
        guint window_size) {


	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_window_size (MixVideoConfigParamsEnc * obj,
        guint *window_size) {



    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_set_intra_period (MixVideoConfigParamsEnc * obj,
        guint intra_period) {


	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_intra_period (MixVideoConfigParamsEnc * obj,
        guint *intra_period) {


	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_set_buffer_pool_size(
		MixVideoConfigParamsEnc * obj, guint bufpoolsize) {

	return MIX_RESULT_SUCCESS;

}

MIX_RESULT mix_videoconfigparamsenc_get_buffer_pool_size(
		MixVideoConfigParamsEnc * obj, guint *bufpoolsize) {

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_set_share_buf_mode (MixVideoConfigParamsEnc * obj,
		gboolean share_buf_mod) {

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_share_buf_mode(MixVideoConfigParamsEnc * obj,
		gboolean *share_buf_mod) {

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_set_ci_frame_info(MixVideoConfigParamsEnc * obj,
        gulong * ci_frame_id, guint ci_frame_num) {


	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_ci_frame_info (MixVideoConfigParamsEnc * obj,
        gulong * *ci_frame_id, guint *ci_frame_num) {

	return MIX_RESULT_SUCCESS;
}


MIX_RESULT mix_videoconfigparamsenc_set_drawable (MixVideoConfigParamsEnc * obj,
        gulong draw) {

	return MIX_RESULT_SUCCESS;

}

MIX_RESULT mix_videoconfigparamsenc_get_drawable (MixVideoConfigParamsEnc * obj,
        gulong *draw) {


	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_set_need_display (
	MixVideoConfigParamsEnc * obj, gboolean need_display) {

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_need_display(MixVideoConfigParamsEnc * obj,
		gboolean *need_display) {

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_set_rate_control(MixVideoConfigParamsEnc * obj,
		MixRateControl rate_control) {

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_rate_control(MixVideoConfigParamsEnc * obj,
		MixRateControl * rate_control) {

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_set_raw_format (MixVideoConfigParamsEnc * obj,
		MixRawTargetFormat raw_format) {

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_raw_format (MixVideoConfigParamsEnc * obj,
		MixRawTargetFormat * raw_format) {

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_set_profile (MixVideoConfigParamsEnc * obj,
		MixProfile profile) {

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_profile (MixVideoConfigParamsEnc * obj,
		MixProfile * profile) {

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_set_level (MixVideoConfigParamsEnc * obj,
		guint8 level) {

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_level (MixVideoConfigParamsEnc * obj,
		guint8 * level) {

	return MIX_RESULT_SUCCESS;
}


MIX_RESULT mix_videoconfigparamsenc_set_CIR_frame_cnt (MixVideoConfigParamsEnc * obj,
		guint CIR_frame_cnt) {

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_CIR_frame_cnt (MixVideoConfigParamsEnc * obj,
		guint * CIR_frame_cnt) {

	return MIX_RESULT_SUCCESS;
}


MIX_RESULT mix_videoconfigparamsenc_set_max_slice_size (MixVideoConfigParamsEnc * obj,
		guint max_slice_size) {

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_max_slice_size (MixVideoConfigParamsEnc * obj,
		guint * max_slice_size) {

	return MIX_RESULT_SUCCESS;
}


MIX_RESULT mix_videoconfigparamsenc_set_refresh_type(MixVideoConfigParamsEnc * obj,
		MixVideoIntraRefreshType refresh_type) {

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_refresh_type (MixVideoConfigParamsEnc * obj,
		MixVideoIntraRefreshType * refresh_type) {

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_set_AIR_params (MixVideoConfigParamsEnc * obj,
		MixAIRParams air_params) {

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_AIR_params (MixVideoConfigParamsEnc * obj,
		MixAIRParams * air_params) {

	return MIX_RESULT_SUCCESS;
}

