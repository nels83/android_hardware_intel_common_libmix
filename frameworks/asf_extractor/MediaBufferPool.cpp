/*************************************************************************************
 * INTEL CONFIDENTIAL
 * Copyright 2011 Intel Corporation All Rights Reserved.
 * The source code contained or described herein and all documents related
 * to the source code ("Material") are owned by Intel Corporation or its
 * suppliers or licensors. Title to the Material remains with Intel
 * Corporation or its suppliers and licensors. The Material contains trade
 * secrets and proprietary and confidential information of Intel or its
 * suppliers and licensors. The Material is protected by worldwide copyright
 * and trade secret laws and treaty provisions. No part of the Material may
 * be used, copied, reproduced, modified, published, uploaded, posted,
 * transmitted, distributed, or disclosed in any way without Intel's prior
 * express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery
 * of the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be express
 * and approved by Intel in writing.
 ************************************************************************************/
/*
* Copyright (C) 2011 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/


#define LOG_TAG "MediaBufferPool"
#include <utils/Log.h>

#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/foundation/ADebug.h>
#include "MediaBufferPool.h"

#define DEFAULT_PAGE_SIZE 4096

namespace android {

MediaBufferPool::MediaBufferPool()
    : mMaxBufferSize(0),
      mFirstBuffer(NULL),
      mLastBuffer(NULL) {
}

MediaBufferPool::~MediaBufferPool() {
    MediaBuffer *next;
    for (MediaBuffer *buffer = mFirstBuffer; buffer != NULL;
         buffer = next) {
        next = buffer->nextBuffer();

        CHECK_EQ(buffer->refcount(), 0);

        buffer->setObserver(NULL);
        buffer->release();
    }
}

status_t MediaBufferPool::acquire_buffer(int size, MediaBuffer **out) {
    Mutex::Autolock autoLock(mLock);

    MediaBuffer *next = NULL;
    while (mFirstBuffer) {
        if ((int)mFirstBuffer->size() >= size) {
            next = mFirstBuffer->nextBuffer();

            // pop first buffer out of list
            *out = mFirstBuffer;
            mFirstBuffer->add_ref();
            mFirstBuffer->reset();

            mFirstBuffer = next;
            if (mFirstBuffer == NULL) {
                mLastBuffer = NULL;
            }
            return OK;
        } else {
            // delete the first buffer from the list
            next = mFirstBuffer->nextBuffer();
            mFirstBuffer->setObserver(NULL);
            mFirstBuffer->release();
            mFirstBuffer = next;
        }
    }

    // not a single buffer matches the requirement. Allocating a new buffer.

    mFirstBuffer = NULL;
    mLastBuffer = NULL;

    size = ((size + DEFAULT_PAGE_SIZE - 1)/DEFAULT_PAGE_SIZE) * DEFAULT_PAGE_SIZE;
    if (size < mMaxBufferSize) {
        size = mMaxBufferSize;
    } else {
        mMaxBufferSize = size;
    }
    MediaBuffer *p = new MediaBuffer(size);
    *out = p;
    return (p != NULL) ? OK : NO_MEMORY;
}

void MediaBufferPool::signalBufferReturned(MediaBuffer *buffer) {
    Mutex::Autolock autoLock(mLock);

    buffer->setObserver(this);

    if (mLastBuffer) {
        mLastBuffer->setNextBuffer(buffer);
    } else {
        mFirstBuffer = buffer;
    }

    mLastBuffer = buffer;
}

}  // namespace android
