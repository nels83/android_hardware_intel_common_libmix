/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */


#include "mixvideolog.h"
#include "mixframemanager.h"
#include "mixvideoframe_private.h"

#define INITIAL_FRAME_ARRAY_SIZE 	16

// Assume only one backward reference is used. This will hold up to 2 frames before forcing
// the earliest frame out of queue.
#define MIX_MAX_ENQUEUE_SIZE        2

// RTP timestamp is 32-bit long and could be rollover in 13 hours (based on 90K Hz clock)
#define TS_ROLLOVER_THRESHOLD          (0xFFFFFFFF/2)


#define MIX_SECOND  (1000000 * INT64_CONSTANT (1000))

MixFrameManager::~MixFrameManager() {
    /* cleanup here */
    mix_framemanager_deinitialize(this);
}

MixFrameManager::MixFrameManager()
        :initialized(FALSE)
        ,flushing(FALSE)
        ,eos(FALSE)
        ,mLock()
        ,frame_list(NULL)
        ,framerate_numerator(30)
        ,framerate_denominator(1)
        ,frame_timestamp_delta(0)
        ,mode(MIX_DISPLAY_ORDER_UNKNOWN)
        ,is_first_frame(TRUE)
        ,last_frame_timestamp(0)
        ,next_frame_timestamp(0)
        ,next_frame_picnumber(0)
        ,max_enqueue_size(MIX_MAX_ENQUEUE_SIZE)
        ,max_picture_number((uint32)-1)
        ,ref_count(1) {
}

MixFrameManager *mix_framemanager_new(void) {
    return new MixFrameManager();
}



MixFrameManager *mix_framemanager_ref(MixFrameManager * fm) {
    if (NULL != fm)
        fm->ref_count++;
    return fm;
}

/* MixFrameManager class methods */

MIX_RESULT mix_framemanager_initialize(
    MixFrameManager *fm, MixDisplayOrderMode mode,
    int framerate_numerator, int framerate_denominator) {

    MIX_RESULT ret = MIX_RESULT_SUCCESS;

    if (fm->initialized) {
        return MIX_RESULT_ALREADY_INIT;
    }

    fm->frame_list = NULL;
    fm->framerate_numerator = framerate_numerator;
    fm->framerate_denominator = framerate_denominator;
    fm->frame_timestamp_delta = fm->framerate_denominator * MIX_SECOND
                                / fm->framerate_numerator;

    fm->mode = mode;

    LOG_V("fm->mode = %d\n",  fm->mode);

    fm->is_first_frame = TRUE;
    fm->next_frame_timestamp = 0;
    fm->last_frame_timestamp = 0;
    fm->next_frame_picnumber = 0;
    fm->initialized = TRUE;
cleanup:

    return ret;
}

MIX_RESULT mix_framemanager_deinitialize(MixFrameManager *fm) {
    if (!fm->initialized) {
        return MIX_RESULT_NOT_INIT;
    }

    mix_framemanager_flush(fm);

    fm->mLock.lock();

    fm->initialized = FALSE;

    fm->mLock.unlock();

    return MIX_RESULT_SUCCESS;
}

MixFrameManager* mix_framemanager_unref(MixFrameManager *fm) {
    if (NULL != fm) {
        fm->ref_count--;
        if (0 == fm->ref_count) {
            delete fm;
            return NULL;
        }
    }
    return fm;
}

