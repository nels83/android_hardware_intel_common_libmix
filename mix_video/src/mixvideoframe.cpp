/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

/**
 * SECTION:mixvideoframe
 * @short_description: MI-X Video Frame Object 
 * 
 * <para>
 * The MixVideoFrame object will be created by 
 * MixVideo and provided to the MMF/App in the 
 * MixVideo mix_video_get_frame() function.
 * </para>
 * <para> 
 * mix_video_release_frame() must be used 
 * to release frame object returned from 
 * mix_video_get_frame(). Caller must not 
 * use mix_videoframe_ref() or mix_videoframe_unref() 
 * or adjust the reference count directly in any way. 
 * This object can be supplied in the mix_video_render() 
 * function to render the associated video frame. 
 * The MMF/App can release this object when it no longer 
 * needs to display/re-display this frame.
 * </para> 
 */


#include <va/va.h>
#ifndef ANDROID
#include <va/va_x11.h>
#endif
#include "mixvideolog.h"
//#include "mixvideoframe_private.h"
#include "mixsurfacepool.h"
#include "mixvideoframe.h"

#define SAFE_FREE(p) if(p) { g_free(p); p = NULL; }

#define MIX_VIDEOFRAME_SETTER_CHECK_INPUT(obj) \
	if(!obj) return MIX_RESULT_NULL_PTR; \
	if(!MIX_IS_VIDEOFRAME(obj)) return MIX_RESULT_FAIL; \

#define MIX_VIDEOFRAME_GETTER_CHECK_INPUT(obj, prop) \
	if(!obj || !prop) return MIX_RESULT_NULL_PTR; \
	if(!MIX_IS_VIDEOFRAME(obj)) return MIX_RESULT_FAIL; \


MixVideoFrame::MixVideoFrame()
	:frame_id(VA_INVALID_SURFACE)
	,timestamp(0)
	,discontinuity(FALSE)
	,reserved1(NULL)
	,reserved2(NULL)
	,reserved3(NULL)
	,reserved4(NULL)
	,pool(NULL)
	,is_skipped(FALSE)
	,real_frame(NULL)
	,sync_flag(FALSE)
	,frame_structure(VA_FRAME_PICTURE)
	,va_display(NULL){
	g_static_rec_mutex_init (&lock);
}

MixVideoFrame::~MixVideoFrame() {
	g_static_rec_mutex_free (&lock);
}

gboolean MixVideoFrame::copy(MixParams *target) const {
	gboolean ret = FALSE;
	MixVideoFrame * this_target = MIX_VIDEOFRAME(target);
	if (NULL != this_target) {
		this_target->frame_id = this->frame_id;
		this_target->timestamp = this->timestamp;
		this_target->discontinuity = this->discontinuity;
		// chain up base class
		ret = MixParams::copy(target);
	}
	return ret;
}

gboolean MixVideoFrame::equal(MixParams* obj) const {
	gboolean ret = FALSE;
	MixVideoFrame * this_obj = MIX_VIDEOFRAME(obj);
	if (NULL != this_obj) {
		/* TODO: add comparison for other properties */
		if (this->frame_id == this_obj->frame_id &&
			this->timestamp == this_obj->timestamp &&
			this->discontinuity == this_obj->discontinuity) {
			ret = MixParams::equal(this_obj);
		}
	}
	return ret;
}

MixParams* MixVideoFrame::dup() const {
	MixParams *ret = new MixVideoFrame();
	if (NULL != ret) {
		if (FALSE == copy(ret)) {
			ret->Unref();
			ret = NULL;
		}
	}
	return ret;
}

void MixVideoFrame::Lock() {
	g_static_rec_mutex_lock(&lock);
}
void MixVideoFrame::Unlock() {
	g_static_rec_mutex_unlock (&lock);
}

MixVideoFrame * mix_videoframe_new(void) {
	return new MixVideoFrame();
}



