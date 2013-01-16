/*
 INTEL CONFIDENTIAL
 Copyright 2011 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

#ifndef __VIDEO_ENCODER_LOG_H__
#define __VIDEO_ENCODER_LOG_H__

// Components
#define VIDEO_ENC_COMP 		"VideoEncoder"

#include <utils/Log.h>

#define VIDEO_ENC_LOG_LEVEL_ERROR     ANDROID_LOG_ERROR
#define VIDEO_ENC_LOG_LEVEL_WARNING   ANDROID_LOG_WARN
#define VIDEO_ENC_LOG_LEVEL_INFO	ANDROID_LOG_INFO
#define VIDEO_ENC_LOG_LEVEL_VERBOSE   ANDROID_LOG_VERBOSE

#define mix_log(comp, level, format, ...) \
    __android_log_print(level, comp, "%s():%d: "format, \
    __FUNCTION__, __LINE__, ##__VA_ARGS__)

//#define VIDEO_ENC_LOG_ENABLE
#if  1
#ifdef VIDEO_ENC_LOG_ENABLE
#define LOG_V(format, ...) mix_log(VIDEO_ENC_COMP, VIDEO_ENC_LOG_LEVEL_VERBOSE, format, ##__VA_ARGS__)
#define LOG_I(format, ...) mix_log(VIDEO_ENC_COMP, VIDEO_ENC_LOG_LEVEL_INFO, format, ##__VA_ARGS__)
#define LOG_W(format, ...) mix_log(VIDEO_ENC_COMP, VIDEO_ENC_LOG_LEVEL_WARNING, format, ##__VA_ARGS__)
#else
#define LOG_V(format, ...)
#define LOG_I(format, ...)
#define LOG_W(format, ...)
#endif

#define LOG_E(format, ...) mix_log(VIDEO_ENC_COMP, VIDEO_ENC_LOG_LEVEL_ERROR, format, ##__VA_ARGS__)
#else
#define LOG_V printf
#define LOG_I printf
#define LOG_W printf
#define LOG_E printf
#endif

#define CHECK_VA_STATUS_RETURN(FUNC)\
    if (vaStatus != VA_STATUS_SUCCESS) {\
        LOG_E(FUNC" failed. vaStatus = %d\n", vaStatus);\
        return ENCODE_DRIVER_FAIL;\
    }

#define CHECK_VA_STATUS_GOTO_CLEANUP(FUNC)\
    if (vaStatus != VA_STATUS_SUCCESS) {\
        LOG_E(FUNC" failed. vaStatus = %d\n", vaStatus);\
        ret = ENCODE_DRIVER_FAIL; \
        goto CLEAN_UP;\
    }

#define CHECK_ENCODE_STATUS_RETURN(FUNC)\
    if (ret != ENCODE_SUCCESS) { \
        LOG_E(FUNC"Failed. ret = 0x%08x\n", ret); \
        return ret; \
    }

#define CHECK_ENCODE_STATUS_CLEANUP(FUNC)\
    if (ret != ENCODE_SUCCESS) { \
        LOG_E(FUNC"Failed, ret = 0x%08x\n", ret); \
        goto CLEAN_UP;\
    }

#define CHECK_NULL_RETURN_IFFAIL(POINTER)\
    if (POINTER == NULL) { \
        LOG_E("Invalid pointer\n"); \
        return ENCODE_NULL_PTR;\
    }
#endif /*  __VIDEO_ENCODER_LOG_H__ */
