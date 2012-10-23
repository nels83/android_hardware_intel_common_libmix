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


#ifndef MEDIA_BUFFER_POOL_H_

#define MEDIA_BUFFER_POOL_H_

#include <media/stagefright/MediaBuffer.h>
#include <utils/Errors.h>
#include <utils/threads.h>

namespace android {

class MediaBuffer;
class MetaData;

class MediaBufferPool : public MediaBufferObserver {
public:
    MediaBufferPool();
    ~MediaBufferPool();

    status_t acquire_buffer(int size, MediaBuffer **buffer);

protected:
    virtual void signalBufferReturned(MediaBuffer *buffer);

private:
    friend class MediaBuffer;

    Mutex mLock;
    int mMaxBufferSize;

    MediaBuffer *mFirstBuffer, *mLastBuffer;

    MediaBufferPool(const MediaBufferPool &);
    MediaBufferPool &operator=(const MediaBufferPool &);
};

}  // namespace android

#endif  // MEDIA_BUFFER_POOL_H_
