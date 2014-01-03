/*
 INTEL CONFIDENTIAL
 Copyright 2011 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

#ifndef __VIDEO_ENCODER_VP8_H__
#define __VIDEO_ENCODER_VP8_H__

#include "VideoEncoderBase.h"

/**
  * VP8 Encoder class, derived from VideoEncoderBase
  */
class VideoEncoderVP8: public VideoEncoderBase {
public:
    VideoEncoderVP8();
    virtual ~VideoEncoderVP8();



protected:
    virtual Encode_Status sendEncodeCommand(EncodeTask *task);
    virtual Encode_Status derivedSetParams(VideoParamConfigSet *videoEncParams);
    virtual Encode_Status derivedGetParams(VideoParamConfigSet *videoEncParams);
    virtual Encode_Status derivedGetConfig(VideoParamConfigSet *videoEncConfig);
    virtual Encode_Status derivedSetConfig(VideoParamConfigSet *videoEncConfig);
    virtual Encode_Status getExtFormatOutput(VideoEncOutputBuffer *outBuffer) {
        return ENCODE_NOT_SUPPORTED;
    }

    // Local Methods
private:
	Encode_Status renderSequenceParams();
	Encode_Status renderPictureParams(EncodeTask *task);
	Encode_Status renderRCParams(void);
	Encode_Status renderHRDParams(void);
	Encode_Status renderFrameRateParams(void);
	Encode_Status renderMaxFrameSizeParams(void);


	VideoConfigVP8 mVideoConfigVP8;
	VideoParamsVP8 mVideoParamsVP8;
	VideoConfigVP8ReferenceFrame mVideoConfigVP8ReferenceFrame;
};

#endif /* __VIDEO_ENCODER_VP8_H__ */
