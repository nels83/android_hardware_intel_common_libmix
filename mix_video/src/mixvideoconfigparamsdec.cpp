/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

/**
 * SECTION:mixvideoconfigparamsdec
 * @short_description: MI-X Video Decode Configuration Parameter Base Object
 *
 * A base object of MI-X video decode configuration parameter objects.
 */

#include <string.h>
#include "mixvideolog.h"
#include "mixvideoconfigparamsdec.h"

#define MIX_VIDEOCONFIGPARAMSDEC_SETTER_CHECK_INPUT(obj) \
	if(!obj) return MIX_RESULT_NULL_PTR; \
	if(!MIX_IS_VIDEOCONFIGPARAMSDEC(obj)) return MIX_RESULT_FAIL; \

#define MIX_VIDEOCONFIGPARAMSDEC_GETTER_CHECK_INPUT(obj, prop) \
	if(!obj || !prop) return MIX_RESULT_NULL_PTR; \
	if(!MIX_IS_VIDEOCONFIGPARAMSDEC(obj)) return MIX_RESULT_FAIL; \

#define MIX_VIDEOCONFIGPARAMSDEC_GETTER_CHECK_INPUT_PAIR(obj, prop, prop2) \
	if(!obj || !prop || !prop2 ) return MIX_RESULT_NULL_PTR; \
	if(!MIX_IS_VIDEOCONFIGPARAMSDEC(obj)) return MIX_RESULT_FAIL; \


MixVideoConfigParamsDec::MixVideoConfigParamsDec() 
	:frame_order_mode(MIX_FRAMEORDER_MODE_DISPLAYORDER)
	,mime_type(NULL)
	,frame_rate_num(0)
	,frame_rate_denom(0)
	,picture_width(0)
	,picture_height(0)
	,raw_format(0)
	,rate_control(0)
	,mixbuffer_pool_size(0)
	,extra_surface_allocation(0)
	,video_range(0)
	,color_matrix(0)
	,bit_rate(0)
	,par_num(0)
	,par_denom(0)
	,reserved1(NULL)
	,reserved2(NULL)
	,reserved3(NULL)
	,reserved4(NULL)
	
{
	memset(&this->header, 0, sizeof(header));
}

MixVideoConfigParamsDec::~MixVideoConfigParamsDec() {
	/* free header */
	if (NULL != this->header.data) {
		g_free(this->header.data);
		memset(&this->header, 0, sizeof(this->header));
	}

	/* free mime_type */
	if (this->mime_type->str)
		g_string_free(this->mime_type, TRUE);
	else
		g_string_free(this->mime_type, FALSE);
}

gboolean MixVideoConfigParamsDec::copy(MixParams *target) const {
	MIX_RESULT mix_result = MIX_RESULT_FAIL;
	MixVideoConfigParamsDec *this_target = MIX_VIDEOCONFIGPARAMSDEC(target);
	LOG_V( "Begin\n");

	if (NULL != this_target) {
		/* copy properties of primitive type */
		this_target->frame_order_mode = this->frame_order_mode;
		this_target->frame_rate_num = this->frame_rate_num;
		this_target->frame_rate_denom = this->frame_rate_denom;
		this_target->picture_width = this->picture_width;
		this_target->picture_height = this->picture_height;
		this_target->raw_format = this->raw_format;
		this_target->rate_control = this->rate_control;
		this_target->mixbuffer_pool_size = this->mixbuffer_pool_size;
		this_target->extra_surface_allocation = this->extra_surface_allocation;
		this_target->video_range = this->video_range;
		this_target->color_matrix = this->color_matrix;
		this_target->bit_rate = this->bit_rate;
		this_target->par_num = this->par_num;
		this_target->par_denom = this->par_denom;

		/* copy properties of non-primitive */

		/* copy header */
		mix_result = mix_videoconfigparamsdec_set_header(this_target,
			const_cast<MixIOVec *>(&this->header));

		if (MIX_RESULT_SUCCESS != mix_result) {
			LOG_E( "set_header failed: mix_result = 0x%x\n", mix_result);
			return FALSE;
		}

		/* copy mime_type */
		if (NULL != this->mime_type) {
			mix_result = mix_videoconfigparamsdec_set_mime_type(this_target,
				this->mime_type->str);
		} else {
			mix_result = mix_videoconfigparamsdec_set_mime_type(this_target, NULL);
		}

		if (MIX_RESULT_SUCCESS != mix_result) {
			LOG_E( "set_mime_type failed: mix_result = 0x%x\n", mix_result);
			return FALSE;
		}
		
		/* TODO: copy other properties if there's any */

		/* Now chainup base class */
		return MixVideoConfigParams::copy(target);
	}

	LOG_V( "End\n");

	return FALSE;
}

