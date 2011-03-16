/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

/**
 * SECTION:mixbufferpool
 * @short_description: MI-X Input Buffer Pool
 *
 * A data object which stores and manipulates a pool of compressed video buffers.
 */

#include "mixvideolog.h"
#include "mixbufferpool.h"
#include "mixbuffer_private.h"

#define SAFE_FREE(p) if(p) { free(p); p = NULL; }

MixBufferPool::MixBufferPool()
        :free_list(NULL)
        ,in_use_list(NULL)
        ,free_list_max_size(0)
        ,high_water_mark(0)
        ,reserved1(NULL)
        ,reserved2(NULL)
        ,reserved3(NULL)
        ,reserved4(NULL)
        ,mLock() {
}

MixBufferPool::~MixBufferPool() {
}

MixBufferPool * mix_bufferpool_new(void) {
    return new MixBufferPool();
}

MixBufferPool * mix_bufferpool_ref(MixBufferPool * mix) {
    if (NULL != mix)
        mix->Ref();
    return mix;
}

MixParams * MixBufferPool::dup() const {
    MixBufferPool * ret = new MixBufferPool();
    MixBufferPool * this_obj = const_cast<MixBufferPool*>(this);
    if (NULL != ret) {
        this_obj->Lock();
        if (FALSE == copy(ret)) {
            ret->Unref();
            ret = NULL;
        }
        this_obj->Unlock();
    }
    return ret;
}


/**
 * mix_bufferpool_copy:
 * @target: copy to target
 * @src: copy from src
 * @returns: boolean indicates if copy is successful.
 *
 * Copy instance data from @src to @target.
 */

bool MixBufferPool::copy(MixParams * target) const {
    bool ret = FALSE;
    MixBufferPool * this_target = MIX_BUFFERPOOL(target);
    MixBufferPool * this_obj = const_cast<MixBufferPool*>(this);
    if (NULL != this_target) {
        this_obj->Lock();
        this_target->Lock();
        this_target->free_list = free_list;
        this_target->in_use_list = in_use_list;
        this_target->free_list_max_size = free_list_max_size;
        this_target->high_water_mark = high_water_mark;
        ret = MixParams::copy(target);
        this_target->Unlock();
        this_obj->Unlock();
    }
    return ret;
}

bool MixBufferPool::equal(MixParams * obj) const {
    bool ret = FALSE;
    MixBufferPool * this_obj = MIX_BUFFERPOOL(obj);
    MixBufferPool * unconst_this = const_cast<MixBufferPool*>(this);
    if (NULL != this_obj) {
        unconst_this->Lock();
        this_obj->Lock();
        if (free_list == this_obj->free_list &&
                in_use_list == this_obj->in_use_list &&
                free_list_max_size == this_obj->free_list_max_size &&
                high_water_mark == this_obj->high_water_mark) {
            ret = MixParams::equal(this_obj);
        }
        this_obj->Unlock();
        unconst_this->Unlock();
    }
    return ret;
}

/**
 * mix_bufferpool_initialize:
 * @returns: MIX_RESULT_SUCCESS if successful in creating the buffer pool
 *
 * Use this method to create a new buffer pool, consisting of a GSList of
 * buffer objects that represents a pool of buffers.
 */
MIX_RESULT mix_bufferpool_initialize(
    MixBufferPool * obj, uint num_buffers) {
    LOG_V( "Begin\n");

    if (obj == NULL)
        return MIX_RESULT_NULL_PTR;

    obj->Lock();

    if ((obj->free_list != NULL) || (obj->in_use_list != NULL)) {
        //buffer pool is in use; return error; need proper cleanup
        //TODO need cleanup here?

        obj->Unlock();

        return MIX_RESULT_ALREADY_INIT;
    }

    if (num_buffers == 0) {
        obj->free_list = NULL;

        obj->in_use_list = NULL;

        obj->free_list_max_size = num_buffers;

        obj->high_water_mark = 0;

        obj->Unlock();

        return MIX_RESULT_SUCCESS;
    }

    // Initialize the free pool with MixBuffer objects

    uint i = 0;
    MixBuffer *buffer = NULL;

    for (; i < num_buffers; i++) {

        buffer = mix_buffer_new();

        if (buffer == NULL) {
            //TODO need to log an error here and do cleanup

            obj->Unlock();

            return MIX_RESULT_NO_MEMORY;
        }

        // Set the pool reference in the private data of the MixBuffer object
        mix_buffer_set_pool(buffer, obj);

        //Add each MixBuffer object to the pool list
        obj->free_list = j_slist_append(obj->free_list, buffer);

    }

    obj->in_use_list = NULL;

    obj->free_list_max_size = num_buffers;

    obj->high_water_mark = 0;

    obj->Unlock();

    LOG_V( "End\n");

    return MIX_RESULT_SUCCESS;
}

/**
 * mix_bufferpool_put:
 * @returns: SUCCESS or FAILURE
 *
 * Use this method to return a buffer to the free pool
 */
