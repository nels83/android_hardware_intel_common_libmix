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

MixDisplayAndroid::MixDisplayAndroid()
	:display(NULL) {
}

MixDisplayAndroid::~MixDisplayAndroid() {
	Finalize();
}

MixDisplay* MixDisplayAndroid::Dup() const {
	MixDisplayAndroid* dup = new MixDisplayAndroid();
	if (NULL != dup ) {
		if(FALSE == Copy(dup)) {
			dup->Unref();
			dup = NULL;
		}
	}
	return dup;

}

gboolean MixDisplayAndroid::Copy(MixDisplay* target) const {
	gboolean ret = FALSE;
	MixDisplayAndroid* this_target = reinterpret_cast<MixDisplayAndroid*>(target);
	if (NULL != this_target) {
		this_target->display = this->display;
		ret = MixDisplay::Copy(target);
	}
	return ret;
}

void MixDisplayAndroid::Finalize() {
	MixDisplay::Finalize();
}

gboolean MixDisplayAndroid::Equal(const MixDisplay* obj) const {
	gboolean ret = FALSE;
	const MixDisplayAndroid* this_obj = reinterpret_cast<const MixDisplayAndroid*>(obj);
	if (NULL != this_obj) {
		if(this_obj->display == this->display)
			ret = MixDisplay::Equal(obj);
	}
	return ret;
}


MixDisplayAndroid * mix_displayandroid_new(void) {
	return new MixDisplayAndroid();
}



MixDisplayAndroid * mix_displayandroid_ref(MixDisplayAndroid * mix) {
	if (NULL != mix)
		mix->Ref();
	return mix;
}

/**
 * mix_mixdisplayandroid_dup:
 * @obj: a #MixDisplayAndroid object
 * @returns: a newly allocated duplicate of the object.
 *
 * Copy duplicate of the object.
 */
MixDisplay * mix_displayandroid_dup(const MixDisplay * obj) {
	MixDisplay *ret = NULL;
	if (NULL != obj) {
		ret = obj->Dup();
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
	if (target == src)
		return TRUE;
	if (NULL == target || NULL == src)
		return FALSE;
	const MixDisplayAndroid *this_src = 
		reinterpret_cast<const MixDisplayAndroid *>(src);
	MixDisplayAndroid *this_target = 
		reinterpret_cast<MixDisplayAndroid *>(target);
	return this_src->Copy(this_target);
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
	if (first == second)
		return TRUE;
	if (NULL == first || NULL == second)
		return FALSE;
	MixDisplayAndroid *this_first = 
		reinterpret_cast<MixDisplayAndroid *>(first);
	MixDisplayAndroid *this_second = 
		reinterpret_cast<MixDisplayAndroid *>(second);
	return first->Equal(second);
}

#define MIX_DISPLAYANDROID_SETTER_CHECK_INPUT(obj) \
	if(!obj) return MIX_RESULT_NULL_PTR;

#define MIX_DISPLAYANDROID_GETTER_CHECK_INPUT(obj, prop) \
	if(!obj || prop == NULL) return MIX_RESULT_NULL_PTR;

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
