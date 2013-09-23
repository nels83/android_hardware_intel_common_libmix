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

#ifndef VIDEO_DECODER_WMV_H_
#define VIDEO_DECODER_WMV_H_

#include "VideoDecoderBase.h"


class VideoDecoderWMV : public VideoDecoderBase {
public:
    VideoDecoderWMV(const char *mimeType);
    virtual ~VideoDecoderWMV();

    virtual Decode_Status start(VideoConfigBuffer *buffer);
    virtual void stop(void);
    virtual void flush(void);
    virtual Decode_Status decode(VideoDecodeBuffer *buffer);

private:
    Decode_Status decodeFrame(VideoDecodeBuffer *buffer, vbp_data_vc1 *data);
    Decode_Status decodePicture(vbp_data_vc1 *data, int32_t picIndex);
    Decode_Status setReference(VAPictureParameterBufferVC1 *params, int32_t picIndex, VASurfaceID current);
    void updateDeblockedPicIndexes(int frameType);
    Decode_Status updateConfigData(uint8_t *configData, int32_t configDataLen, uint8_t **newConfigData, int32_t *newConfigDataLen);
    Decode_Status startVA(vbp_data_vc1 *data);
    void updateFormatInfo(vbp_data_vc1 *data);
    inline Decode_Status allocateVABufferIDs(int32_t number);
    Decode_Status parseBuffer(uint8_t *data, int32_t size, vbp_data_vc1 **vbpData);

private:
    enum {
        VC1_SURFACE_NUMBER = 10,
        VC1_EXTRA_SURFACE_NUMBER = 3,
    };

    VABufferID *mBufferIDs;
    int32_t mNumBufferIDs;
    bool mConfigDataParsed;
    bool mRangeMapped;

    int32_t mDeblockedCurrPicIndex;
    int32_t mDeblockedLastPicIndex;
    int32_t mDeblockedForwardPicIndex;
};



#endif /* VIDEO_DECODER_WMV_H_ */
