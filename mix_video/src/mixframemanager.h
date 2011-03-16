/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

#ifndef __MIX_FRAMEMANAGER_H__
#define __MIX_FRAMEMANAGER_H__

#include <mixtypes.h>
#include "mixvideodef.h"
#include "mixvideoframe.h"
#include "mixvideothread.h"
#include <j_slist.h>

/*
* MIX_FRAMEORDER_MODE_DECODEORDER is here interpreted as
* MIX_DISPLAY_ORDER_FIFO,  a special case of display order mode.
*/
typedef enum
{
    MIX_DISPLAY_ORDER_UNKNOWN,
    MIX_DISPLAY_ORDER_FIFO,
    MIX_DISPLAY_ORDER_TIMESTAMP,
    MIX_DISPLAY_ORDER_PICNUMBER,
    MIX_DISPLAY_ORDER_PICTYPE,
    MIX_DISPLAY_ORDER_LAST
} MixDisplayOrderMode;


class MixFrameManager {
public:
    MixFrameManager();
    ~MixFrameManager();

public:
    bool initialized;
    bool flushing;
    bool eos;
    MixVideoMutex mLock;
    JSList* frame_list;
    int framerate_numerator;
    int framerate_denominator;
    uint64 frame_timestamp_delta;
    MixDisplayOrderMode mode;
    bool is_first_frame;
    uint64 last_frame_timestamp;
    uint64 next_frame_timestamp;
    uint32 next_frame_picnumber;
    int    max_enqueue_size;
    uint32 max_picture_number;
    uint32 ref_count;
};


/**
 * mix_framemanager_new:
 * @returns: A newly allocated instance of #MixFrameManager
 *
 * Use this method to create new instance of #MixFrameManager
 */
MixFrameManager *mix_framemanager_new(void);

/**
 * mix_framemanager_ref:
 * @mix: object to add reference
 * @returns: the MixFrameManager instance where reference count has been increased.
 *
 * Add reference count.
 */
MixFrameManager *mix_framemanager_ref(MixFrameManager * mix);

/**
 * mix_framemanager_unref:
 * @obj: object to unref.
 *
 * Decrement reference count of the object.
 */
MixFrameManager* mix_framemanager_unref(MixFrameManager* fm);

/* Class Methods */

/*
 * Initialize FM
 */
MIX_RESULT mix_framemanager_initialize(
    MixFrameManager *fm, MixDisplayOrderMode mode,
    int framerate_numerator, int framerate_denominator);
/*
 * Deinitialize FM
 */
MIX_RESULT mix_framemanager_deinitialize(MixFrameManager *fm);

/*
 * Set new framerate
 */
MIX_RESULT mix_framemanager_set_framerate(
    MixFrameManager *fm, int framerate_numerator, int framerate_denominator);

/*
 * Get framerate
 */
MIX_RESULT mix_framemanager_get_framerate(
    MixFrameManager *fm, int *framerate_numerator, int *framerate_denominator);


/*
 * Set miximum size of queue
 */
MIX_RESULT mix_framemanager_set_max_enqueue_size(
    MixFrameManager *fm, int size);


/*
 * Set miximum picture number
 */
MIX_RESULT mix_framemanager_set_max_picture_number(
    MixFrameManager *fm, uint32 num);


/*
 * Get Display Order Mode
 */
MIX_RESULT mix_framemanager_get_display_order_mode(
    MixFrameManager *fm, MixDisplayOrderMode *mode);

/*
 * For discontiunity, reset FM
 */
MIX_RESULT mix_framemanager_flush(MixFrameManager *fm);

/*
 * Enqueue MixVideoFrame
 */
MIX_RESULT mix_framemanager_enqueue(MixFrameManager *fm, MixVideoFrame *mvf);

/*
 * Dequeue MixVideoFrame in proper order depends on MixDisplayOrderMode value
 * during initialization.
 */
MIX_RESULT mix_framemanager_dequeue(MixFrameManager *fm, MixVideoFrame **mvf);

/*
 * End of stream.
 */
MIX_RESULT mix_framemanager_eos(MixFrameManager *fm);

#endif /* __MIX_FRAMEMANAGER_H__ */
