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


#ifndef VA_VIDEO_DECODER_H_
#define VA_VIDEO_DECODER_H_

#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaSource.h>
#include <utils/Vector.h>

class IVideoDecoder;

namespace android {

struct VAVideoDecoder : public MediaSource,
                        public MediaBufferObserver {
    VAVideoDecoder(const sp<MediaSource> &source);

    virtual status_t start(MetaData *params);
    virtual status_t stop();
    virtual sp<MetaData> getFormat();
    virtual status_t read(MediaBuffer **buffer, const ReadOptions *options);
    virtual void signalBufferReturned(MediaBuffer* buffer) {}

protected:
    virtual ~VAVideoDecoder();

private:
    MediaBuffer *getOutputBuffer(bool bDraining = false);
    VAVideoDecoder(const VAVideoDecoder &);
    VAVideoDecoder &operator=(const VAVideoDecoder &);

private:
    enum {
        NUM_OF_MEDIA_BUFFER = 20,
    };
    // The maximum input size is set to be size of one 1080P 4:4:4 image.
    // It is calculated as 1920x1080x3 = 6220800 bytes.
   enum {
        MAXINPUTSIZE = 6220800,
   };
    sp<MediaSource> mSource;
    bool mStarted;
    bool mRawOutput;
    sp<MetaData> mFormat;
    Vector<MediaBuffer *> mFrames;
    MediaBuffer *mInputBuffer;
    int64_t mTargetTimeUs;
    uint32_t mFrameIndex;
    uint32_t mErrorCount;
    IVideoDecoder *mDecoder;
};

} // namespace android

#endif  // VA_VIDEO_DECODER_H_
