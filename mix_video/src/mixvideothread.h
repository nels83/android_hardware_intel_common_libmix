/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

#ifndef __MIX_VIDEO_THREAD_H__
#define __MIX_VIDEO_THREAD_H__

#include <pthread.h>


class MixVideoMutex {
public:
    enum {
        PRIVATE = 0,
        SHARED = 1
    };

    MixVideoMutex();
    MixVideoMutex(const char* name);
    MixVideoMutex(int type, const char* name = NULL);
    ~MixVideoMutex();

    // lock or unlock the mutex
    int lock();
    void unlock();
    // lock if possible; returns 0 on success, error otherwise
    int tryLock();
private:
    // A mutex cannot be copied
    MixVideoMutex(const MixVideoMutex&);
    MixVideoMutex& operator = (const MixVideoMutex&);

private:
    pthread_mutex_t mMutex;
};




#endif /*  __MIX_VIDEO_THREAD_H__ */

