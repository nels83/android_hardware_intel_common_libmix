/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

/**
 * SECTION:mixvideoconfigparams
 * @short_description: MI-X Video Configuration Parameter Base Object
 * @include: mixvideoconfigparams.h
 *
 * <para>
 * A base object of MI-X video configuration parameter objects.
 * </para> 
 * <para>
 * The derived MixVideoConfigParams object is created by the MMF/App
 * and provided in the MixVideo mix_video_configure() function. The get and set
 * methods for the properties will be available for the caller to set and get information at
 * configuration time. It will also be created by MixVideo and returned from the
 * mix_video_get_config() function, whereupon the MMF/App can get the get methods to
 * obtain current configuration information.
 * </para> 
 * <para>
 * There are decode mode objects (for example, MixVideoConfigParamsDec) and encode
 * mode objects (for example, MixVideoConfigParamsEnc). Each of these types is refined
 * further with media specific objects. The application should create the correct type of
 * object to match the media format of the stream to be handled, e.g. if the media
 * format of the stream to be decoded is H.264, the application would create a
 * MixVideoConfigParamsDecH264 object for the mix_video_configure() call.
 * </para> 
 */

#include <string.h>
#include "mixvideolog.h"
#include "mixvideoconfigparams.h"

static GType _mix_videoconfigparams_type = 0;
static MixParamsClass *parent_class = NULL;

#define _do_init { _mix_videoconfigparams_type = g_define_type_id; }

gboolean mix_videoconfigparams_copy(MixParams * target, const MixParams * src);
MixParams *mix_videoconfigparams_dup(const MixParams * obj);
gboolean mix_videoconfigparams_equal(MixParams * first, MixParams * second);
static void mix_videoconfigparams_finalize(MixParams * obj);

G_DEFINE_TYPE_WITH_CODE (MixVideoConfigParams, mix_videoconfigparams,
		MIX_TYPE_PARAMS, _do_init);

static void mix_videoconfigparams_init(MixVideoConfigParams * self) {

	/* initialize properties here */
	self->reserved1 = NULL;
	self->reserved2 = NULL;
	self->reserved3 = NULL;
	self->reserved4 = NULL;
}

static void mix_videoconfigparams_class_init(MixVideoConfigParamsClass * klass) {
	MixParamsClass *mixparams_class = MIX_PARAMS_CLASS(klass);

	/* setup static parent class */
	parent_class = (MixParamsClass *) g_type_class_peek_parent(klass);

	mixparams_class->finalize = mix_videoconfigparams_finalize;
	mixparams_class->copy = (MixParamsCopyFunction) mix_videoconfigparams_copy;
	mixparams_class->dup = (MixParamsDupFunction) mix_videoconfigparams_dup;
	mixparams_class->equal
			= (MixParamsEqualFunction) mix_videoconfigparams_equal;
}

MixVideoConfigParams *
mix_videoconfigparams_new(void) {
	MixVideoConfigParams *ret =
			(MixVideoConfigParams *) g_type_create_instance(
					MIX_TYPE_VIDEOCONFIGPARAMS);

	return ret;
}

void mix_videoconfigparams_finalize(MixParams * obj) {

	/* clean up here. */
	/* MixVideoConfigParams *self = MIX_VIDEOCONFIGPARAMS(obj); */

	/* Chain up parent */
	if (parent_class->finalize) {
		parent_class->finalize(obj);
	}
}

MixVideoConfigParams *
mix_videoconfigparams_ref(MixVideoConfigParams * mix) {
	return (MixVideoConfigParams *) mix_params_ref(MIX_PARAMS(mix));
}

/**
 * mix_videoconfigparams_dup:
 * @obj: a #MixVideoConfigParams object
 * @returns: a newly allocated duplicate of the object.
 *
 * Copy duplicate of the object.
 */
MixParams *
mix_videoconfigparams_dup(const MixParams * obj) {
	MixParams *ret = NULL;

	if (MIX_IS_VIDEOCONFIGPARAMS(obj)) {
		MixVideoConfigParams *duplicate = mix_videoconfigparams_new();
		if (mix_videoconfigparams_copy(MIX_PARAMS(duplicate), MIX_PARAMS(obj))) {
			ret = MIX_PARAMS(duplicate);
		} else {
			mix_videoconfigparams_unref(duplicate);
		}
	}

	return ret;
}

/**
 * mix_videoconfigparams_copy:
 * @target: copy to target
 * @src: copy from src
 * @returns: boolean indicates if copy is successful.
 *
 * Copy instance data from @src to @target.
 */
gboolean mix_videoconfigparams_copy(MixParams * target, const MixParams * src) {

	LOG_V( "Begin\n");

	if (MIX_IS_VIDEOCONFIGPARAMS(target) && MIX_IS_VIDEOCONFIGPARAMS(src)) {

		/* TODO: copy other properties if there's any */

		/* Now chainup base class */
		if (parent_class->copy) {
			LOG_V( "parent_class->copy != NULL\n");
			return parent_class->copy(MIX_PARAMS_CAST(target), MIX_PARAMS_CAST(
					src));
		} else {
			LOG_V( "parent_class->copy == NULL\n");
			return TRUE;
		}
	}

	LOG_V( "End\n");
	return FALSE;
}

/**
 * mix_videoconfigparams_:
 * @first: first object to compare
 * @second: seond object to compare
 * @returns: boolean indicates if instance are equal.
 *
 * Copy instance data from @src to @target.
 */
gboolean mix_videoconfigparams_equal(MixParams * first, MixParams * second) {

	gboolean ret = FALSE;

	if (MIX_IS_VIDEOCONFIGPARAMS(first) && MIX_IS_VIDEOCONFIGPARAMS(second)) {

		/* chaining up. */
		MixParamsClass *klass = MIX_PARAMS_CLASS(parent_class);
		if (klass->equal)
			ret = parent_class->equal(first, second);
		else
			ret = TRUE;
	}

	return ret;
}
