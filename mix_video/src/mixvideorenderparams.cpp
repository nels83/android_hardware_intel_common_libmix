/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

/**
 * SECTION:mixvideorenderparams
 * @short_description: MI-X Video Render Parameters
 *
 * The #MixVideoRenderParams object  will be created by the MMF/App 
 * and provided to #MixVideo in the #MixVideo mix_video_render() function.
 */

#include <va/va.h>             /* libVA */
#include "mixvideorenderparams.h"
#include "mixvideorenderparams_internal.h"

#include <string.h>

#define MIX_VIDEORENDERPARAMS_SETTER_CHECK_INPUT(obj) \
	if(!obj) return MIX_RESULT_NULL_PTR; \
	if(!MIX_IS_VIDEORENDERPARAMS(obj)) return MIX_RESULT_FAIL; \

#define MIX_VIDEORENDERPARAMS_GETTER_CHECK_INPUT(obj, prop) \
	if(!obj || !prop) return MIX_RESULT_NULL_PTR; \
	if(!MIX_IS_VIDEORENDERPARAMS(obj)) return MIX_RESULT_FAIL; \

gboolean mix_rect_equal(MixRect rc1, MixRect rc2);

MixVideoRenderParams::MixVideoRenderParams()
	:display(NULL)
	,clipping_rects(NULL)
	,number_of_clipping_rects(0)
	,reserved(NULL)
	,reserved1(NULL)
	,reserved2(NULL)
	,reserved3(NULL)
	,reserved4(NULL)
	,mVa_cliprects(NULL) {
	/* initialize properties here */
	memset(&src_rect, 0, sizeof(MixRect));
	memset(&dst_rect, 0, sizeof(MixRect));
}

MixVideoRenderParams::~MixVideoRenderParams() {
	if (NULL != clipping_rects) {
		g_free(clipping_rects);
		clipping_rects = NULL;
	}
	if(NULL != mVa_cliprects) {
		g_free(mVa_cliprects);
		mVa_cliprects = NULL;
	}
	number_of_clipping_rects = 0;
	if (NULL != display) {
		mix_display_unref(display);
		display = NULL;
	}	
}

gboolean MixVideoRenderParams::copy(MixParams *target) const {
	if (NULL == target) return FALSE;
	MixVideoRenderParams *this_target = MIX_VIDEORENDERPARAMS(target);
	MIX_RESULT mix_result = MIX_RESULT_FAIL;

	if (this_target == this) {
		return TRUE;
	}

	if(NULL != this_target) {
		mix_result = mix_videorenderparams_set_display(this_target, display);
		if (MIX_RESULT_SUCCESS != mix_result) {
			return FALSE;
		}
		
		mix_result = mix_videorenderparams_set_clipping_rects(this_target,
			clipping_rects, number_of_clipping_rects);

		if (MIX_RESULT_SUCCESS != mix_result) {
			return FALSE;
		}

		this_target->src_rect = src_rect;
		this_target->dst_rect = dst_rect;
		
	}
	return MixParams::copy(target);
}

gboolean MixVideoRenderParams::equal(MixParams* obj) const {
	gboolean ret = FALSE;
	MixVideoRenderParams *this_obj = MIX_VIDEORENDERPARAMS(obj);
	if (NULL != this_obj) {
		// Deep compare
		if (mix_display_equal(MIX_DISPLAY(display), MIX_DISPLAY(
				this_obj->display)) && mix_rect_equal(src_rect,
				this_obj->src_rect) && mix_rect_equal(dst_rect,
				this_obj->dst_rect) && number_of_clipping_rects
				== this_obj->number_of_clipping_rects && memcmp(
				(guchar *) number_of_clipping_rects,
				(guchar *) this_obj->number_of_clipping_rects,
				number_of_clipping_rects) == 0) {
			// members within this scope equal. chaining up.
			ret = MixParams::equal(obj);
		}
	}
	return ret;
}

MixParams* MixVideoRenderParams::dup() const {
	MixParams *ret = NULL;
	MixVideoRenderParams *duplicate = mix_videorenderparams_new();
	if (copy(duplicate)) {
		ret = duplicate;
	} else {
		mix_videorenderparams_unref(duplicate);
	}
	return ret;
}

MIX_RESULT MixVideoRenderParams::set_clipping_rects(
	MixRect* clipping_rects, 
	guint number_of_clipping_rects) {

	if (this->clipping_rects) {
		g_free(this->clipping_rects);
		this->clipping_rects = NULL;
		this->number_of_clipping_rects = 0;
	}

	if(this->mVa_cliprects) {
		g_free(this->mVa_cliprects);
		this->mVa_cliprects = NULL;
	}

	if ((NULL == clipping_rects) && (0 != number_of_clipping_rects)) {
		this->clipping_rects = reinterpret_cast<MixRect*>(g_memdup(clipping_rects,
			number_of_clipping_rects * sizeof(MixRect)));
		if (NULL == this->clipping_rects) {
			return MIX_RESULT_NO_MEMORY;
		}
		this->number_of_clipping_rects = number_of_clipping_rects;

		/* create VARectangle list */
		this->mVa_cliprects = reinterpret_cast<VARectangle*>(g_malloc(number_of_clipping_rects * sizeof(VARectangle)));
		if (NULL == this->mVa_cliprects) {
			return MIX_RESULT_NO_MEMORY;
		}

		for (guint idx = 0; idx < number_of_clipping_rects; ++idx) {
			this->mVa_cliprects[idx].x = clipping_rects[idx].x;
			this->mVa_cliprects[idx].y = clipping_rects[idx].y;
			this->mVa_cliprects[idx].width = clipping_rects[idx].width;
			this->mVa_cliprects[idx].height = clipping_rects[idx].height;
		}
	}

	return MIX_RESULT_SUCCESS;
}