MIX_RESULT mix_framemanager_set_framerate(
    MixFrameManager *fm, int framerate_numerator, int framerate_denominator) {

    if (framerate_numerator <= 0 || framerate_denominator <= 0) {
        return MIX_RESULT_INVALID_PARAM;
    }
    fm->mLock.lock();
    fm->framerate_numerator = framerate_numerator;
    fm->framerate_denominator = framerate_denominator;
    fm->frame_timestamp_delta = fm->framerate_denominator * MIX_SECOND
                                / fm->framerate_numerator;
    fm->mLock.unlock();
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_framemanager_get_framerate(MixFrameManager *fm,
        int *framerate_numerator, int *framerate_denominator) {
    if (!framerate_numerator || !framerate_denominator) {
        return MIX_RESULT_INVALID_PARAM;
    }
    fm->mLock.lock();
    *framerate_numerator = fm->framerate_numerator;
    *framerate_denominator = fm->framerate_denominator;
    fm->mLock.unlock();
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_framemanager_get_display_order_mode(
    MixFrameManager *fm, MixDisplayOrderMode *mode) {
    if (!mode) {
        return MIX_RESULT_INVALID_PARAM;
    }
    /* no need to use lock */
    *mode = fm->mode;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_framemanager_set_max_enqueue_size(
    MixFrameManager *fm, int size) {
    if (size <= 0) {
        return MIX_RESULT_FAIL;
    }
    fm->mLock.lock();
    fm->max_enqueue_size = size;
    LOG_V("max enqueue size is %d\n", size);
    fm->mLock.unlock();
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_framemanager_set_max_picture_number(
    MixFrameManager *fm, uint32 num) {
    // NOTE: set maximum picture order number only if pic_order_cnt_type is 0  (see H.264 spec)
    if (num < 16) {
        // Refer to H.264 spec: log2_max_pic_order_cnt_lsb_minus4. Max pic order will never be less than 16.
        return MIX_RESULT_INVALID_PARAM;
    }
    fm->mLock.lock();
    // max_picture_number is exclusie (range from 0 to num - 1).
    // Note that this number may not be reliable if encoder does not conform to the spec, as of this, the
    // implementaion will not automatically roll-over fm->next_frame_picnumber when it reaches
    // fm->max_picture_number.
    fm->max_picture_number = num;
    LOG_V("max picture number is %d\n", num);

    fm->mLock.unlock();
    return MIX_RESULT_SUCCESS;
}


MIX_RESULT mix_framemanager_flush(MixFrameManager *fm) {
    MixVideoFrame *frame = NULL;
    if (!fm->initialized) {
        return MIX_RESULT_NOT_INIT;
    }

    fm->mLock.lock();
    while (fm->frame_list) {
        frame = (MixVideoFrame*) j_slist_nth_data(fm->frame_list, 0);
        fm->frame_list = j_slist_remove(fm->frame_list, frame);
        mix_videoframe_unref(frame);
        LOG_V("one frame is flushed\n");
    };

    fm->eos = FALSE;
    fm->is_first_frame = TRUE;
    fm->next_frame_timestamp = 0;
    fm->last_frame_timestamp = 0;
    fm->next_frame_picnumber = 0;

    fm->mLock.unlock();
    return MIX_RESULT_SUCCESS;
}


MIX_RESULT mix_framemanager_enqueue(MixFrameManager *fm, MixVideoFrame *mvf) {

    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    LOG_V("Begin fm->mode = %d\n", fm->mode);

    if (!mvf) {
        return MIX_RESULT_INVALID_PARAM;
    }

    if (!fm->initialized) {
        return MIX_RESULT_NOT_INIT;
    }

    bool discontinuity = FALSE;
    mix_videoframe_get_discontinuity(mvf, &discontinuity);
    if (discontinuity)
    {
        LOG_V("current frame has discontinuity!\n");
        mix_framemanager_flush(fm);
    }
#ifdef MIX_LOG_ENABLE
    if (fm->mode == MIX_DISPLAY_ORDER_PICNUMBER) {
        uint32 num;
        mix_videoframe_get_displayorder(mvf, &num);
        LOG_V("pic %d is enqueued.\n", num);
    }

    if (fm->mode == MIX_DISPLAY_ORDER_TIMESTAMP) {
        uint64 ts;
        mix_videoframe_get_timestamp(mvf, &ts);
        LOG_V("ts %"UINT64_FORMAT" is enqueued.\n", ts);
    }
#endif

    fm->mLock.lock();
    fm->frame_list = j_slist_append(fm->frame_list, (void*)mvf);
    fm->mLock.unlock();
    LOG_V("End\n");
    return ret;
}

void mix_framemanager_update_timestamp(MixFrameManager *fm, MixVideoFrame *mvf)
{
    // this function finds the lowest time stamp in the list and assign it to the dequeued video frame,
    // if that timestamp is smaller than the timestamp of dequeued video frame.
    int i;
    uint64 ts = 0, min_ts = 0;
    MixVideoFrame *p = NULL, *min_p = NULL;
    int len = j_slist_length(fm->frame_list);
    if (len == 0)
    {
        // nothing to update
        return;
    }

    // find video frame with the smallest timestamp, take rollover into account when
    // comparing timestamp.
    for (i = 0; i < len; i++)
    {
        p = (MixVideoFrame*)j_slist_nth_data(fm->frame_list, i);
        mix_videoframe_get_timestamp(p, &ts);
        if (i == 0 ||
                (ts < min_ts && min_ts - ts < TS_ROLLOVER_THRESHOLD) ||
                (ts > min_ts && ts - min_ts > TS_ROLLOVER_THRESHOLD))
        {
            min_ts = ts;
            min_p = p;
        }
    }

    mix_videoframe_get_timestamp(mvf, &ts);
    if ((ts < min_ts && min_ts - ts < TS_ROLLOVER_THRESHOLD) ||
            (ts > min_ts && ts - min_ts > TS_ROLLOVER_THRESHOLD))
    {
        // frame to be updated has smaller time stamp
    }
    else
    {
        // time stamp needs to be monotonically non-decreasing so swap timestamp.
        mix_videoframe_set_timestamp(mvf, min_ts);
        mix_videoframe_set_timestamp(min_p, ts);
        LOG_V("timestamp for current frame is updated from %"UINT64_FORMAT" to %"UINT64_FORMAT"\n",
              ts, min_ts);
    }
}


MIX_RESULT mix_framemanager_pictype_based_dequeue(MixFrameManager *fm, MixVideoFrame **mvf)
{
    int i, num_i_or_p;
    MixVideoFrame *p, *first_i_or_p;
    MixFrameType type;
    int len = j_slist_length(fm->frame_list);

    num_i_or_p = 0;
    first_i_or_p = NULL;

    for (i = 0; i < len; i++)
    {
        p = (MixVideoFrame*)j_slist_nth_data(fm->frame_list, i);
        mix_videoframe_get_frame_type(p, &type);
        if (type == TYPE_B)
        {
            // B frame has higher display priority as only one reference frame is kept in the list
            // and it should be backward reference frame for B frame.
            fm->frame_list = j_slist_remove(fm->frame_list, p);
            mix_framemanager_update_timestamp(fm, p);
            *mvf = p;
            LOG_V("B frame is dequeued.\n");
            return MIX_RESULT_SUCCESS;
        }

        if (type != TYPE_I && type != TYPE_P)
        {
            // this should never happen
            LOG_E("Frame typs is invalid!!!\n");
            fm->frame_list = j_slist_remove(fm->frame_list, p);
            mix_videoframe_unref(p);
            return MIX_RESULT_FRAME_NOTAVAIL;
        }
        num_i_or_p++;
        if (first_i_or_p == NULL)
        {
            first_i_or_p = p;
        }
    }

    // if there are more than one reference frame in the list, the first one is dequeued.
    if (num_i_or_p > 1 || fm->eos)
    {
        if (first_i_or_p == NULL)
        {
            // this should never happen!
            LOG_E("first_i_or_p frame is NULL!\n");
            return MIX_RESULT_FAIL;
        }
        fm->frame_list = j_slist_remove(fm->frame_list, first_i_or_p);
        mix_framemanager_update_timestamp(fm, first_i_or_p);
        *mvf = first_i_or_p;
#ifdef MIX_LOG_ENABLE
        mix_videoframe_get_frame_type(first_i_or_p, &type);
        if (type == TYPE_I)
        {
            LOG_V("I frame is dequeued.\n");
        }
        else
        {
            LOG_V("P frame is dequeued.\n");
        }
#endif
        return MIX_RESULT_SUCCESS;
    }

    return MIX_RESULT_FRAME_NOTAVAIL;
}

MIX_RESULT mix_framemanager_timestamp_based_dequeue(MixFrameManager *fm, MixVideoFrame **mvf)
{
    int i, len;
    MixVideoFrame *p, *p_out_of_dated;
    uint64 ts, ts_next_pending, ts_out_of_dated;
    uint64 tolerance = fm->frame_timestamp_delta/4;

retry:
    // len may be changed during retry!
    len = j_slist_length(fm->frame_list);
    ts_next_pending = (uint64)-1;
    ts_out_of_dated = 0;
    p_out_of_dated = NULL;


    for (i = 0; i < len; i++)
    {
        p = (MixVideoFrame*)j_slist_nth_data(fm->frame_list, i);
        mix_videoframe_get_timestamp(p, &ts);
        if (ts >= fm->last_frame_timestamp &&
                ts <= fm->next_frame_timestamp + tolerance)
        {
            fm->frame_list = j_slist_remove(fm->frame_list, p);
            *mvf = p;
            mix_videoframe_get_timestamp(p, &(fm->last_frame_timestamp));
            fm->next_frame_timestamp = fm->last_frame_timestamp + fm->frame_timestamp_delta;
            LOG_V("frame is dequeud, ts = %"UINT64_FORMAT".\n", ts);
            return MIX_RESULT_SUCCESS;
        }

        if (ts > fm->next_frame_timestamp + tolerance &&
                ts < ts_next_pending)
        {
            ts_next_pending = ts;
        }
        if (ts < fm->last_frame_timestamp &&
                ts >= ts_out_of_dated)
        {
            // video frame that is most recently out-of-dated.
            // this may happen in variable frame rate scenario where two adjacent frames both meet
            // the "next frame" criteria, and the one with larger timestamp is dequeued first.
            ts_out_of_dated = ts;
            p_out_of_dated = p;
        }
    }

    if (p_out_of_dated &&
            fm->last_frame_timestamp - ts_out_of_dated < TS_ROLLOVER_THRESHOLD)
    {
        fm->frame_list = j_slist_remove(fm->frame_list, p_out_of_dated);
        mix_videoframe_unref(p_out_of_dated);
        LOG_W("video frame is out of dated. ts = %"UINT64_FORMAT" compared to last ts =  %"UINT64_FORMAT".\n",
              ts_out_of_dated, fm->last_frame_timestamp);
        return MIX_RESULT_FRAME_NOTAVAIL;
    }

    if (len <= fm->max_enqueue_size && fm->eos == FALSE)
    {
        LOG_V("no frame is dequeued, expected ts = %"UINT64_FORMAT", next pending ts = %"UINT64_FORMAT".(List size = %d)\n",
              fm->next_frame_timestamp, ts_next_pending, len);
        return MIX_RESULT_FRAME_NOTAVAIL;
    }

    // timestamp has gap
    if (ts_next_pending != (uint64)-1)
    {
        LOG_V("timestamp has gap, jumping from %"UINT64_FORMAT" to %"UINT64_FORMAT".\n",
              fm->next_frame_timestamp, ts_next_pending);

        fm->next_frame_timestamp = ts_next_pending;
        goto retry;
    }

    // time stamp roll-over
    LOG_V("time stamp is rolled over, resetting next frame timestamp from %"UINT64_FORMAT" to 0.\n",
          fm->next_frame_timestamp);

    fm->next_frame_timestamp = 0;
    fm->last_frame_timestamp = 0;
    goto retry;

    // should never run to here
    LOG_E("Error in timestamp-based dequeue implementation!\n");
    return MIX_RESULT_FAIL;
}

MIX_RESULT mix_framemanager_picnumber_based_dequeue(MixFrameManager *fm, MixVideoFrame **mvf)
{
    int i, len;
    MixVideoFrame* p;
    uint32 picnum;
    uint32 next_picnum_pending;

    len = j_slist_length(fm->frame_list);

retry:
    next_picnum_pending = (uint32)-1;

    for (i = 0; i < len; i++)
    {
        p = (MixVideoFrame*)j_slist_nth_data(fm->frame_list, i);
        mix_videoframe_get_displayorder(p, &picnum);
        if (picnum == fm->next_frame_picnumber)
        {
            fm->frame_list = j_slist_remove(fm->frame_list, p);
            mix_framemanager_update_timestamp(fm, p);
            *mvf = p;
            LOG_V("frame is dequeued, poc = %d.\n", fm->next_frame_picnumber);
            fm->next_frame_picnumber++;
            //if (fm->next_frame_picnumber == fm->max_picture_number)
            //    fm->next_frame_picnumber = 0;
            return MIX_RESULT_SUCCESS;
        }

        if (picnum > fm->next_frame_picnumber &&
                picnum < next_picnum_pending)
        {
            next_picnum_pending = picnum;
        }

        if (picnum < fm->next_frame_picnumber &&
                fm->next_frame_picnumber - picnum < 8)
        {
            // the smallest value of "max_pic_order_cnt_lsb_minus4" is 16. If the distance of "next frame pic number"
            // to the pic number  in the list is less than half of 16, it is safe to assume that pic number
            // is reset when a new IDR is encoded. (where pic numbfer of top or bottom field must be 0, subclause 8.2.1).
            LOG_V("picture number is reset to %d, next pic number is %d, next pending number is %d.\n",
                  picnum, fm->next_frame_picnumber, next_picnum_pending);
            break;
        }
    }

    if (len <= fm->max_enqueue_size && fm->eos == FALSE)
    {
        LOG_V("No frame is dequeued. Expected POC = %d, next pending POC = %d. (List size = %d)\n",
              fm->next_frame_picnumber, next_picnum_pending, len);
        return MIX_RESULT_FRAME_NOTAVAIL;
    }

    // picture number  has gap
    if (next_picnum_pending != (uint32)-1)
    {
        LOG_V("picture number has gap, jumping from %d to %d.\n",
              fm->next_frame_picnumber, next_picnum_pending);

        fm->next_frame_picnumber = next_picnum_pending;
        goto retry;
    }

    // picture number roll-over
    LOG_V("picture number is rolled over, resetting next picnum from %d to 0.\n",
          fm->next_frame_picnumber);

    fm->next_frame_picnumber = 0;
    goto retry;

    // should never run to here
    LOG_E("Error in picnumber-based dequeue implementation!\n");
    return MIX_RESULT_FAIL;
}

MIX_RESULT mix_framemanager_dequeue(MixFrameManager *fm, MixVideoFrame **mvf) {

    MIX_RESULT ret = MIX_RESULT_SUCCESS;

    LOG_V("Begin\n");

    if (!mvf) {
        return MIX_RESULT_INVALID_PARAM;
    }

    if (!fm->initialized) {
        return MIX_RESULT_NOT_INIT;
    }

    fm->mLock.lock();

    if (fm->frame_list == NULL)
    {
        if (fm->eos)
        {
            LOG_V("No frame is dequeued (eos)!\n");
            ret = MIX_RESULT_EOS;
        }
        else
        {
            LOG_V("No frame is dequeued as queue is empty!\n");
            ret = MIX_RESULT_FRAME_NOTAVAIL;
        }
    }
    else if (fm->is_first_frame)
    {
        // dequeue the first entry in the list. Not need to update the time stamp as
        // the list should contain only one frame.
#ifdef MIX_LOG_ENABLE
        if (j_slist_length(fm->frame_list) != 1)
        {
            LOG_W("length of list is not equal to 1 for the first frame.\n");
        }
#endif
        *mvf = (MixVideoFrame*) j_slist_nth_data(fm->frame_list, 0);
        fm->frame_list = j_slist_remove(fm->frame_list, (*mvf));

        if (fm->mode == MIX_DISPLAY_ORDER_TIMESTAMP)
        {
            mix_videoframe_get_timestamp(*mvf, &(fm->last_frame_timestamp));
            fm->next_frame_timestamp = fm->last_frame_timestamp + fm->frame_timestamp_delta;
            LOG_V("The first frame is dequeued, ts = %"UINT64_FORMAT"\n", fm->last_frame_timestamp);
        }
        else if (fm->mode == MIX_DISPLAY_ORDER_PICNUMBER)
        {
            mix_videoframe_get_displayorder(*mvf, &(fm->next_frame_picnumber));
            LOG_V("The first frame is dequeued, POC = %d\n", fm->next_frame_picnumber);
            fm->next_frame_picnumber++;
            //if (fm->next_frame_picnumber == fm->max_picture_number)
            //   fm->next_frame_picnumber = 0;
        }
        else
        {
#ifdef MIX_LOG_ENABLE
            MixFrameType type;
            mix_videoframe_get_frame_type(*mvf, &type);
            LOG_V("The first frame is dequeud, frame type is %d.\n", type);
#endif
        }
        fm->is_first_frame = FALSE;

        ret = MIX_RESULT_SUCCESS;
    }
    else
    {
        // not the first frame and list is not empty
        switch (fm->mode)
        {
        case MIX_DISPLAY_ORDER_TIMESTAMP:
            ret = mix_framemanager_timestamp_based_dequeue(fm, mvf);
            break;

        case MIX_DISPLAY_ORDER_PICNUMBER:
            ret = mix_framemanager_picnumber_based_dequeue(fm, mvf);
            break;

        case MIX_DISPLAY_ORDER_PICTYPE:
            ret = mix_framemanager_pictype_based_dequeue(fm, mvf);
            break;

        case MIX_DISPLAY_ORDER_FIFO:
            *mvf = (MixVideoFrame*) j_slist_nth_data(fm->frame_list, 0);
            fm->frame_list = j_slist_remove(fm->frame_list, (*mvf));
            ret = MIX_RESULT_SUCCESS;
            LOG_V("One frame is dequeued.\n");
            break;

        default:
            LOG_E("Invalid frame order mode\n");
            ret = MIX_RESULT_FAIL;
            break;
        }
    }

    fm->mLock.unlock();
    LOG_V("End\n");
    return ret;
}

MIX_RESULT mix_framemanager_eos(MixFrameManager *fm) {
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    if (!fm->initialized) {
        return MIX_RESULT_NOT_INIT;
    }
    fm->mLock.lock();
    fm->eos = TRUE;
    LOG_V("EOS is received.\n");
    fm->mLock.unlock();
    return ret;
}

