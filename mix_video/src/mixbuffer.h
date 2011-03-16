/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

#ifndef __MIX_BUFFER_H__
#define __MIX_BUFFER_H__

#include <mixparams.h>
#include "mixvideodef.h"

/**
 * MIX_BUFFER:
 * @obj: object to be type-casted.
 */
#define MIX_BUFFER(obj) (reinterpret_cast<MixBuffer*>(obj))

/**
 * MIX_IS_BUFFER:
 * @obj: an object.
 *
 * Checks if the given object is an instance of #MixParams
 */
#define MIX_IS_BUFFER(obj) (NULL != MIX_BUFFER(obj))

typedef void (*MixBufferCallback)(ulong token, uchar *data);

class MixBufferPool;

/**
 * MixBuffer:
 *
 * MI-X Buffer Parameter object
 */
class MixBuffer : public MixParams {
public:
    MixBuffer();
    virtual ~MixBuffer();
    virtual bool copy(MixParams* target) const;
    virtual MixParams* dup() const;
    virtual bool equal(MixParams* obj) const;
public:
    /* Pointer to coded data buffer */
    uchar *data;

    /* Size of coded data buffer */
    uint size;

    /* Token that will be passed to
     * the callback function. Can be
     * used by the application for
     * any information to be associated
     * with this coded data buffer,
     * such as a pointer to a structure
     * belonging to the application. */
    ulong token;

    /* callback function pointer */
    MixBufferCallback callback;

    /* < private > */
    MixBufferPool *pool;
};

/**
 * mix_buffer_new:
 * @returns: A newly allocated instance of #MixBuffer
 *
 * Use this method to create new instance of #MixBuffer
 */
MixBuffer *mix_buffer_new(void);
/**
 * mix_buffer_ref:
 * @mix: object to add reference
 * @returns: the #MixBuffer instance where reference count has been increased.
 *
 * Add reference count.
 */
MixBuffer *mix_buffer_ref(MixBuffer * mix);

/**
 * mix_buffer_unref:
 * @obj: object to unref.
 *
 * Decrement reference count of the object.
 */
void mix_buffer_unref(MixBuffer * mix);

/* Class Methods */

/**
 * mix_buffer_set_data:
 * @obj: #MixBuffer object
 * @data: data buffer
 * @size: data buffer size
 * @token:  token
 * @callback: callback function pointer
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set data buffer, size, token and callback function
 */
MIX_RESULT mix_buffer_set_data(MixBuffer * obj, uchar *data, uint size,
                               ulong token, MixBufferCallback callback);

#endif /* __MIX_BUFFER_H__ */
