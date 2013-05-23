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

#ifndef VIDEO_DECODER_AVC_SECURE_H_
#define VIDEO_DECODER_AVC_SECURE_H_

#include "VideoDecoderAVC.h"


class VideoDecoderAVCSecure : public VideoDecoderAVC {
public:
    VideoDecoderAVCSecure(const char *mimeType);
    virtual ~VideoDecoderAVCSecure();

    virtual Decode_Status start(VideoConfigBuffer *buffer);
    virtual void stop(void);

    // data in the decoded buffer is all encrypted.
    virtual Decode_Status decode(VideoDecodeBuffer *buffer);

private:
    enum {
        MAX_SLICE_HEADER_SIZE  = 30,
        MAX_NALU_HEADER_BUFFER = 8192,
        MAX_NALU_NUMBER = 400,  // > 4096/12
    };

    // Information of Network Abstraction Layer Unit
    struct NaluInfo {
        int32_t naluOffset;                        // offset of NAL unit in the firewalled buffer
        int32_t naluLen;                           // length of NAL unit
        int32_t naluHeaderLen;                     // length of NAL unit header
    };

    struct NaluMetadata {
        NaluInfo *naluInfo;
        int32_t naluNumber;  // number of NAL units
    };

    struct NaluByteStream {
        int32_t naluOffset;
        int32_t naluLen;
        int32_t streamPos;
        uint8_t *byteStream;   // 4 bytes of naluCount, 4 bytes of naluOffset, 4 bytes of naulLen, 4 bytes of naluHeaderLen, followed by naluHeaderData
        int32_t naluCount;
    };

    virtual Decode_Status decodeSlice(vbp_data_h264 *data, uint32_t picIndex, uint32_t sliceIndex);
    int32_t findNalUnitOffset(uint8_t *stream, int32_t offset, int32_t length);
    Decode_Status copyNaluHeader(uint8_t *stream, NaluByteStream *naluStream);
    Decode_Status parseAnnexBStream(uint8_t *stream, int32_t length, NaluByteStream *naluStream);

private:
    NaluMetadata mMetadata;
    NaluByteStream mByteStream;
    uint8_t *mNaluHeaderBuffer;
    uint8_t *mInputBuffer;
};



#endif /* VIDEO_DECODER_AVC_SECURE_H_ */
