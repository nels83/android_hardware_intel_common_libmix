/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

/**
 * SECTION:mixsurfacepool
 * @short_description: MI-X Video Surface Pool
 *
 * A data object which stores and manipulates a pool of video surfaces.
 */

#include "mixvideolog.h"
#include "mixsurfacepool.h"
#include "mixvideoframe_private.h"

#define MIX_LOCK(lock) g_mutex_lock(lock);
#define MIX_UNLOCK(lock) g_mutex_unlock(lock);


#define SAFE_FREE(p) if(p) { free(p); p = NULL; }

MixSurfacePool::MixSurfacePool()
/* initialize properties here */
        :free_list(NULL)
        ,in_use_list(NULL)
        ,free_list_max_size(0)
        ,free_list_cur_size(0)
        ,high_water_mark(0)
        ,initialized(FALSE)
        ,reserved1(NULL)
        ,reserved2(NULL)
        ,reserved3(NULL)
        ,reserved4(NULL)
        ,mLock() {
}

MixSurfacePool::~MixSurfacePool() {
}

MixParams* MixSurfacePool::dup() const {
    MixParams *ret = NULL;
    mLock.lock();
    ret = new MixSurfacePool();
    if (NULL != ret) {
        if (FALSE == copy(ret)) {
            ret->Unref();
            ret = NULL;
        }
    }
    mLock.unlock();
    return ret;
}

bool MixSurfacePool::copy(MixParams* target) const {
    if (NULL == target) return FALSE;
    MixSurfacePool* this_target = MIX_SURFACEPOOL(target);

    mLock.lock();
    this_target->mLock.lock();
    // Free the existing properties
    // Duplicate string
    this_target->free_list = free_list;
    this_target->in_use_list = in_use_list;
    this_target->free_list_max_size = free_list_max_size;
    this_target->free_list_cur_size = free_list_cur_size;
    this_target->high_water_mark = high_water_mark;

    this_target->mLock.unlock();
    mLock.unlock();

    MixParams::copy(target);
    return TRUE;
}

bool MixSurfacePool::equal(MixParams *first) const {
    if (NULL == first) return FALSE;
    bool ret = FALSE;
    MixSurfacePool *this_first = MIX_SURFACEPOOL(first);
    mLock.lock();
    this_first->mLock.lock();
    if (this_first->free_list == free_list
            && this_first->in_use_list == in_use_list
            && this_first->free_list_max_size
            == free_list_max_size
            && this_first->free_list_cur_size
            == free_list_cur_size
            && this_first->high_water_mark == high_water_mark) {
        ret = MixParams::equal(first);
    }
    this_first->mLock.unlock();
    mLock.unlock();
    return ret;
}

MixSurfacePool *
mix_surfacepool_new(void) {
    return new MixSurfacePool();
}

MixSurfacePool *
mix_surfacepool_ref(MixSurfacePool * mix) {
    if (NULL != mix)
        mix->Ref();
    return mix;
}

/*  Class Methods  */

/**
 * mix_surfacepool_initialize:
 * @returns: MIX_RESULT_SUCCESS if successful in creating the surface pool
 *
 * Use this method to create a new surface pool, consisting of a GSList of
 * frame objects that represents a pool of surfaces.
 */