gboolean MixVideoConfigParamsDec::equal(MixParams* obj) const {
	gboolean ret = FALSE;
	MixVideoConfigParamsDec *this_obj = MIX_VIDEOCONFIGPARAMSDEC(obj);

	if (NULL != this_obj) {
		// Deep compare

		/* check the equalitiy of the primitive type properties */
		if (this->frame_order_mode != this_obj->frame_order_mode) {
			goto not_equal;
		}

		if ((this->frame_rate_num != this_obj->frame_rate_num) &&
			(this->frame_rate_denom != this_obj->frame_rate_denom)) {
			goto not_equal;
		}

		if ((this->picture_width != this_obj->picture_width) &&
			(this->picture_height != this_obj->picture_height)) {
			goto not_equal;
		}

		if (this->raw_format != this_obj->raw_format) {
			goto not_equal;
		}

		if (this->rate_control != this_obj->rate_control) {
			goto not_equal;
		}

		if (this->mixbuffer_pool_size != this_obj->mixbuffer_pool_size) {
			goto not_equal;
		}

		if (this->extra_surface_allocation != this_obj->extra_surface_allocation) {
			goto not_equal;
		}

		/* check the equalitiy of the none-primitive type properties */

		/* MixIOVec header */

		if (this->header.data_size != this_obj->header.data_size) {
			goto not_equal;
		}

		if (this->header.buffer_size != this_obj->header.buffer_size) {
			goto not_equal;
		}

		if (this->header.data && this_obj->header.data) {
			if (memcmp(this->header.data, this_obj->header.data,
				this_obj->header.data_size) != 0) {
				goto not_equal;
			}
		} else if (!(!this->header.data && !this_obj->header.data)) {
			goto not_equal;
		}

		/* compare mime_type */
		if (this->mime_type && this_obj->mime_type) {
			if (g_string_equal(this->mime_type, this_obj->mime_type)
					!= TRUE) {
				goto not_equal;
			}
		} else if (!(!this->mime_type && !this_obj->mime_type)) {
			goto not_equal;
		}

		if (this->video_range != this_obj->video_range) {
			goto not_equal;
		}

		if (this->color_matrix != this_obj->color_matrix) {
			goto not_equal;
		}

		if (this->bit_rate != this_obj->bit_rate) {
			goto not_equal;
		}

		if (this->par_num != this_obj->par_num) {
			goto not_equal;
		}

		if (this->par_denom != this_obj->par_denom) {
			goto not_equal;
		}
		ret = TRUE;

not_equal:

		if (TRUE != ret) {
			return ret;
		}

		/* chaining up. */
		ret = MixVideoConfigParams::equal(obj);
	}

	return ret;
}

MixParams* MixVideoConfigParamsDec::dup() const {
	MixParams *ret = NULL;
	MixVideoConfigParamsDec *duplicate = new MixVideoConfigParamsDec();
	if (FALSE != copy(duplicate)) {
		ret = duplicate;
	} else {
		mix_videoconfigparamsdec_unref(duplicate);
	}
	return ret;
}

MixVideoConfigParamsDec *
mix_videoconfigparamsdec_new(void) {
	return new MixVideoConfigParamsDec();
}

MixVideoConfigParamsDec *
mix_videoconfigparamsdec_ref(MixVideoConfigParamsDec * mix) {
	return (MixVideoConfigParamsDec *) mix_params_ref(MIX_PARAMS(mix));
}


/* TODO: Add getters and setters for other properties. The following is incomplete */