MixVideoFrame * mix_videoframe_ref(MixVideoFrame * obj) {
	if (NULL != obj) {	
		obj->Lock();
		LOG_I("obj %x, new refcount is %d\n", (guint) obj,
			obj->GetRefCount() + 1);
		obj->Ref();
		obj->Unlock();
	}
	return obj;
}

void mix_videoframe_unref(MixVideoFrame * obj) {
	if(NULL == obj) {
		LOG_E("obj is NULL\n");
		return;
	}

	obj->Lock();
	LOG_I("obj %x, frame id %d, new refcount is %d\n", (guint) obj,
			(guint) obj->frame_id, obj->GetRefCount() - 1);

	// Check if we have reduced to 1, in which case we add ourselves to free pool
	// but only do this for real frames, not skipped frames
	if (((obj->GetRefCount() - 1) == 1) && (!(obj->is_skipped))) {
		LOG_I("Adding obj %x, frame id %d back to pool\n", (guint) obj,
			(guint) obj->frame_id);
		MixSurfacePool *pool = obj->pool;
		if(pool == NULL) {
			LOG_E("pool is NULL\n");
			obj->Unlock();
			return;
		}
		mix_videoframe_reset(obj);
		mix_surfacepool_put(pool, obj);
	}

	//If this is a skipped frame that is being deleted, release the real frame
	if (((obj->GetRefCount() - 1) == 0) && (obj->is_skipped)) {
		LOG_I("skipped frame obj %x, releasing real frame %x \n",
				(guint) obj, (guint) obj->real_frame);
		mix_videoframe_unref(obj->real_frame);
	}

	// Unref through base class
	obj->Unref();
	obj->Unlock();
}