MIX_RESULT mix_bufferpool_put(MixBufferPool * obj, MixBuffer * buffer) {

    if (obj == NULL || buffer == NULL)
        return MIX_RESULT_NULL_PTR;

    obj->Lock();

    if (obj->in_use_list == NULL) {
        //in use list cannot be empty if a buffer is in use
        //TODO need better error code for this

        obj->Unlock();

        return MIX_RESULT_FAIL;
    }

    JSList *element = j_slist_find(obj->in_use_list, buffer);
    if (element == NULL) {
        //Integrity error; buffer not found in in use list
        //TODO need better error code and handling for this

        obj->Unlock();

        return MIX_RESULT_FAIL;
    } else {
        //Remove this element from the in_use_list
        obj->in_use_list = j_slist_remove_link(obj->in_use_list, element);

        //Concat the element to the free_list
        obj->free_list = j_slist_concat(obj->free_list, element);
    }

    //Note that we do nothing with the ref count for this.  We want it to
    //stay at 1, which is what triggered it to be added back to the free list.

    obj->Unlock();

    return MIX_RESULT_SUCCESS;
}

/**
 * mix_bufferpool_get:
 * @returns: SUCCESS or FAILURE
 *
 * Use this method to get a buffer from the free pool
 */
MIX_RESULT mix_bufferpool_get(MixBufferPool * obj, MixBuffer ** buffer) {

    if (obj == NULL || buffer == NULL)
        return MIX_RESULT_NULL_PTR;

    obj->Lock();

    if (obj->free_list == NULL) {
        //We are out of buffers
        //TODO need to log this as well

        obj->Unlock();

        return MIX_RESULT_POOLEMPTY;
    }

    //Remove a buffer from the free pool

    //We just remove the one at the head, since it's convenient
    JSList *element = obj->free_list;
    obj->free_list = j_slist_remove_link(obj->free_list, element);
    if (element == NULL) {
        //Unexpected behavior
        //TODO need better error code and handling for this

        obj->Unlock();

        return MIX_RESULT_FAIL;
    } else {
        //Concat the element to the in_use_list
        obj->in_use_list = j_slist_concat(obj->in_use_list, element);

        //TODO replace with proper logging

        LOG_I( "buffer refcount%d\n",
               MIX_PARAMS(element->data)->ref_count);

        //Set the out buffer pointer
        *buffer = (MixBuffer *) element->data;

        //Check the high water mark for buffer use
        uint size = j_slist_length(obj->in_use_list);
        if (size > obj->high_water_mark)
            obj->high_water_mark = size;
        //TODO Log this high water mark
    }

    //Increment the reference count for the buffer
    mix_buffer_ref(*buffer);

    obj->Unlock();

    return MIX_RESULT_SUCCESS;
}

/**
 * mix_bufferpool_deinitialize:
 * @returns: SUCCESS or FAILURE
 *
 * Use this method to teardown a buffer pool
 */
MIX_RESULT mix_bufferpool_deinitialize(MixBufferPool * obj) {
    if (obj == NULL)
        return MIX_RESULT_NULL_PTR;

    obj->Lock();

    if ((obj->in_use_list != NULL) || (j_slist_length(obj->free_list)
                                       != obj->free_list_max_size)) {
        //TODO better error code
        //We have outstanding buffer objects in use and they need to be
        //freed before we can deinitialize.

        obj->Unlock();

        return MIX_RESULT_FAIL;
    }

    //Now remove buffer objects from the list

    MixBuffer *buffer = NULL;

    while (obj->free_list != NULL) {
        //Get the buffer object from the head of the list
        buffer = reinterpret_cast<MixBuffer*>(obj->free_list->data);
        //buffer = g_slist_nth_data(obj->free_list, 0);

        //Release it
        mix_buffer_unref(buffer);

        //Delete the head node of the list and store the new head
        obj->free_list = j_slist_delete_link(obj->free_list, obj->free_list);

        //Repeat until empty
    }

    obj->free_list_max_size = 0;

    //May want to log this information for tuning
    obj->high_water_mark = 0;

    obj->Unlock();

    return MIX_RESULT_SUCCESS;
}

#define MIX_BUFFERPOOL_SETTER_CHECK_INPUT(obj) \
	if(!obj) return MIX_RESULT_NULL_PTR; \
	if(!MIX_IS_BUFFERPOOL(obj)) return MIX_RESULT_FAIL; \
 
#define MIX_BUFFERPOOL_GETTER_CHECK_INPUT(obj, prop) \
	if(!obj || !prop) return MIX_RESULT_NULL_PTR; \
	if(!MIX_IS_BUFFERPOOL(obj)) return MIX_RESULT_FAIL; \
 

MIX_RESULT
mix_bufferpool_dumpbuffer(MixBuffer *buffer) {
    LOG_I( "\tBuffer %x, ptr %x, refcount %d\n", (uint)buffer,
           (uint)buffer->data, MIX_PARAMS(buffer)->ref_count);
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT
mix_bufferpool_dumpprint (MixBufferPool * obj) {
    //TODO replace this with proper logging later
    LOG_I( "BUFFER POOL DUMP:\n");
    LOG_I( "Free list size is %d\n", j_slist_length(obj->free_list));
    LOG_I( "In use list size is %d\n", j_slist_length(obj->in_use_list));
    LOG_I( "High water mark is %lu\n", obj->high_water_mark);

    //Walk the free list and report the contents
    LOG_I( "Free list contents:\n");
    j_slist_foreach(obj->free_list, (JFunc) mix_bufferpool_dumpbuffer, NULL);

    //Walk the in_use list and report the contents
    LOG_I( "In Use list contents:\n");
    j_slist_foreach(obj->in_use_list, (JFunc) mix_bufferpool_dumpbuffer, NULL);

    return MIX_RESULT_SUCCESS;
}

