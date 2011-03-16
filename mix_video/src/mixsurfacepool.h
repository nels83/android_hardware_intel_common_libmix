/*
INTEL CONFIDENTIAL
Copyright 2009 Intel Corporation All Rights Reserved.
The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
*/

#ifndef __MIX_SURFACEPOOL_H__
#define __MIX_SURFACEPOOL_H__

#include <mixparams.h>
#include "mixvideodef.h"
#include "mixvideoframe.h"
#include "mixvideothread.h"
#include <va/va.h>
#include <j_slist.h>


/**
* MIX_SURFACEPOOL:
* @obj: object to be type-casted.
*/
#define MIX_SURFACEPOOL(obj) (reinterpret_cast<MixSurfacePool*>(obj))

/**
* MixSurfacePool:
*
* MI-X Video Surface Pool object
*/
class MixSurfacePool : public MixParams
{
public:
    /*< public > */
    JSList *free_list;		/* list of free surfaces */
    JSList *in_use_list;		/* list of surfaces in use */
    ulong free_list_max_size;	/* initial size of the free list */
    ulong free_list_cur_size;	/* current size of the free list */
    ulong high_water_mark;	/* most surfaces in use at one time */
    bool initialized;
//  uint64 timestamp;

    void *reserved1;
    void *reserved2;
    void *reserved3;
    void *reserved4;

    /*< private > */
    mutable MixVideoMutex mLock;
public:
    MixSurfacePool();
    virtual ~MixSurfacePool();
    virtual bool copy(MixParams *target) const;
    virtual bool equal(MixParams* obj) const;
    virtual MixParams* dup() const;
};

/**
* mix_surfacepool_new:
* @returns: A newly allocated instance of #MixSurfacePool
*
* Use this method to create new instance of #MixSurfacePool
*/
MixSurfacePool *mix_surfacepool_new (void);
/**
* mix_surfacepool_ref:
* @mix: object to add reference
* @returns: the MixSurfacePool instance where reference count has been increased.
*
* Add reference count.
*/
MixSurfacePool *mix_surfacepool_ref (MixSurfacePool * mix);

/**
* mix_surfacepool_unref:
* @obj: object to unref.
*
* Decrement reference count of the object.
*/
#define mix_surfacepool_unref(obj) mix_params_unref(MIX_PARAMS(obj))

/* Class Methods */

MIX_RESULT mix_surfacepool_initialize (MixSurfacePool * obj,
                                       VASurfaceID *surfaces, uint num_surfaces, VADisplay va_display);
MIX_RESULT mix_surfacepool_put (MixSurfacePool * obj,
                                MixVideoFrame * frame);

MIX_RESULT mix_surfacepool_get (MixSurfacePool * obj,
                                MixVideoFrame ** frame);

MIX_RESULT mix_surfacepool_get_frame_with_ci_frameidx (MixSurfacePool * obj,
        MixVideoFrame ** frame, MixVideoFrame *in_frame);

MIX_RESULT mix_surfacepool_check_available (MixSurfacePool * obj);

MIX_RESULT mix_surfacepool_deinitialize (MixSurfacePool * obj);

#endif /* __MIX_SURFACEPOOL_H__ */
