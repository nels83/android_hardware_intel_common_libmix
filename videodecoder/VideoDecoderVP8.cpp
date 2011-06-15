/* INTEL CONFIDENTIAL
* Copyright (c) 2009 Intel Corporation.  All rights reserved.
*
* The source code contained or described herein and all documents
* related to the source code ("Material") are owned by Intel
* Corporation or its suppliers or licensors.  Title to the
* Material remains with Intel Corporation or its suppliers and
* licensors.  The Material contains trade secrets and proprietary
* and confidential information of Intel or its suppliers and
* licensors. The Material is protected by worldwide copyright and
* trade secret laws and treaty provisions.  No part of the Material
* may be used, copied, reproduced, modified, published, uploaded,
* posted, transmitted, distributed, or disclosed in any way without
* Intel's prior express written permission.
*
* No license under any patent, copyright, trade secret or other
* intellectual property right is granted to or conferred upon you
* by disclosure or delivery of the Materials, either expressly, by
* implication, inducement, estoppel or otherwise. Any license
* under such intellectual property rights must be express and
* approved by Intel in writing.
*
*/

#include "VideoDecoderVP8.h"
#include "VideoDecoderTrace.h"
#include <string.h>

VideoDecoderVP8::VideoDecoderVP8(const char *mimeType)
    : VideoDecoderBase(mimeType, (_vbp_parser_type)VBP_INVALID) {
}

VideoDecoderVP8::~VideoDecoderVP8() {
    stop();
}

Decode_Status VideoDecoderVP8::start(VideoConfigBuffer *buffer) {
    Decode_Status status;

    status = VideoDecoderBase::start(buffer);
    CHECK_STATUS("VideoDecoderBase::start");

    // config VP8 software decoder if necessary
    // TODO: update mVideoFormatInfo here

    status = VideoDecoderBase::setupVA(
            VP8_SURFACE_NUMBER,
            (VAProfile)VAProfileSoftwareDecoding);

    return status;
}

void VideoDecoderVP8::stop(void) {
    VideoDecoderBase::stop();
}

void VideoDecoderVP8::flush(void) {
    VideoDecoderBase::flush();
}

Decode_Status VideoDecoderVP8::decode(VideoDecodeBuffer *buffer) {
    Decode_Status status;

    status = acquireSurfaceBuffer();
    CHECK_STATUS("acquireSurfaceBuffer");

    // TODO: decode sample to mAcquiredBuffer->mappedAddr.
    // make sure decoded output is in NV12 format.
    // << add decoding codes here>>


    // set referenceFrame to true if frame decoded is I/P frame, false otherwise.
    mAcquiredBuffer->referenceFrame = true;
    // assume it is frame picture.
    mAcquiredBuffer->renderBuffer.scanFormat = VA_FRAME_PICTURE;
    mAcquiredBuffer->renderBuffer.timeStamp = buffer->timeStamp;
    mAcquiredBuffer->renderBuffer.flag = 0;

    // if sample is successfully decoded, call outputSurfaceBuffer(); otherwise
    // call releaseSurfacebuffer();
    status = outputSurfaceBuffer();
    return status;
}


