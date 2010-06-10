/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

/**
 * SECTION:mixdisplayandroid
 * @short_description: MI-X Video Android Display 
 *
 * A data object which stores Android specific parameters.
 * 
 * <note>
 * <title>Data Structures Used in MixDisplayAndroid Fields:</title>
 * </note>
 */

#ifdef ANDROID

#include "mixdisplayandroid.h"

#define SAFE_FREE(p) if(p) { g_free(p); p = NULL; }

static GType _mix_displayandroid_type = 0;
static MixDisplayClass *parent_class = NULL;

#define _do_init { _mix_displayandroid_type = g_define_type_id; }

gboolean mix_displayandroid_copy(MixDisplay * target, const MixDisplay * src);
MixDisplay *mix_displayandroid_dup(const MixDisplay * obj);
gboolean mix_displayandroid_equal(MixDisplay * first, MixDisplay * second);
static void mix_displayandroid_finalize(MixDisplay * obj);

G_DEFINE_TYPE_WITH_CODE (MixDisplayAndroid, mix_displayandroid,
		MIX_TYPE_DISPLAY, _do_init);

static void mix_displayandroid_init(MixDisplayAndroid * self) {

	/* Initialize member varibles */
	self->display = NULL;
//	self->drawable = 0;
}

static void mix_displayandroid_class_init(MixDisplayAndroidClass * klass) {
	MixDisplayClass *mixdisplay_class = MIX_DISPLAY_CLASS(klass);

	/* setup static parent class */
	parent_class = (MixDisplayClass *) g_type_class_peek_parent(klass);

	mixdisplay_class->finalize = mix_displayandroid_finalize;
	mixdisplay_class->copy = (MixDisplayCopyFunction) mix_displayandroid_copy;
	mixdisplay_class->dup = (MixDisplayDupFunction) mix_displayandroid_dup;
	mixdisplay_class->equal = (MixDisplayEqualFunction) mix_displayandroid_equal;
}

MixDisplayAndroid *
mix_displayandroid_new(void) {
	MixDisplayAndroid *ret = (MixDisplayAndroid *) g_type_create_instance(
			MIX_TYPE_DISPLAYANDROID);

	return ret;
}

void mix_displayandroid_finalize(MixDisplay * obj) {
	/* clean up here. */
	/* MixDisplayAndroid *self = MIX_DISPLAYANDROID (obj); */

	/* NOTE: we don't need to do anything
	 * with display and drawable */

	/* Chain up parent */
	if (parent_class->finalize)
		parent_class->finalize(obj);
}

MixDisplayAndroid *
mix_displayandroid_ref(MixDisplayAndroid * mix) {
	return (MixDisplayAndroid *) mix_display_ref(MIX_DISPLAY(mix));
}

/**
 * mix_mixdisplayandroid_dup:
 * @obj: a #MixDisplayAndroid object
 * @returns: a newly allocated duplicate of the object.
 *
 * Copy duplicate of the object.
 */
MixDisplay *
mix_displayandroid_dup(const MixDisplay * obj) {
	MixDisplay *ret = NULL;

	if (MIX_IS_DISPLAYANDROID(obj)) {
		MixDisplayAndroid *duplicate = mix_displayandroid_new();
		if (mix_displayandroid_copy(MIX_DISPLAY(duplicate), MIX_DISPLAY(obj))) {
			ret = MIX_DISPLAY(duplicate);
		} else {
			mix_displayandroid_unref(duplicate);
		}
	}
	return ret;
}

/**
 * mix_mixdisplayandroid_copy:
 * @target: copy to target
 * @src: copy from src
 * @returns: boolean indicates if copy is successful.
 *
 * Copy instance data from @src to @target.
 */
gboolean mix_displayandroid_copy(MixDisplay * target, const MixDisplay * src) {
	MixDisplayAndroid *this_target, *this_src;

	if (MIX_IS_DISPLAYANDROID(target) && MIX_IS_DISPLAYANDROID(src)) {
		// Cast the base object to this child object
		this_target = MIX_DISPLAYANDROID(target);
		this_src = MIX_DISPLAYANDROID(src);

		// Copy properties from source to target.

		this_target->display = this_src->display;
//		this_target->drawable = this_src->drawable;

		// Now chainup base class
		if (parent_class->copy) {
			return parent_class->copy(MIX_DISPLAY_CAST(target),
					MIX_DISPLAY_CAST(src));
		} else {
			return TRUE;
		}
	}
	return FALSE;
}

/**
 * mix_mixdisplayandroid_equal:
 * @first: first object to compare
 * @second: seond object to compare
 * @returns: boolean indicates if instance are equal.
 *
 * Copy instance data from @src to @target.
 */
gboolean mix_displayandroid_equal(MixDisplay * first, MixDisplay * second) {
	gboolean ret = FALSE;

	MixDisplayAndroid *this_first, *this_second;

	this_first = MIX_DISPLAYANDROID(first);
	this_second = MIX_DISPLAYANDROID(second);

	if (MIX_IS_DISPLAYANDROID(first) && MIX_IS_DISPLAYANDROID(second)) {
		// Compare member variables

		// TODO: if in the copy method we just copy the pointer of display, the comparison
		//      below is enough. But we need to decide how to copy!

		if (this_first->display == this_second->display /*&& this_first->drawable
				== this_second->drawable*/) {
			// members within this scope equal. chaining up.
			MixDisplayClass *klass = MIX_DISPLAY_CLASS(parent_class);
			if (klass->equal)
				ret = parent_class->equal(first, second);
			else
				ret = TRUE;
		}
	}
	return ret;
}

#define MIX_DISPLAYANDROID_SETTER_CHECK_INPUT(obj) \
	if(!obj) return MIX_RESULT_NULL_PTR; \
	if(!MIX_IS_DISPLAYANDROID(obj)) return MIX_RESULT_FAIL; \

#define MIX_DISPLAYANDROID_GETTER_CHECK_INPUT(obj, prop) \
	if(!obj || prop == NULL) return MIX_RESULT_NULL_PTR; \
	if(!MIX_IS_DISPLAYANDROID(obj)) return MIX_RESULT_FAIL; \

MIX_RESULT mix_displayandroid_set_display(MixDisplayAndroid * obj, void * display) {
	MIX_DISPLAYANDROID_SETTER_CHECK_INPUT (obj);

	// TODO: needs to decide to clone or just copy pointer
	obj->display = display;
	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_displayandroid_get_display(MixDisplayAndroid * obj, void ** display) {
	MIX_DISPLAYANDROID_GETTER_CHECK_INPUT (obj, display);

	// TODO: needs to decide to clone or just copy pointer
	*display = obj->display;

	return MIX_RESULT_SUCCESS;
}

#endif /* ANDROID */
