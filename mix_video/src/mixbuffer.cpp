/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

/**
 * SECTION:mixbuffer
 * @short_description: MI-X Video Buffer Parameters
 *
 *<para>
 * #MixBuffer objects are used to wrap input data buffers in a reference counted object as
 * described in the buffer model section. Data buffers themselves are allocated by the
 * App/MMF. #MixBuffer objects are allocated by #MixVideo in a pool and retrieved by the
 * application using mix_video_get_mixbuffer(). The application will wrap a data buffer
 * in a #Mixbuffer object and pass it into mix_video_decode() or to mix_video_encode().
 * </para>
 * <para>
 * The #MixBuffer objects will be released by #MixVideo when they are no longer needed
 * for the decode or encoder operation. The App/MMF will also release the #MixBuffer
 * object after use. When the #MixBuffer is completely released, the callback to the
 * function registered in the #MixBuffer will be called (allowing the App/MMF to release
 * data buffers as necessary).
 * </para>
 */

#include <assert.h>

#include "mixvideolog.h"
#include "mixbufferpool.h"
#include "mixbuffer.h"
#include "mixbuffer_private.h"

#define SAFE_FREE(p) if(p) { free(p); p = NULL; }

MixBuffer::MixBuffer()
        :data(NULL)
        ,size(0)
        ,token(0)
        ,callback(NULL)
        ,pool(NULL) {
}

MixBuffer::~MixBuffer() {
}

/**
 * mix_buffer_dup:
 * @obj: a #MixBuffer object
 * @returns: a newly allocated duplicate of the object.
 *
 * Copy duplicate of the object.
 */
MixParams * MixBuffer::dup() const {
    MixParams *ret = new MixBuffer();
    if (NULL != ret) {
        if (FALSE == copy(ret)) {
            ret->Unref();
            ret = NULL;
        }
    }
    return ret;
}

/**
 * @target: copy to target
 * @src: copy from src
 * @returns: boolean indicates if copy is successful.
 *
 * Copy instance data from @src to @target.
 */
bool MixBuffer::copy(MixParams * target) const {
    bool ret = FALSE;
    MixBuffer * this_target = MIX_BUFFER(target);
    if (NULL != this_target) {
        this_target->data = data;
        this_target->size = size;
        this_target->token = token;
        this_target->callback = callback;
        ret = MixParams::copy(target);
    }
    return ret;
}

bool MixBuffer::equal(MixParams * obj) const {
    bool ret = FALSE;
    MixBuffer * this_obj = MIX_BUFFER(obj);
    if (NULL != this_obj) {
        if (this_obj->data == data &&
                this_obj->size == size &&
                this_obj->token == token &&
                this_obj->callback == callback) {
            ret = MixParams::equal(this_obj);
        }
    }
    return ret;
}

MixBuffer * mix_buffer_new(void) {
    return new MixBuffer();
}

MixBuffer * mix_buffer_ref(MixBuffer * mix) {
    if (NULL != mix)
        mix->Ref();
    return mix;
}



MIX_RESULT mix_buffer_set_data(
    MixBuffer * obj, uchar *data, uint size,
    ulong token, MixBufferCallback callback) {
    obj->data = data;
    obj->size = size;
    obj->token = token;
    obj->callback = callback;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_buffer_set_pool(MixBuffer *obj, MixBufferPool *pool) {
    obj->pool = pool;
    return MIX_RESULT_SUCCESS;
}

void mix_buffer_unref(MixBuffer * obj) {

    if (NULL != obj) {
        int newRefcount = obj->GetRefCount() - 1;
        LOG_I( "after unref, refcount = %d\n", newRefcount);
        // Unref through base class
        obj->Unref();
        if (1 == newRefcount) {
            return_if_fail(obj->pool != NULL);
            if (obj->callback) {
                obj->callback(obj->token, obj->data);
            }
            mix_bufferpool_put(obj->pool, obj);
        }
    }
}