MIX_RESULT MixVideoRenderParams::get_clipping_rects(MixRect ** clipping_rects, 
	guint* number_of_clipping_rects) {
	if (NULL == clipping_rects || NULL == number_of_clipping_rects)
		return MIX_RESULT_NULL_PTR;

	*clipping_rects = NULL;
	*number_of_clipping_rects = 0;

	if ((NULL != this->clipping_rects) && (0 != this->number_of_clipping_rects)) {
		*clipping_rects = reinterpret_cast<MixRect*>(g_memdup(this->clipping_rects, 
			this->number_of_clipping_rects * sizeof(MixRect)));
		if (NULL == *clipping_rects) {
			return MIX_RESULT_NO_MEMORY;
		}
		*number_of_clipping_rects = this->number_of_clipping_rects;
	}
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT MixVideoRenderParams::get_va_cliprects(VARectangle ** va_cliprects, 
	guint* number_of_cliprects) {
	if (NULL == va_cliprects || NULL == number_of_cliprects)
		return MIX_RESULT_NULL_PTR;

	*va_cliprects = NULL;
	*number_of_cliprects = 0;

	if ((NULL != mVa_cliprects) && (0 != number_of_clipping_rects)) {
		*va_cliprects = mVa_cliprects;
		*number_of_cliprects = number_of_clipping_rects;
	}
	return MIX_RESULT_SUCCESS;
}

MixVideoRenderParams *
mix_videorenderparams_new(void) {
	return new MixVideoRenderParams();
}

MixVideoRenderParams *
mix_videorenderparams_ref(MixVideoRenderParams * mix) {
	if (NULL != mix)
		mix->Ref();
	return mix;
}

gboolean mix_rect_equal(MixRect rc1, MixRect rc2) {
	if (rc1.x == rc2.x && rc1.y == rc2.y && rc1.width == rc2.width
			&& rc1.height == rc2.height) {
		return TRUE;
	}
	return FALSE;
}


/* TODO: Add getters and setters for other properties. The following is just an exmaple, not implemented yet. */

MIX_RESULT mix_videorenderparams_set_display(
	MixVideoRenderParams * obj, MixDisplay * display) {
	MIX_VIDEORENDERPARAMS_SETTER_CHECK_INPUT (obj);
	if (obj->display) {
		mix_display_unref(obj->display);
		obj->display = NULL;
	}
	/* dup */
	if (display) {
		obj->display = mix_display_ref(display);
	}
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videorenderparams_get_display(
	MixVideoRenderParams * obj, MixDisplay ** display) {
	MIX_VIDEORENDERPARAMS_GETTER_CHECK_INPUT (obj, display);
	/* dup? */
	if (obj->display) {
		*display = mix_display_ref(obj->display);
	}
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videorenderparams_set_src_rect(
	MixVideoRenderParams * obj, MixRect src_rect) {
	MIX_VIDEORENDERPARAMS_SETTER_CHECK_INPUT (obj);
	obj->src_rect = src_rect;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videorenderparams_get_src_rect(
	MixVideoRenderParams * obj, MixRect * src_rect) {
	MIX_VIDEORENDERPARAMS_GETTER_CHECK_INPUT (obj, src_rect);
	*src_rect = obj->src_rect;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videorenderparams_set_dest_rect(
	MixVideoRenderParams * obj, MixRect dst_rect) {
	MIX_VIDEORENDERPARAMS_SETTER_CHECK_INPUT (obj);
	obj->dst_rect = dst_rect;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videorenderparams_get_dest_rect(
	MixVideoRenderParams * obj, MixRect * dst_rect) {
	MIX_VIDEORENDERPARAMS_GETTER_CHECK_INPUT (obj, dst_rect);
	*dst_rect = obj->dst_rect;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videorenderparams_set_clipping_rects(
	MixVideoRenderParams * obj, MixRect* clipping_rects, 
	guint number_of_clipping_rects) {
	if (NULL == obj)
		return MIX_RESULT_NULL_PTR;
	return obj->set_clipping_rects(clipping_rects, number_of_clipping_rects);
}

MIX_RESULT mix_videorenderparams_get_clipping_rects(
	MixVideoRenderParams * obj, MixRect ** clipping_rects,
	guint* number_of_clipping_rects) {
	if (NULL == obj)
		return MIX_RESULT_NULL_PTR;
	return obj->get_clipping_rects(clipping_rects, number_of_clipping_rects);
}

/* The mixvideo internal method */
MIX_RESULT mix_videorenderparams_get_cliprects_internal(
		MixVideoRenderParams * obj, VARectangle ** va_cliprects,
		guint* number_of_cliprects) {
	if (NULL == obj)
		return MIX_RESULT_NULL_PTR;
	return obj->get_va_cliprects(va_cliprects, number_of_cliprects);;
}

/* TODO: implement properties' setters and getters */