MIX_RESULT mix_videoconfigparamsdec_set_frame_order_mode(
	MixVideoConfigParamsDec * obj, MixFrameOrderMode frame_order_mode) {
	MIX_VIDEOCONFIGPARAMSDEC_SETTER_CHECK_INPUT (obj);
	obj->frame_order_mode = frame_order_mode;
	LOG_V("obj->frame_order_mode = %d", obj->frame_order_mode);
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsdec_get_frame_order_mode(
	MixVideoConfigParamsDec * obj, MixFrameOrderMode * frame_order_mode) {
	MIX_VIDEOCONFIGPARAMSDEC_GETTER_CHECK_INPUT (obj, frame_order_mode);
	*frame_order_mode = obj->frame_order_mode;
	LOG_V("obj->frame_order_mode = %d", obj->frame_order_mode);
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsdec_set_header(
	MixVideoConfigParamsDec * obj, MixIOVec * header) {
	MIX_VIDEOCONFIGPARAMSDEC_SETTER_CHECK_INPUT (obj);

	if (!header) {
		return MIX_RESULT_NULL_PTR;
	}

	if (header->data && header->buffer_size) {
		obj->header.data = (guchar*)g_memdup(header->data, header->buffer_size);
		if (!obj->header.data) {
			return MIX_RESULT_NO_MEMORY;
		}
		obj->header.buffer_size = header->buffer_size;
		obj->header.data_size = header->data_size;
	}
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsdec_get_header(
	MixVideoConfigParamsDec * obj, MixIOVec ** header) {
	MIX_VIDEOCONFIGPARAMSDEC_GETTER_CHECK_INPUT (obj, header);

	if (obj->header.data && obj->header.buffer_size) {
		*header = (MixIOVec*)g_malloc(sizeof(MixIOVec));
		if (*header == NULL) {
			return MIX_RESULT_NO_MEMORY;
		}
		(*header)->data = (guchar*)g_memdup(obj->header.data, obj->header.buffer_size);
		(*header)->buffer_size = obj->header.buffer_size;
		(*header)->data_size = obj->header.data_size;
	} else {
		*header = NULL;
	}
	return MIX_RESULT_SUCCESS;

}

MIX_RESULT mix_videoconfigparamsdec_set_mime_type(
	MixVideoConfigParamsDec * obj, const gchar * mime_type) {
	MIX_VIDEOCONFIGPARAMSDEC_SETTER_CHECK_INPUT (obj);
	if (!mime_type) {
		return MIX_RESULT_NULL_PTR;
	}
	if (obj->mime_type) {
		if (obj->mime_type->str)
			g_string_free(obj->mime_type, TRUE);
		else
			g_string_free(obj->mime_type, FALSE);
	}
	obj->mime_type = g_string_new(mime_type);
	if (!obj->mime_type) {
		return MIX_RESULT_NO_MEMORY;
	}
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsdec_get_mime_type(
	MixVideoConfigParamsDec * obj, gchar ** mime_type) {
	MIX_VIDEOCONFIGPARAMSDEC_GETTER_CHECK_INPUT (obj, mime_type);
	if (!obj->mime_type) {
		*mime_type = NULL;
		return MIX_RESULT_SUCCESS;
	}
	*mime_type = g_strdup(obj->mime_type->str);
	if (!*mime_type) {
		return MIX_RESULT_NO_MEMORY;
	}
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsdec_set_frame_rate(
	MixVideoConfigParamsDec * obj, guint frame_rate_num,
	guint frame_rate_denom) {
	MIX_VIDEOCONFIGPARAMSDEC_SETTER_CHECK_INPUT (obj);
	obj->frame_rate_num = frame_rate_num;
	obj->frame_rate_denom = frame_rate_denom;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsdec_get_frame_rate(
	MixVideoConfigParamsDec * obj, guint * frame_rate_num,
	guint * frame_rate_denom) {
	MIX_VIDEOCONFIGPARAMSDEC_GETTER_CHECK_INPUT_PAIR (obj, frame_rate_num, frame_rate_denom);
	*frame_rate_num = obj->frame_rate_num;
	*frame_rate_denom = obj->frame_rate_denom;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsdec_set_picture_res(
	MixVideoConfigParamsDec * obj, guint picture_width,
	guint picture_height) {
	MIX_VIDEOCONFIGPARAMSDEC_SETTER_CHECK_INPUT (obj);
	obj->picture_width = picture_width;
	obj->picture_height = picture_height;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsdec_get_picture_res(
	MixVideoConfigParamsDec * obj, guint * picture_width,
	guint * picture_height) {
	MIX_VIDEOCONFIGPARAMSDEC_GETTER_CHECK_INPUT_PAIR (obj, picture_width, picture_height);
	*picture_width = obj->picture_width;
	*picture_height = obj->picture_height;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsdec_set_raw_format(
	MixVideoConfigParamsDec * obj, guint raw_format) {
	MIX_VIDEOCONFIGPARAMSDEC_SETTER_CHECK_INPUT (obj);

	/* TODO: check if the value of raw_format is valid */
	obj->raw_format = raw_format;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsdec_get_raw_format(
	MixVideoConfigParamsDec * obj, guint *raw_format) {
	MIX_VIDEOCONFIGPARAMSDEC_GETTER_CHECK_INPUT (obj, raw_format);
	*raw_format = obj->raw_format;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsdec_set_rate_control(
	MixVideoConfigParamsDec * obj, guint rate_control) {
	MIX_VIDEOCONFIGPARAMSDEC_SETTER_CHECK_INPUT (obj);

	/* TODO: check if the value of rate_control is valid */
	obj->rate_control = rate_control;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsdec_get_rate_control(
	MixVideoConfigParamsDec * obj, guint *rate_control) {
	MIX_VIDEOCONFIGPARAMSDEC_GETTER_CHECK_INPUT (obj, rate_control);
	*rate_control = obj->rate_control;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsdec_set_buffer_pool_size(
	MixVideoConfigParamsDec * obj, guint bufpoolsize) {
	MIX_VIDEOCONFIGPARAMSDEC_SETTER_CHECK_INPUT (obj);
	obj->mixbuffer_pool_size = bufpoolsize;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsdec_get_buffer_pool_size(
	MixVideoConfigParamsDec * obj, guint *bufpoolsize) {
	MIX_VIDEOCONFIGPARAMSDEC_GETTER_CHECK_INPUT (obj, bufpoolsize);
	*bufpoolsize = obj->mixbuffer_pool_size;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsdec_set_extra_surface_allocation(
	MixVideoConfigParamsDec * obj, guint extra_surface_allocation) {
	MIX_VIDEOCONFIGPARAMSDEC_SETTER_CHECK_INPUT (obj);
	obj->extra_surface_allocation = extra_surface_allocation;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsdec_get_extra_surface_allocation(
	MixVideoConfigParamsDec * obj, guint *extra_surface_allocation) {
	MIX_VIDEOCONFIGPARAMSDEC_GETTER_CHECK_INPUT (obj, extra_surface_allocation);
	*extra_surface_allocation = obj->extra_surface_allocation;
	return MIX_RESULT_SUCCESS;
}


MIX_RESULT mix_videoconfigparamsdec_set_video_range(
	MixVideoConfigParamsDec * obj, guint8 video_range) {
	MIX_VIDEOCONFIGPARAMSDEC_SETTER_CHECK_INPUT (obj);
	obj->video_range = video_range;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsdec_get_video_range(
	MixVideoConfigParamsDec * obj, guint8 *video_range) {
	MIX_VIDEOCONFIGPARAMSDEC_GETTER_CHECK_INPUT (obj, video_range);
	*video_range = obj->video_range;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsdec_set_color_matrix(
	MixVideoConfigParamsDec * obj, guint8 color_matrix) {
	MIX_VIDEOCONFIGPARAMSDEC_SETTER_CHECK_INPUT (obj);
	obj->color_matrix = color_matrix;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsdec_get_color_matrix(
	MixVideoConfigParamsDec * obj, guint8 *color_matrix) {
	MIX_VIDEOCONFIGPARAMSDEC_GETTER_CHECK_INPUT (obj, color_matrix);
	*color_matrix = obj->color_matrix;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsdec_set_bit_rate(
	MixVideoConfigParamsDec * obj, guint bit_rate) {
	MIX_VIDEOCONFIGPARAMSDEC_SETTER_CHECK_INPUT (obj);
	obj->bit_rate = bit_rate;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsdec_get_bit_rate(
	MixVideoConfigParamsDec * obj, guint *bit_rate) {
	MIX_VIDEOCONFIGPARAMSDEC_GETTER_CHECK_INPUT (obj, bit_rate);
	*bit_rate = obj->bit_rate;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsdec_set_pixel_aspect_ratio(
	MixVideoConfigParamsDec * obj, guint par_num, guint par_denom) {
	MIX_VIDEOCONFIGPARAMSDEC_SETTER_CHECK_INPUT (obj);
	obj->par_num = par_num;
	obj->par_denom = par_denom;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsdec_get_pixel_aspect_ratio(
	MixVideoConfigParamsDec * obj, guint * par_num, guint * par_denom) {
	MIX_VIDEOCONFIGPARAMSDEC_GETTER_CHECK_INPUT_PAIR (obj, par_num, par_denom);
	*par_num = obj->par_num;
	*par_denom = obj->par_denom;
	return MIX_RESULT_SUCCESS;
}


