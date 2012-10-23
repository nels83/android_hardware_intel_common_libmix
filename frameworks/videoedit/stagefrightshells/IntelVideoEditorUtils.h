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
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
*************************************************************************
* @file   VideoEditorUtils.cpp
* @brief  StageFright shell Utilities
*************************************************************************
*/
#ifndef ANDROID_UTILS_H_
#define ANDROID_UTILS_H_

/*******************
 *     HEADERS     *
 *******************/

#include "M4OSA_Debug.h"

#include "utils/Log.h"
#include <utils/RefBase.h>
#include <utils/threads.h>
#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MetaData.h>

/**
 *************************************************************************
 * VIDEOEDITOR_CHECK(test, errCode)
 * @note This macro displays an error message and goes to function cleanUp label
 *       if the test fails.
 *************************************************************************
 */
#define VIDEOEDITOR_CHECK(test, errCode) \
{ \
    if( !(test) ) { \
        ALOGW("!!! %s (L%d) check failed : " #test ", yields error 0x%.8x", \
            __FILE__, __LINE__, errCode); \
        err = (errCode); \
        goto cleanUp; \
    } \
}

/**
 *************************************************************************
 * SAFE_FREE(p)
 * @note This macro calls free and makes sure the pointer is set to NULL.
 *************************************************************************
 */
#define SAFE_FREE(p) \
{ \
    if(M4OSA_NULL != (p)) { \
        free((p)) ; \
        (p) = M4OSA_NULL ; \
    } \
}

/**
 *************************************************************************
 * SAFE_MALLOC(p, type, count, comment)
 * @note This macro allocates a buffer, checks for success and fills the buffer
 *       with 0.
 *************************************************************************
 */
#define SAFE_MALLOC(p, type, count, comment) \
{ \
    (p) = (type*)M4OSA_32bitAlignedMalloc(sizeof(type)*(count), 0xFF,(M4OSA_Char*)comment);\
    VIDEOEDITOR_CHECK(M4OSA_NULL != (p), M4ERR_ALLOC); \
    memset((void *)(p), 0,sizeof(type)*(count)); \
}


    /********************
     *    UTILITIES     *
     ********************/


namespace android {

/*--------------------------*/
/* DISPLAY METADATA CONTENT */
/*--------------------------*/
void displayMetaData(const sp<MetaData> meta);

// Build the AVC codec spcific info from the StageFright encoders output
status_t buildAVCCodecSpecificData(uint8_t **outputData, size_t *outputSize,
        const uint8_t *data, size_t size, MetaData *param);

// Remove the AVC codec specific info from the StageFright encoders output
status_t removeAVCCodecSpecificData(uint8_t **outputData, size_t *outputSize,
        const uint8_t *data, size_t size, MetaData *param);
}//namespace android


#endif //ANDROID_UTILS_H_
