/*
 INTEL CONFIDENTIAL
 Copyright 2011 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

#include "VideoEncoderMP4.h"
#include "VideoEncoderH263.h"
#include "VideoEncoderAVC.h"
#include "VideoEncoderVP8.h"
#ifndef IMG_GFX
#include "PVSoftMPEG4Encoder.h"
#endif
#include "VideoEncoderHost.h"
#include <string.h>
#include <cutils/properties.h>
#include <wrs_omxil_core/log.h>

int32_t gLogLevel = 0;

IVideoEncoder *createVideoEncoder(const char *mimeType) {

    char logLevelProp[PROPERTY_VALUE_MAX];

    if (property_get("libmix.debug", logLevelProp, NULL)) {
        gLogLevel = atoi(logLevelProp);
        LOGD("Debug level is %d", gLogLevel);
    }

    if (mimeType == NULL) {
        LOGE("NULL mime type");
        return NULL;
    }

    if (strcasecmp(mimeType, "video/avc") == 0 ||
            strcasecmp(mimeType, "video/h264") == 0) {
        VideoEncoderAVC *p = new VideoEncoderAVC();
        return (IVideoEncoder *)p;
    } else if (strcasecmp(mimeType, "video/h263") == 0) {
#ifdef IMG_GFX
        VideoEncoderH263 *p = new VideoEncoderH263();
#else
        PVSoftMPEG4Encoder *p = new PVSoftMPEG4Encoder("OMX.google.h263.encoder");
#endif
        return (IVideoEncoder *)p;
    } else if (strcasecmp(mimeType, "video/mpeg4") == 0 ||
            strcasecmp(mimeType, "video/mp4v-es") == 0) {
#ifdef IMG_GFX
        VideoEncoderMP4 *p = new VideoEncoderMP4();
#else
        PVSoftMPEG4Encoder *p = new PVSoftMPEG4Encoder("OMX.google.mpeg4.encoder");
#endif
        return (IVideoEncoder *)p;
    } else if (strcasecmp(mimeType, "video/x-vnd.on2.vp8") == 0) {
        VideoEncoderVP8 *p = new VideoEncoderVP8();
        return (IVideoEncoder *)p;
    } else {
        LOGE ("Unknown mime type: %s", mimeType);
    }
    return NULL;
}

void releaseVideoEncoder(IVideoEncoder *p) {
    if (p) delete p;
}

