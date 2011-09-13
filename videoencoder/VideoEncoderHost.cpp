/*
 INTEL CONFIDENTIAL
 Copyright 2011 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

#include "VideoEncoderMP4.h"
#include "VideoEncoderH263.h"
#include "VideoEncoderAVC.h"
#include "VideoEncoderHost.h"
#include "VideoEncoderLog.h"
#include <string.h>

IVideoEncoder *createVideoEncoder(const char *mimeType) {

    if (mimeType == NULL) {
        LOG_E("NULL mime type");
        return NULL;
    }

    if (strcasecmp(mimeType, "video/avc") == 0 ||
            strcasecmp(mimeType, "video/h264") == 0) {
        VideoEncoderAVC *p = new VideoEncoderAVC();
        return (IVideoEncoder *)p;
    } else if (strcasecmp(mimeType, "video/h263") == 0) {
        VideoEncoderH263 *p = new VideoEncoderH263();
        return (IVideoEncoder *)p;
    } else if (strcasecmp(mimeType, "video/mpeg4") == 0 ||
            strcasecmp(mimeType, "video/mp4v-es") == 0) {
        VideoEncoderMP4 *p = new VideoEncoderMP4();
        return (IVideoEncoder *)p;
    } else {
        LOG_E ("Unknown mime type: %s", mimeType);
    }
    return NULL;
}

void releaseVideoEncoder(IVideoEncoder *p) {
    if (p) delete p;
}

