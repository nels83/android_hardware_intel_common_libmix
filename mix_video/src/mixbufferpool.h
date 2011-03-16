/*
INTEL CONFIDENTIAL
Copyright 2009 Intel Corporation All Rights Reserved.
The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
*/

#ifndef __MIX_BUFFERPOOL_H__
#define __MIX_BUFFERPOOL_H__

#include <mixparams.h>
#include "mixvideodef.h"
#include "mixbuffer.h"
#include "mixvideothread.h"
#include <va/va.h>
#include <j_slist.h>


class MixBuffer;

/**
* MIX_BUFFERPOOL:
* @obj: object to be type-casted.
*/
#define MIX_BUFFERPOOL(obj) (reinterpret_cast<MixBufferPool*>(obj))

/**
* MIX_IS_BUFFERPOOL:
* @obj: an object.
*
* Checks if the given object is an instance of #MixBufferPool
*/
#define MIX_IS_BUFFERPOOL(obj) (NULL != MIX_BUFFERPOOL(obj))

/**
* MixBufferPool:
*
* MI-X Video Buffer Pool object
*/
class MixBufferPool : public MixParams
{
public:
    MixBufferPool();
    virtual ~MixBufferPool();
    virtual bool copy(MixParams* target) const;
    virtual MixParams* dup() const;
    virtual bool equal(MixParams* obj) const;

    void Lock() {
        mLock.lock();
    }
    void Unlock() {
        mLock.unlock();
    }
public:
    /*< public > */
    JSList *free_list;		/* list of free buffers */
    JSList *in_use_list;		/* list of buffers in use */
    ulong free_list_max_size;	/* initial size of the free list */
    ulong high_water_mark;	/* most buffers in use at one time */

    void *reserved1;
    void *reserved2;
    void *reserved3;
    void *reserved4;

    /*< private > */
    MixVideoMutex mLock;
};



/**
* mix_bufferpool_new:
* @returns: A newly allocated instance of #MixBufferPool
*
* Use this method to create new instance of #MixBufferPool
*/
MixBufferPool *mix_bufferpool_new (void);
/**
* mix_bufferpool_ref:
* @mix: object to add reference
* @returns: the MixBufferPool instance where reference count has been increased.
*
* Add reference count.
*/
MixBufferPool *mix_bufferpool_ref (MixBufferPool * mix);

/**
* mix_bufferpool_unref:
* @obj: object to unref.
*
* Decrement reference count of the object.
*/
#define mix_bufferpool_unref(obj) mix_params_unref(MIX_PARAMS(obj))

/* Class Methods */
MIX_RESULT mix_bufferpool_initialize (MixBufferPool * obj, uint num_buffers);
MIX_RESULT mix_bufferpool_put (MixBufferPool * obj, MixBuffer * buffer);
MIX_RESULT mix_bufferpool_get (MixBufferPool * obj, MixBuffer ** buffer);
MIX_RESULT mix_bufferpool_deinitialize (MixBufferPool * obj);

#endif /* __MIX_BUFFERPOOL_H__ */
