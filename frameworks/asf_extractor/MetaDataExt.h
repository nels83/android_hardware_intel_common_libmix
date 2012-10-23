/*************************************************************************************
 * INTEL CONFIDENTIAL
 * Copyright 2011 Intel Corporation All Rights Reserved.
 * The source code contained or described herein and all documents related
 * to the source code ("Material") are owned by Intel Corporation or its
 * suppliers or licensors. Title to the Material remains with Intel
 * Corporation or its suppliers and licensors. The Material contains trade
 * secrets and proprietary and confidential information of Intel or its
 * suppliers and licensors. The Material is protected by worldwide copyright
 * and trade secret laws and treaty provisions. No part of the Material may
 * be used, copied, reproduced, modified, published, uploaded, posted,
 * transmitted, distributed, or disclosed in any way without Intel's prior
 * express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery
 * of the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be express
 * and approved by Intel in writing.
 ************************************************************************************/
/*
* Copyright (C) 2011 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/


#ifndef META_DATA_EXT_H_
#define META_DATA_EXT_H_

#include <media/stagefright/MetaData.h>

namespace android {

#define MEDIA_MIMETYPE_AUDIO_WMA        "audio/x-ms-wma"
#define MEDIA_MIMETYPE_AUDIO_AC3        "audio/ac3"
#define MEDIA_MIMETYPE_VIDEO_WMV        "video/wmv"
#define MEDIA_MIMETYPE_CONTAINER_ASF    "video/x-ms-asf"
#define MEDIA_MIMETYPE_VIDEO_VA         "video/x-va"
#define MEDIA_MIMETYPE_AUDIO_WMA_VOICE  "audio/wma-voice"


enum
{
    // value by default takes int32_t unless specified
    kKeyConfigData              = 'kcfg',  // raw data
    kKeyProtected               = 'prot',  // int32_t (bool)
    kKeyCropLeft                = 'clft',
    kKeyCropRight               = 'crit',
    kKeyCropTop                 = 'ctop',
    kKeyCropBottom              = 'cbtm',
    kKeySuggestedBufferSize     = 'sgbz',
    kKeyWantRawOutput           = 'rawo'
};

enum
{
    kTypeConfigData             = 'tcfg',
};

}  // namespace android

#endif  // META_DATA_EXT_H_
