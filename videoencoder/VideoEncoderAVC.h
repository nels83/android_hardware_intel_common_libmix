/*
 INTEL CONFIDENTIAL
 Copyright 2011 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

#ifndef __VIDEO_ENCODER_AVC_H__
#define __VIDEO_ENCODER_AVC_H__

#include "VideoEncoderBase.h"

class VideoEncoderAVC : public VideoEncoderBase {

public:
    VideoEncoderAVC();
    ~VideoEncoderAVC() {};

    virtual Encode_Status start();

    virtual Encode_Status derivedSetParams(VideoParamConfigSet *videoEncParams);
    virtual Encode_Status derivedGetParams(VideoParamConfigSet *videoEncParams);
    virtual Encode_Status derivedGetConfig(VideoParamConfigSet *videoEncConfig);
    virtual Encode_Status derivedSetConfig(VideoParamConfigSet *videoEncConfig);

protected:

    virtual Encode_Status sendEncodeCommand(EncodeTask *task);
    virtual Encode_Status getExtFormatOutput(VideoEncOutputBuffer *outBuffer);
    virtual Encode_Status updateFrameInfo(EncodeTask* task);
private:
    // Local Methods

    Encode_Status getOneNALUnit(uint8_t *inBuffer, uint32_t bufSize, uint32_t *nalSize, uint32_t *nalType, uint32_t *nalOffset, uint32_t status);
    Encode_Status getHeader(uint8_t *inBuffer, uint32_t bufSize, uint32_t *headerSize, uint32_t status);
    Encode_Status outputCodecData(VideoEncOutputBuffer *outBuffer);
    Encode_Status outputOneNALU(VideoEncOutputBuffer *outBuffer, bool startCode);
    Encode_Status outputLengthPrefixed(VideoEncOutputBuffer *outBuffer);

    Encode_Status renderMaxSliceSize();
    Encode_Status renderAIR();
    Encode_Status renderSequenceParams(EncodeTask *task);
    Encode_Status renderPictureParams(EncodeTask *task);
    Encode_Status renderSliceParams(EncodeTask *task);
    int calcLevel(int numMbs);

public:

    VideoParamsAVC mVideoParamsAVC;
    uint32_t mSliceNum;

};

#endif /* __VIDEO_ENCODER_AVC_H__ */