/* TODO: Add getters and setters for other properties. The following is just an exmaple, not implemented yet. */
MIX_RESULT mix_videoframe_set_frame_id(
	MixVideoFrame * obj, gulong frame_id) {
	MIX_VIDEOFRAME_SETTER_CHECK_INPUT (obj);
	obj->frame_id = frame_id;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoframe_get_frame_id(
	MixVideoFrame * obj, gulong * frame_id) {
	MIX_VIDEOFRAME_GETTER_CHECK_INPUT (obj, frame_id);
	*frame_id = obj->frame_id;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoframe_set_ci_frame_idx (
	MixVideoFrame * obj, guint ci_frame_idx) {
	MIX_VIDEOFRAME_SETTER_CHECK_INPUT (obj);
	obj->ci_frame_idx = ci_frame_idx;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoframe_get_ci_frame_idx (
	MixVideoFrame * obj, guint * ci_frame_idx) {
	MIX_VIDEOFRAME_GETTER_CHECK_INPUT (obj, ci_frame_idx);
	*ci_frame_idx = obj->ci_frame_idx;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoframe_set_timestamp(
	MixVideoFrame * obj, guint64 timestamp) {
	MIX_VIDEOFRAME_SETTER_CHECK_INPUT (obj);
	obj->timestamp = timestamp;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoframe_get_timestamp(
	MixVideoFrame * obj, guint64 * timestamp) {
	MIX_VIDEOFRAME_GETTER_CHECK_INPUT (obj, timestamp);
	*timestamp = obj->timestamp;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoframe_set_discontinuity(
	MixVideoFrame * obj, gboolean discontinuity) {
	MIX_VIDEOFRAME_SETTER_CHECK_INPUT (obj);
	obj->discontinuity = discontinuity;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoframe_get_discontinuity(
	MixVideoFrame * obj, gboolean * discontinuity) {
	MIX_VIDEOFRAME_GETTER_CHECK_INPUT (obj, discontinuity);
	*discontinuity = obj->discontinuity;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoframe_set_frame_structure(
	MixVideoFrame * obj, guint32 frame_structure) {
	MIX_VIDEOFRAME_SETTER_CHECK_INPUT (obj);	
	obj->frame_structure = frame_structure;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoframe_get_frame_structure(
	MixVideoFrame * obj, guint32* frame_structure) {
	MIX_VIDEOFRAME_GETTER_CHECK_INPUT (obj, frame_structure);
	*frame_structure = obj->frame_structure;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoframe_set_pool(
	MixVideoFrame * obj, MixSurfacePool * pool) {
	/* set pool pointer in private structure */
	obj->pool = pool;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoframe_set_frame_type(
	MixVideoFrame *obj, MixFrameType frame_type) {
	obj->frame_type = frame_type;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoframe_get_frame_type(
	MixVideoFrame *obj, MixFrameType *frame_type) {
	MIX_VIDEOFRAME_GETTER_CHECK_INPUT(obj, frame_type);
	*frame_type = obj->frame_type;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoframe_set_is_skipped(
	MixVideoFrame *obj, gboolean is_skipped) {
	MIX_VIDEOFRAME_SETTER_CHECK_INPUT (obj);
	obj->is_skipped = is_skipped;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoframe_get_is_skipped(
	MixVideoFrame *obj, gboolean *is_skipped) {
	MIX_VIDEOFRAME_GETTER_CHECK_INPUT(obj, is_skipped);
	*is_skipped = obj->is_skipped;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoframe_set_real_frame(
	MixVideoFrame *obj, MixVideoFrame *real) {
	MIX_VIDEOFRAME_SETTER_CHECK_INPUT (obj);
	obj->real_frame = real;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoframe_get_real_frame(
	MixVideoFrame *obj, MixVideoFrame **real) {
	MIX_VIDEOFRAME_GETTER_CHECK_INPUT(obj, real);
	*real = obj->real_frame;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoframe_reset(MixVideoFrame *obj) {
	MIX_VIDEOFRAME_SETTER_CHECK_INPUT (obj);
	obj->timestamp = 0;
	obj->discontinuity = FALSE;
	obj->is_skipped = FALSE;
	obj->real_frame = NULL;
	obj->sync_flag  = FALSE;
	obj->frame_structure = VA_FRAME_PICTURE;
	return MIX_RESULT_SUCCESS;
}


MIX_RESULT mix_videoframe_set_sync_flag(
	MixVideoFrame *obj, gboolean sync_flag) {
	MIX_VIDEOFRAME_SETTER_CHECK_INPUT (obj);
	obj->sync_flag = sync_flag;
	if (obj->real_frame && obj->real_frame != obj) {
		mix_videoframe_set_sync_flag(obj->real_frame, sync_flag);
	}
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoframe_get_sync_flag(
	MixVideoFrame *obj, gboolean *sync_flag) {
	MIX_VIDEOFRAME_GETTER_CHECK_INPUT(obj, sync_flag);
	if (obj->real_frame && obj->real_frame != obj) {
		return mix_videoframe_get_sync_flag(obj->real_frame, sync_flag);
	} else {
		*sync_flag = obj -> sync_flag;
	}
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoframe_set_vadisplay(
	MixVideoFrame * obj, void *va_display) {
	MIX_VIDEOFRAME_SETTER_CHECK_INPUT (obj);
	obj->va_display = va_display;
	if (obj->real_frame && obj->real_frame != obj) {
		mix_videoframe_set_vadisplay(obj->real_frame, va_display);
	}
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoframe_get_vadisplay(
	MixVideoFrame * obj, void **va_display) {
	MIX_VIDEOFRAME_GETTER_CHECK_INPUT(obj, va_display);
	if (obj->real_frame && obj->real_frame != obj) {
		return mix_videoframe_get_vadisplay(obj->real_frame, va_display);
	} else {
		*va_display = obj->va_display;
	}
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoframe_set_displayorder(
	MixVideoFrame *obj, guint32 displayorder) {
	MIX_VIDEOFRAME_SETTER_CHECK_INPUT (obj);
	obj->displayorder = displayorder;
	return MIX_RESULT_SUCCESS;
}


MIX_RESULT mix_videoframe_get_displayorder(
	MixVideoFrame *obj, guint32 *displayorder) {
	MIX_VIDEOFRAME_GETTER_CHECK_INPUT(obj, displayorder);
	*displayorder = obj->displayorder;
	return MIX_RESULT_SUCCESS;
}



