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

#ifndef VIDEO_DECODER_AVC_H_
#define VIDEO_DECODER_AVC_H_

#include "VideoDecoderBase.h"


class VideoDecoderAVC : public VideoDecoderBase {
public:
    VideoDecoderAVC(const char *mimeType);
    virtual ~VideoDecoderAVC();

    virtual Decode_Status start(VideoConfigBuffer *buffer);
    virtual void stop(void);
    virtual void flush(void);
    virtual Decode_Status decode(VideoDecodeBuffer *buffer);

protected:
    Decode_Status decodeFrame(VideoDecodeBuffer *buffer, vbp_data_h264 *data);
    Decode_Status beginDecodingFrame(vbp_data_h264 *data);
    Decode_Status continueDecodingFrame(vbp_data_h264 *data);
    virtual Decode_Status decodeSlice(vbp_data_h264 *data, uint32_t picIndex, uint32_t sliceIndex);
    Decode_Status setReference(VASliceParameterBufferH264 *sliceParam);
    Decode_Status updateDPB(VAPictureParameterBufferH264 *picParam);
    Decode_Status updateReferenceFrames(vbp_picture_data_h264 *picData);
    void removeReferenceFromDPB(VAPictureParameterBufferH264 *picParam);
    inline uint32_t getPOC(VAPictureH264 *pic); // Picture Order Count
    inline VASurfaceID findSurface(VAPictureH264 *pic);
    inline VideoSurfaceBuffer* findSurfaceBuffer(VAPictureH264 *pic);
    inline void invalidateDPB(int toggle);
    inline void clearAsReference(int toggle);
    Decode_Status startVA(vbp_data_h264 *data);
    void updateFormatInfo(vbp_data_h264 *data);
    Decode_Status handleNewSequence(vbp_data_h264 *data);
    bool isNewFrame(vbp_data_h264 *data, bool equalPTS);
    int32_t getDPBSize(vbp_data_h264 *data);

private:
    struct DecodedPictureBuffer {
        VideoSurfaceBuffer *surfaceBuffer;
        uint32_t poc; // Picture Order Count
    };

    enum {
        AVC_EXTRA_SURFACE_NUMBER = 11,
        // maximum DPB (Decoded Picture Buffer) size
        MAX_REF_NUMBER = 16,
        DPB_SIZE = 17,         // DPB_SIZE = MAX_REF_NUMBER + 1,
        REF_LIST_SIZE = 32,
    };

    // maintain 2 ping-pong decoded picture buffers
    DecodedPictureBuffer mDPBs[2][DPB_SIZE];
    uint8_t mToggleDPB; // 0 or 1
    bool mErrorConcealment;
    uint32_t mLastPictureFlags;
    VideoExtensionBuffer mExtensionBuffer;
    PackedFrameData mPackedFrame;
};



#endif /* VIDEO_DECODER_AVC_H_ */