MIX_RESULT mix_surfacepool_initialize(MixSurfacePool * obj,
                                      VASurfaceID *surfaces, uint num_surfaces, VADisplay va_display) {

    LOG_V( "Begin\n");

    if (obj == NULL || surfaces == NULL) {

        LOG_E(
            "Error NULL ptrs, obj %x, surfaces %x\n", (uint) obj,
            (uint) surfaces);

        return MIX_RESULT_NULL_PTR;
    }

    obj->mLock.lock();

    if ((obj->free_list != NULL) || (obj->in_use_list != NULL)) {
        //surface pool is in use; return error; need proper cleanup
        //TODO need cleanup here?

        obj->mLock.unlock();

        return MIX_RESULT_ALREADY_INIT;
    }

    if (num_surfaces == 0) {
        obj->free_list = NULL;

        obj->in_use_list = NULL;

        obj->free_list_max_size = num_surfaces;

        obj->free_list_cur_size = num_surfaces;

        obj->high_water_mark = 0;

        /* assume it is initialized */
        obj->initialized = TRUE;

        obj->mLock.unlock();

        return MIX_RESULT_SUCCESS;
    }

    // Initialize the free pool with frame objects

    uint i = 0;
    MixVideoFrame *frame = NULL;

    for (; i < num_surfaces; i++) {

        //Create a frame object for each surface ID
        frame = mix_videoframe_new();

        if (frame == NULL) {
            //TODO need to log an error here and do cleanup

            obj->mLock.unlock();

            return MIX_RESULT_NO_MEMORY;
        }

        // Set the frame ID to the surface ID
        mix_videoframe_set_frame_id(frame, surfaces[i]);
        // Set the ci frame index to the surface ID
        mix_videoframe_set_ci_frame_idx (frame, i);
        // Leave timestamp for each frame object as zero
        // Set the pool reference in the private data of the frame object
        mix_videoframe_set_pool(frame, obj);

        mix_videoframe_set_vadisplay(frame, va_display);

        //Add each frame object to the pool list
        obj->free_list = j_slist_append(obj->free_list, frame);

    }

    obj->in_use_list = NULL;

    obj->free_list_max_size = num_surfaces;

    obj->free_list_cur_size = num_surfaces;

    obj->high_water_mark = 0;

    obj->initialized = TRUE;

    obj->mLock.unlock();

    LOG_V( "End\n");

    return MIX_RESULT_SUCCESS;
}

/**
 * mix_surfacepool_put:
 * @returns: SUCCESS or FAILURE
 *
 * Use this method to return a surface to the free pool
 */
MIX_RESULT mix_surfacepool_put(MixSurfacePool * obj, MixVideoFrame * frame) {

    LOG_V( "Begin\n");
    if (obj == NULL || frame == NULL)
        return MIX_RESULT_NULL_PTR;

    LOG_V( "Frame id: %d\n", frame->frame_id);
    obj->mLock.lock();

    if (obj->in_use_list == NULL) {
        //in use list cannot be empty if a frame is in use
        //TODO need better error code for this

        obj->mLock.unlock();

        return MIX_RESULT_FAIL;
    }

    JSList *element = j_slist_find(obj->in_use_list, frame);
    if (element == NULL) {
        //Integrity error; frame not found in in use list
        //TODO need better error code and handling for this

        obj->mLock.unlock();

        return MIX_RESULT_FAIL;
    } else {
        //Remove this element from the in_use_list
        obj->in_use_list = j_slist_remove_link(obj->in_use_list, element);

        //Concat the element to the free_list and reset the timestamp of the frame
        //Note that the surface ID stays valid
        mix_videoframe_set_timestamp(frame, 0);
        obj->free_list = j_slist_concat(obj->free_list, element);

        //increment the free list count
        obj->free_list_cur_size++;
    }

    //Note that we do nothing with the ref count for this.  We want it to
    //stay at 1, which is what triggered it to be added back to the free list.

    obj->mLock.unlock();

    LOG_V( "End\n");
    return MIX_RESULT_SUCCESS;
}

/**
 * mix_surfacepool_get:
 * @returns: SUCCESS or FAILURE
 *
 * Use this method to get a surface from the free pool
 */
