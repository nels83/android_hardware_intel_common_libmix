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

#ifndef VIDEO_DECODER_MPEG4_H_
#define VIDEO_DECODER_MPEG4_H_

#include "VideoDecoderBase.h"


class VideoDecoderMPEG4 : public VideoDecoderBase {
public:
    VideoDecoderMPEG4(const char *mimeType);
    virtual ~VideoDecoderMPEG4();

    virtual Decode_Status start(VideoConfigBuffer *buffer);
    virtual void stop(void);
    virtual void flush(void);
    virtual Decode_Status decode(VideoDecodeBuffer *buffer);

private:
    Decode_Status decodeFrame(VideoDecodeBuffer *buffer, vbp_data_mp42 *data);
    Decode_Status beginDecodingFrame(vbp_data_mp42 *data);
    Decode_Status continueDecodingFrame(vbp_data_mp42 *data);
    Decode_Status decodeSlice(vbp_data_mp42 *data, vbp_picture_data_mp42 *picData);
    Decode_Status setReference(VAPictureParameterBufferMPEG4 *picParam);
    Decode_Status startVA(vbp_data_mp42 *data);
    void updateFormatInfo(vbp_data_mp42 *data);

private:
    // Value of VOP type defined here follows MP4 spec
    enum {
        MP4_VOP_TYPE_I = 0,
        MP4_VOP_TYPE_P = 1,
        MP4_VOP_TYPE_B = 2,
        MP4_VOP_TYPE_S = 3,
    };

    enum {
        MP4_SURFACE_NUMBER = 10,
    };

    uint64_t mLastVOPTimeIncrement;
    bool mExpectingNVOP; // indicate if future n-vop is a placeholder of a packed frame
    bool mSendIQMatrixBuf; // indicate if iq_matrix_buffer is sent to driver
    int32_t mLastVOPCodingType;
    bool mIsSyncFrame; // indicate if it is SyncFrame in container
    VideoExtensionBuffer mExtensionBuffer;
    PackedFrameData mPackedFrame;
};



#endif /* VIDEO_DECODER_MPEG4_H_ */
