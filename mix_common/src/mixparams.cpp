/* 
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved. 
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
*/

/**
 * SECTION:mixparams
 * @short_description: Lightweight base class for the MIX media params
 *
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mixparams.h"


#define DEBUG_REFCOUNT

MixParams::MixParams()
	:ref_count(1)
	,_reserved(NULL) {
}

MixParams::~MixParams() {
	finalize();
}

MixParams* MixParams::Ref() {
	this->ref_count++;
	return this;
}

void MixParams::Unref() {
	this->ref_count--;
	if (0 == this->ref_count) {
		delete this;
	}
}

MixParams* MixParams::dup() const {
	MixParams *ret = new MixParams();
	if (FALSE != copy(ret)) {
		return ret;
	}
	return NULL;
}

gboolean MixParams::copy(MixParams* target) const {
	gboolean ret =  FALSE;
	if ( NULL != target) {
		return TRUE;
	}
	return ret;
}

void MixParams::finalize() {
}

gboolean MixParams::equal(MixParams *obj) const {
	gboolean ret =  FALSE;
	if ( NULL != obj) {
		return TRUE;
	}
	return ret;
}

MixParams* mix_params_new () {
	/* we don't support dynamic types because they really aren't useful,*/
	/* and could cause ref_count problems */
	return new MixParams();
}

gboolean mix_params_copy (MixParams *target, const MixParams *src) {
	if ( NULL != target && NULL != src) {
		return src->copy(target);
	} else
		return FALSE;
}

MixParams* mix_params_ref (MixParams *obj) {
	if (NULL == obj)
		return NULL;
	return obj->Ref();
}

void mix_params_unref(MixParams *obj) {
	if (NULL != obj)
		obj->Unref();
}

void mix_params_replace (MixParams **olddata, MixParams *newdata) {
	if (NULL == olddata)
		return;
	MixParams *olddata_val =
		reinterpret_cast<MixParams*>(g_atomic_pointer_get((gpointer *) olddata));
	if (olddata_val == newdata)
		return;
	if (NULL != newdata)
		newdata->Ref();
	while (!g_atomic_pointer_compare_and_exchange ((gpointer *) olddata,
		olddata_val, newdata)) {
		olddata_val =
			reinterpret_cast<MixParams*>(g_atomic_pointer_get ((gpointer *) olddata));
	}
	if (NULL != olddata_val)
		olddata_val->Unref();
}

MixParams * mix_params_dup(const MixParams *obj) {
	if (NULL != obj) {
		return obj->dup();
	} else {
		return NULL;
	}
}

gboolean mix_params_equal (MixParams *first, MixParams *second) {
	if (NULL != first && NULL != second)
		return first->equal(second);
	else
		return FALSE;
}

