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

#ifndef VIDEO_DECODER_PAVC_H_
#define VIDEO_DECODER_PAVC_H_

#include "VideoDecoderAVC.h"


class VideoDecoderPAVC : public VideoDecoderAVC {
public:
    VideoDecoderPAVC(const char *mimeType);
    virtual ~VideoDecoderPAVC();

    // data in the decoded buffer only contains clearHeader and decrypted data.
    // encrypted data is not included in the buffer as it may contain start code emulation bytes.
    virtual Decode_Status decode(VideoDecodeBuffer *buffer);

private:
    virtual Decode_Status decodeSlice(vbp_data_h264 *data, uint32_t picIndex, uint32_t sliceIndex);

    // structure PAVCMetadata is appended after the VideodecodeBuffer::data + VideoDecoderBuffer::size
    // number of structures is equal to number of nal units in the buffer.
    struct PAVCMetadata
    {
        uint32_t clearHeaderSize;  // 0 means no more meta data
        uint32_t decryptionDataSize;
        uint32_t encryptionDataSize;
        uint32_t clearHeaderIMROffset;  // always points to clear header in the IMR
    };

private:
    uint8_t *mMetadata;  // pointer to metadata appended at end of buffer
};



#endif /* VIDEO_DECODER_PAVC_H_ */
