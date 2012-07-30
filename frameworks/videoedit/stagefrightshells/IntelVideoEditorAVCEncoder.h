/*
 * INTEL CONFIDENTIAL
 * Copyright 2010-2011 Intel Corporation All Rights Reserved.

 * The source code, information and material ("Material") contained herein is owned
 * by Intel Corporation or its suppliers or licensors, and title to such Material
 * remains with Intel Corporation or its suppliers or licensors. The Material contains
 * proprietary information of Intel or its suppliers and licensors. The Material is
 * protected by worldwide copyright laws and treaty provisions. No part of the Material
 * may be used, copied, reproduced, modified, published, uploaded, posted, transmitted,
 * distributed or disclosed in any way without Intel's prior express written permission.
 * No license under any patent, copyright or other intellectual property rights in the
 * Material is granted to or conferred upon you, either expressly, by implication, inducement,
 * estoppel or otherwise. Any license under such intellectual property rights must be express
 * and approved by Intel in writing.

 * Unless otherwise agreed by Intel in writing, you may not remove or alter this notice or any
 * other notice embedded in Materials by Intel or Intel's suppliers or licensors in any way.
 */

#ifndef INTELVIDEOEDITORAVCENCODER_H
#define INTELVIDEOEDITORAVCENCODER_H

#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaBufferGroup.h>
#include <media/stagefright/MediaSource.h>
#include <utils/Vector.h>
#include "va/va.h"
#include "VideoEncoderHost.h"
#include <IntelBufferSharing.h>

namespace android {
struct IntelVideoEditorAVCEncoder :  public MediaSource {
    IntelVideoEditorAVCEncoder(const sp<MediaSource> &source,
            const sp<MetaData>& meta);

    virtual status_t start(MetaData *params);
    virtual status_t stop();

    virtual sp<MetaData> getFormat();

    virtual status_t read(MediaBuffer **buffer, const ReadOptions *options);


protected:
    virtual ~IntelVideoEditorAVCEncoder();

private:
    sp<MediaSource> mSource;
    sp<MetaData>    mMeta;

    int32_t  mVideoWidth;
    int32_t  mVideoHeight;
    int32_t  mFrameSize;
    int32_t  mVideoFrameRate;
    int32_t  mVideoBitRate;
    int32_t  mVideoColorFormat;
    int32_t  mUseSyncMode;
    status_t mInitCheck;
    bool     mStarted;
    bool     mFirstFrame;
    int32_t  mFrameCount;
    static const int OUTPUT_BUFFERS = 6;
    static const int INPUT_SHARED_BUFFERS = 8;
    IVideoEncoder         *mVAEncoder;
    VideoParamsCommon     mEncParamsCommon;
    VideoParamsAVC        mEncParamsH264;
    SharedBufferType      mSharedBufs[INPUT_SHARED_BUFFERS];
    const ReadOptions     *mReadOptions;
    MediaBufferGroup      *mOutBufGroup;   /* group of output buffers*/
    MediaBuffer           *mLastInputBuffer;

private:
    status_t initCheck(const sp<MetaData>& meta);
    int32_t calcBitrate(int width, int height);
    status_t getSharedBuffers();
    status_t setSharedBuffers();
    static int SBShutdownFunc(void* arg);

    IntelVideoEditorAVCEncoder(const IntelVideoEditorAVCEncoder &);
    IntelVideoEditorAVCEncoder &operator=(const IntelVideoEditorAVCEncoder &);
};
};
#endif

