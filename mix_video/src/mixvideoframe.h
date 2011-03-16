/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

#ifndef __MIX_VIDEOFRAME_H__
#define __MIX_VIDEOFRAME_H__

#include <mixparams.h>
#include "mixvideodef.h"
#include "mixvideothread.h"

class MixSurfacePool;

/**
 * MIX_VIDEOFRAME:
 * @obj: object to be type-casted.
 */
#define MIX_VIDEOFRAME(obj) (reinterpret_cast<MixVideoFrame*>(obj))

/**
 * MIX_IS_VIDEOFRAME:
 * @obj: an object.
 *
 * Checks if the given object is an instance of #MixVideoFrame
 */
#define MIX_IS_VIDEOFRAME(obj) ((NULL != MIX_VIDEOFRAME(obj)) ? TRUE : FALSE)



typedef enum _MixFrameType {
    TYPE_I,
    TYPE_P,
    TYPE_B,
    TYPE_INVALID
} MixFrameType;

/**
 * MixVideoFrame:
 *
 * MI-X VideoConfig Parameter object
 */
class MixVideoFrame : public MixParams {
public:
    MixVideoFrame();
    ~MixVideoFrame();
    virtual bool copy(MixParams *target) const;
    virtual bool equal(MixParams* obj) const;
    virtual MixParams* dup() const;
    void Lock();
    void Unlock();
public:
    /* ID associated with the decoded frame */
    ulong frame_id;

    /* ID associated with the CI frame
     * (used for encode only) */
    uint ci_frame_idx;

    /* 64 bit timestamp. For decode,
     * this is preserved from the corresponding
     * MixVideoDecodeParams field. For encode,
     * this is created during encoding. */
    uint64 timestamp;

    /* Flag indicating whether there
     * is a discontinuity. For decode,
     * this is preserved from the corresponding
     * MixVideoDecodeParams field. */
    bool discontinuity;

    /* Reserved for future use */
    void *reserved1;

    /* Reserved for future use */
    void *reserved2;

    /* Reserved for future use */
    void *reserved3;

    /* Reserved for future use */
    void *reserved4;

public:
    // from structure MixVideoFramePrivate
    MixSurfacePool *pool;
    MixFrameType frame_type;
    bool is_skipped;
    MixVideoFrame *real_frame;
//	GStaticRecMutex lock;
    mutable MixVideoMutex mLock;
    bool sync_flag;
    uint32 frame_structure; // 0: frame, 1: top field, 2: bottom field
    void *va_display;
    uint32 displayorder;

};

/**
 * mix_videoframe_new:
 * @returns: A newly allocated instance of #MixVideoFrame
 *
 * Use this method to create new instance of #MixVideoFrame
 */
MixVideoFrame *mix_videoframe_new(void);
/**
 * mix_videoframe_ref:
 * @mix: object to add reference
 * @returns: the #MixVideoFrame instance where reference count has been increased.
 *
 * Add reference count.
 */
MixVideoFrame *mix_videoframe_ref(MixVideoFrame * obj);

/**
 * mix_videoframe_unref:
 * @obj: object to unref.
 *
 * Decrement reference count of the object.
 */
void mix_videoframe_unref(MixVideoFrame * obj);

/* Class Methods */

/**
 * mix_videoframe_set_frame_id:
 * @obj: #MixVideoFrame object
 * @frame_id: ID associated with the decoded frame
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set Frame ID
 */
MIX_RESULT mix_videoframe_set_frame_id(MixVideoFrame * obj, ulong frame_id);

/**
 * mix_videoframe_get_frame_id:
 * @obj: #MixVideoFrame object
 * @frame_id: frame ID to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get Frame ID
 */
MIX_RESULT mix_videoframe_get_frame_id(MixVideoFrame * obj, ulong * frame_id);

