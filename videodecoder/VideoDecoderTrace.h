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


#ifndef VIDEO_DECODER_TRACE_H_
#define VIDEO_DECODER_TRACE_H_


#define ENABLE_VIDEO_DECODER_TRACE
//#define ANDROID


#ifdef ENABLE_VIDEO_DECODER_TRACE

#ifndef ANDROID

#include <stdio.h>
#include <stdarg.h>

extern void TraceVideoDecoder(const char* cat, const char* fun, int line, const char* format, ...);
#define VIDEO_DECODER_TRACE(cat, format, ...) \
TraceVideoDecoder(cat, __FUNCTION__, __LINE__, format,  ##__VA_ARGS__)

#define ETRACE(format, ...) VIDEO_DECODER_TRACE("ERROR:   ",  format, ##__VA_ARGS__)
#define WTRACE(format, ...) VIDEO_DECODER_TRACE("WARNING: ",  format, ##__VA_ARGS__)
#define ITRACE(format, ...) VIDEO_DECODER_TRACE("INFO:    ",  format, ##__VA_ARGS__)
#define VTRACE(format, ...) VIDEO_DECODER_TRACE("VERBOSE: ",  format, ##__VA_ARGS__)

#else
// for Android OS

//#define LOG_NDEBUG 0

#define LOG_TAG "VideoDecoder"

#include <utils/Log.h>
#define ETRACE(...) LOGE(__VA_ARGS__)
#define WTRACE(...) LOGW(__VA_ARGS__)
#define ITRACE(...) LOGI(__VA_ARGS__)
#define VTRACE(...) LOGV(__VA_ARGS__)

#endif


#else

#define ETRACE(format, ...)
#define WTRACE(format, ...)
#define ITRACE(format, ...)
#define VTRACE(format, ...)


#endif /* ENABLE_VIDEO_DECODER_TRACE*/


#define CHECK_STATUS(FUNC)\
    if (status != DECODE_SUCCESS) {\
        if (status > DECODE_SUCCESS) {\
            WTRACE(FUNC" failed. status = %d", status);\
        } else {\
            ETRACE(FUNC" failed. status = %d", status);\
        }\
        return status;\
     }

#define CHECK_VA_STATUS(FUNC)\
    if (vaStatus != VA_STATUS_SUCCESS) {\
        ETRACE(FUNC" failed. vaStatus = 0x%x", vaStatus);\
        return DECODE_DRIVER_FAIL;\
    }

#define CHECK_VBP_STATUS(FUNC)\
    if (vbpStatus != VBP_OK) {\
        ETRACE(FUNC" failed. vbpStatus = %d", (int)vbpStatus);\
        if (vbpStatus == VBP_ERROR) {\
            return DECODE_FAIL;\
        }\
        return DECODE_PARSER_FAIL;\
    }

#endif /*VIDEO_DECODER_TRACE_H_*/