MIX_RESULT mix_surfacepool_get(MixSurfacePool * obj, MixVideoFrame ** frame) {

    LOG_V( "Begin\n");

    if (obj == NULL || frame == NULL)
        return MIX_RESULT_NULL_PTR;

    obj->mLock.lock();

#if 0
    if (obj->free_list == NULL) {
#else
    if (obj->free_list_cur_size <= 1) {  //Keep one surface free at all times for VBLANK bug
#endif
        //We are out of surfaces
        //TODO need to log this as well

        obj->mLock.unlock();

        LOG_E( "out of surfaces\n");

        return MIX_RESULT_OUTOFSURFACES;
    }

    //Remove a frame from the free pool

    //We just remove the one at the head, since it's convenient
    JSList *element = obj->free_list;
    obj->free_list = j_slist_remove_link(obj->free_list, element);
    if (element == NULL) {
        //Unexpected behavior
        //TODO need better error code and handling for this

        obj->mLock.unlock();

        LOG_E( "Element is null\n");

        return MIX_RESULT_FAIL;
    } else {
        //Concat the element to the in_use_list
        obj->in_use_list = j_slist_concat(obj->in_use_list, element);

        //TODO replace with proper logging

        LOG_I( "frame refcount%d\n",
               MIX_PARAMS(element->data)->ref_count);

        //Set the out frame pointer
        *frame = (MixVideoFrame *) element->data;

        LOG_V( "Frame id: %d\n", (*frame)->frame_id);

        //decrement the free list count
        obj->free_list_cur_size--;

        //Check the high water mark for surface use
        uint size = j_slist_length(obj->in_use_list);
        if (size > obj->high_water_mark)
            obj->high_water_mark = size;
        //TODO Log this high water mark
    }

    //Increment the reference count for the frame
    mix_videoframe_ref(*frame);

    obj->mLock.unlock();

    LOG_V( "End\n");

    return MIX_RESULT_SUCCESS;
}


int mixframe_compare_index (MixVideoFrame * a, MixVideoFrame * b)
{
    if (a == NULL || b == NULL)
        return -1;
    if (a->ci_frame_idx == b->ci_frame_idx)
        return 0;
    else
        return -1;
}

/**
 * mix_surfacepool_get:
 * @returns: SUCCESS or FAILURE
 *
 * Use this method to get a surface from the free pool according to the CI frame idx
 */

MIX_RESULT mix_surfacepool_get_frame_with_ci_frameidx (MixSurfacePool * obj, MixVideoFrame ** frame, MixVideoFrame *in_frame) {

    LOG_V( "Begin\n");

    if (obj == NULL || frame == NULL)
        return MIX_RESULT_NULL_PTR;

    obj->mLock.lock();

    if (obj->free_list == NULL) {
        //We are out of surfaces
        //TODO need to log this as well

        obj->mLock.unlock();

        LOG_E( "out of surfaces\n");

        return MIX_RESULT_OUTOFSURFACES;
    }

    //Remove a frame from the free pool

    //We just remove the one at the head, since it's convenient
    JSList *element = j_slist_find_custom (obj->free_list, in_frame, (JCompareFunc) mixframe_compare_index);
    obj->free_list = j_slist_remove_link(obj->free_list, element);
    if (element == NULL) {
        //Unexpected behavior
        //TODO need better error code and handling for this

        obj->mLock.unlock();

        LOG_E( "Element associated with the given frame index is null\n");

        return MIX_RESULT_DROPFRAME;
    } else {
        //Concat the element to the in_use_list
        obj->in_use_list = j_slist_concat(obj->in_use_list, element);

        //TODO replace with proper logging

        LOG_I( "frame refcount%d\n",
               MIX_PARAMS(element->data)->ref_count);

        //Set the out frame pointer
        *frame = (MixVideoFrame *) element->data;

        //Check the high water mark for surface use
        uint size = j_slist_length(obj->in_use_list);
        if (size > obj->high_water_mark)
            obj->high_water_mark = size;
        //TODO Log this high water mark
    }

    //Increment the reference count for the frame
    mix_videoframe_ref(*frame);

    obj->mLock.unlock();

    LOG_V( "End\n");

    return MIX_RESULT_SUCCESS;
}
/**
 * mix_surfacepool_check_available:
 * @returns: SUCCESS or FAILURE
 *
 * Use this method to check availability of getting a surface from the free pool
 */
MIX_RESULT mix_surfacepool_check_available(MixSurfacePool * obj) {

    LOG_V( "Begin\n");

    if (obj == NULL)
        return MIX_RESULT_NULL_PTR;

    obj->mLock.lock();

    if (obj->initialized == FALSE)
    {
        LOG_W("surface pool is not initialized, probably configuration data has not been received yet.\n");
        obj->mLock.unlock();
        return MIX_RESULT_NOT_INIT;
    }


#if 0
    if (obj->free_list == NULL) {
#else
    if (obj->free_list_cur_size <= 1) {  //Keep one surface free at all times for VBLANK bug
#endif
        //We are out of surfaces

        obj->mLock.unlock();

        LOG_W(
            "Returning MIX_RESULT_POOLEMPTY because out of surfaces\n");

        return MIX_RESULT_POOLEMPTY;
    } else {
        //Pool is not empty

        obj->mLock.unlock();

        LOG_I(
            "Returning MIX_RESULT_SUCCESS because surfaces are available\n");

        return MIX_RESULT_SUCCESS;
    }

}

/**
 * mix_surfacepool_deinitialize:
 * @returns: SUCCESS or FAILURE
 *
 * Use this method to teardown a surface pool
 */
MIX_RESULT mix_surfacepool_deinitialize(MixSurfacePool * obj) {
    if (obj == NULL)
        return MIX_RESULT_NULL_PTR;

    obj->mLock.lock();

    if ((obj->in_use_list != NULL) || (j_slist_length(obj->free_list)
                                       != obj->free_list_max_size)) {
        //TODO better error code
        //We have outstanding frame objects in use and they need to be
        //freed before we can deinitialize.

        obj->mLock.unlock();

        return MIX_RESULT_FAIL;
    }

    //Now remove frame objects from the list

    MixVideoFrame *frame = NULL;

    while (obj->free_list != NULL) {
        //Get the frame object from the head of the list
        frame = reinterpret_cast<MixVideoFrame*>(obj->free_list->data);
        //frame = g_slist_nth_data(obj->free_list, 0);

        //Release it
        mix_videoframe_unref(frame);

        //Delete the head node of the list and store the new head
        obj->free_list = j_slist_delete_link(obj->free_list, obj->free_list);

        //Repeat until empty
    }

    obj->free_list_max_size = 0;
    obj->free_list_cur_size = 0;

    //May want to log this information for tuning
    obj->high_water_mark = 0;

    obj->mLock.unlock();

    return MIX_RESULT_SUCCESS;
}

#define MIX_SURFACEPOOL_SETTER_CHECK_INPUT(obj) \
	if(!obj) return MIX_RESULT_NULL_PTR; \
	if(!MIX_IS_SURFACEPOOL(obj)) return MIX_RESULT_FAIL; \
 
#define MIX_SURFACEPOOL_GETTER_CHECK_INPUT(obj, prop) \
	if(!obj || !prop) return MIX_RESULT_NULL_PTR; \
	if(!MIX_IS_SURFACEPOOL(obj)) return MIX_RESULT_FAIL; \
 

MIX_RESULT
mix_surfacepool_dumpframe(MixVideoFrame *frame)
{
    LOG_I( "\tFrame %x, id %lu, refcount %d, ts %lu\n", (uint)frame,
           frame->frame_id, MIX_PARAMS(frame)->ref_count, (ulong) frame->timestamp);

    return MIX_RESULT_SUCCESS;
}

MIX_RESULT
mix_surfacepool_dumpprint (MixSurfacePool * obj)
{
    //TODO replace this with proper logging later

    LOG_I( "SURFACE POOL DUMP:\n");
    LOG_I( "Free list size is %d\n", obj->free_list_cur_size);
    LOG_I( "In use list size is %d\n", j_slist_length(obj->in_use_list));
    LOG_I( "High water mark is %lu\n", obj->high_water_mark);

    //Walk the free list and report the contents
    LOG_I( "Free list contents:\n");
    j_slist_foreach(obj->free_list, (JFunc) mix_surfacepool_dumpframe, NULL);

    //Walk the in_use list and report the contents
    LOG_I( "In Use list contents:\n");
    j_slist_foreach(obj->in_use_list, (JFunc) mix_surfacepool_dumpframe, NULL);

    return MIX_RESULT_SUCCESS;
}
