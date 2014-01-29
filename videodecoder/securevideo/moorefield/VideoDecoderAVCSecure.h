/* INTEL CONFIDENTIAL
* Copyright (c) 2009-2011 Intel Corporation.  All rights reserved.
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

#ifndef VIDEO_DECODER_AVC_SECURE_H
#define VIDEO_DECODER_AVC_SECURE_H

#include "VideoDecoderBase.h"
#include "VideoDecoderAVC.h"
#include "VideoDecoderDefs.h"

class VideoDecoderAVCSecure : public VideoDecoderAVC {
public:
    VideoDecoderAVCSecure(const char *mimeType);
    virtual Decode_Status start(VideoConfigBuffer *buffer);
    virtual void stop(void);

    // data in the decoded buffer is all encrypted.
    virtual Decode_Status decode(VideoDecodeBuffer *buffer);
protected:
    virtual Decode_Status decodeFrame(VideoDecodeBuffer *buffer, vbp_data_h264 *data);
    virtual Decode_Status continueDecodingFrame(vbp_data_h264 *data);
    virtual Decode_Status beginDecodingFrame(vbp_data_h264 *data);
    Decode_Status parseSliceHeader(VideoDecodeBuffer *buffer, vbp_data_h264 *data);
    Decode_Status updateSliceParameter(vbp_data_h264 *data, void *sliceheaderbuf);
    virtual Decode_Status decodeSlice(vbp_data_h264 *data, uint32_t picIndex, uint32_t sliceIndex);
private:
    int32_t     mIsEncryptData;
    int32_t     mFrameSize;
    uint8_t*    mFrameData;
    uint8_t*    mClearData;
    int32_t     mFrameIdx;
};

#endif