/**
 * mix_videoframe_set_ci_frame_idx:
 * @obj: #MixVideoFrame object
 * @ci_frame_idx: ID associated with the CI frame (used for encode only)
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set CI Frame ID
 */
MIX_RESULT mix_videoframe_set_ci_frame_idx(MixVideoFrame * obj, uint ci_frame_idx);

/**
 * mix_videoframe_get_ci_frame_idx:
 * @obj: #MixVideoFrame object
 * @ci_frame_idx: CI Frame ID to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get CI Frame ID
 */
MIX_RESULT mix_videoframe_get_ci_frame_idx(MixVideoFrame * obj, uint * ci_frame_idx);

/**
 * mix_videoframe_set_timestamp:
 * @obj: #MixVideoFrame object
 * @timestamp: Frame timestamp
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Set Frame timestamp
 */
MIX_RESULT mix_videoframe_set_timestamp(MixVideoFrame * obj, uint64 timestamp);

/**
 * mix_videoframe_get_timestamp:
 * @obj: #MixVideoFrame object
 * @timestamp: Frame timestamp to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get Frame timestamp
 */
MIX_RESULT mix_videoframe_get_timestamp(MixVideoFrame * obj, uint64 * timestamp);

/**
 * mix_videoframe_set_discontinuity:
 * @obj: #MixVideoFrame object
 * @discontinuity: Discontinuity flag
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get discontinuity flag
 */
MIX_RESULT mix_videoframe_set_discontinuity(MixVideoFrame * obj, bool discontinuity);

/**
 * mix_videoframe_get_discontinuity:
 * @obj: #MixVideoFrame object
 * @discontinuity: Discontinuity flag to be returned
 * @returns: <link linkend="MixVideo-mixvideodef">Common Video Error Return Codes</link>
 *
 * Get discontinuity flag
 */
MIX_RESULT mix_videoframe_get_discontinuity(MixVideoFrame * obj, bool * discontinuity);

/**
 * TODO: Add document the following 2 functions
 *
 */
MIX_RESULT mix_videoframe_set_vadisplay(MixVideoFrame * obj, void *va_display);
MIX_RESULT mix_videoframe_get_vadisplay(MixVideoFrame * obj, void **va_display);
MIX_RESULT mix_videoframe_get_frame_structure(MixVideoFrame * obj, uint32* frame_structure);

// from private structure MixVideoFramePrivate
/* Private functions */
MIX_RESULT
mix_videoframe_set_pool (MixVideoFrame *obj, MixSurfacePool *pool);

MIX_RESULT
mix_videoframe_set_frame_type (MixVideoFrame *obj,  MixFrameType frame_type);

MIX_RESULT
mix_videoframe_get_frame_type (MixVideoFrame *obj,  MixFrameType *frame_type);

MIX_RESULT
mix_videoframe_set_is_skipped (MixVideoFrame *obj,  bool is_skipped);

MIX_RESULT
mix_videoframe_get_is_skipped (MixVideoFrame *obj,  bool *is_skipped);

MIX_RESULT
mix_videoframe_set_real_frame (MixVideoFrame *obj,  MixVideoFrame *real);

MIX_RESULT
mix_videoframe_get_real_frame (MixVideoFrame *obj,  MixVideoFrame **real);

MIX_RESULT
mix_videoframe_reset(MixVideoFrame *obj);

MIX_RESULT
mix_videoframe_set_sync_flag(MixVideoFrame *obj, bool sync_flag);

MIX_RESULT
mix_videoframe_get_sync_flag(MixVideoFrame *obj, bool *sync_flag);

MIX_RESULT
mix_videoframe_set_frame_structure(MixVideoFrame * obj, uint32 frame_structure);

MIX_RESULT
mix_videoframe_set_displayorder(MixVideoFrame *obj, uint32 displayorder);

MIX_RESULT
mix_videoframe_get_displayorder(MixVideoFrame *obj, uint32 *displayorder);

#endif /* __MIX_VIDEOFRAME_H__ */
