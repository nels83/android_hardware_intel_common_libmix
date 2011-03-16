/*
INTEL CONFIDENTIAL
Copyright 2009 Intel Corporation All Rights Reserved.
The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
*/

/**
* SECTION:mixdisplay
* @short_description: Lightweight Base Object for MI-X Video Display
*
*/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mixdisplay.h"

#define DEBUG_REFCOUNT

MixDisplay::MixDisplay()
        :refcount(1) {
}
MixDisplay::~MixDisplay() {
    Finalize();
}

MixDisplay* MixDisplay::Dup() const {
    MixDisplay* dup = new MixDisplay();
    if (NULL != dup ) {
        if (FALSE == Copy(dup)) {
            dup->Unref();
            dup = NULL;
        }
    }
    return dup;
}

bool MixDisplay::Copy(MixDisplay* target) const {
    if (NULL != target)
        return TRUE;
    else
        return FALSE;
}

void MixDisplay::Finalize() {
}

bool MixDisplay::Equal(const MixDisplay* obj) const {
    if (NULL != obj)
        return TRUE;
    else
        return FALSE;
}

MixDisplay * MixDisplay::Ref() {
    ++refcount;
    return this;
}
void MixDisplay::Unref () {
    if (0 == (--refcount)) {
        delete this;
    }
}

bool mix_display_copy (MixDisplay * target, const MixDisplay * src) {
    if (target == src)
        return TRUE;
    if (NULL == target || NULL == src)
        return FALSE;
    return src->Copy(target);
}


MixDisplay * mix_display_dup (const MixDisplay * obj) {
    if (NULL == obj)
        return NULL;
    return obj->Dup();
}



MixDisplay * mix_display_new (void) {
    return new MixDisplay();
}

MixDisplay * mix_display_ref (MixDisplay * obj) {
    if (NULL != obj)
        obj->Ref();
    return obj;
}

void mix_display_unref (MixDisplay * obj) {
    if (NULL != obj)
        obj->Unref();
}


/**
* mix_display_replace:
* @olddata: pointer to a pointer to a object to be replaced
* @newdata: pointer to new object
*
* Modifies a pointer to point to a new object.  The modification
* is done atomically, and the reference counts are updated correctly.
* Either @newdata and the value pointed to by @olddata may be NULL.
*/
void mix_display_replace (MixDisplay ** olddata, MixDisplay * newdata) {
    if (NULL == olddata)
        return;
    if (*olddata == newdata)
        return;
    MixDisplay *olddata_val = *olddata;
    if (NULL != newdata)
        newdata->Ref();
    *olddata = newdata;
    if (NULL != olddata_val)
        olddata_val->Unref();
}

bool mix_display_equal (MixDisplay * first, MixDisplay * second) {
    if (first == second)
        return TRUE;
    if (NULL == first || NULL == second)
        return FALSE;
    return first->Equal(second);
}


