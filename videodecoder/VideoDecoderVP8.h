/* INTEL CONFIDENTIAL
* Copyright (c) 2012 Intel Corporation.  All rights reserved.
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

#ifndef VIDEO_DECODER_VP8_H_
#define VIDEO_DECODER_VP8_H_

#include "VideoDecoderBase.h"


class VideoDecoderVP8 : public VideoDecoderBase {
public:
    VideoDecoderVP8(const char *mimeType);
    virtual ~VideoDecoderVP8();

    virtual Decode_Status start(VideoConfigBuffer *buffer);
    virtual void stop(void);
    virtual void flush(void);
    virtual Decode_Status decode(VideoDecodeBuffer *buffer);

private:
    Decode_Status decodeFrame(VideoDecodeBuffer* buffer, vbp_data_vp8 *data);
    Decode_Status decodePicture(vbp_data_vp8 *data, int32_t picIndex);
    Decode_Status setReference(VAPictureParameterBufferVP8 *picParam, int32_t picIndex);
    Decode_Status startVA(vbp_data_vp8 *data);
    void updateReferenceFrames(vbp_data_vp8 *data);
    void refreshLastReference(vbp_data_vp8 *data);
    void refreshGoldenReference(vbp_data_vp8 *data);
    void refreshAltReference(vbp_data_vp8 *data);
    void updateFormatInfo(vbp_data_vp8 *data);
    void invalidateReferenceFrames(int toggle);

private:
    enum {
        VP8_SURFACE_NUMBER = 9,
        VP8_REF_SIZE = 3,
    };

    enum {
        VP8_KEY_FRAME = 0,
        VP8_INTER_FRAME,
        VP8_SKIPPED_FRAME,
    };

    enum {
        VP8_LAST_REF_PIC = 0,
        VP8_GOLDEN_REF_PIC,
        VP8_ALT_REF_PIC,
    };

    enum {
        BufferCopied_NoneToGolden   = 0,
        BufferCopied_LastToGolden   = 1,
        BufferCopied_AltRefToGolden = 2
    };

    enum {
        BufferCopied_NoneToAltRef   = 0,
        BufferCopied_LastToAltRef   = 1,
        BufferCopied_GoldenToAltRef = 2
    };

    struct ReferenceFrameBuffer {
        VideoSurfaceBuffer *surfaceBuffer;
        int32_t index;
    };

    //[2] : [0 for current each reference frame, 1 for the previous each reference frame]
    //[VP8_REF_SIZE] : [0 for last ref pic, 1 for golden ref pic, 2 for alt ref pic]
    ReferenceFrameBuffer mRFBs[2][VP8_REF_SIZE];
};



#endif /* VIDEO_DECODER_VP8_H_ */
