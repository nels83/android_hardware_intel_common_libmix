/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

/**
 * SECTION:mixvideodef
 * @title: MI-X Video Data Definitons And Common Error Code
 * @short_description: MI-X Video data definitons and common error code
 * @include: mixvideodef.h
 * 
 * The section includes the definition of enum and struct as well as 
 * <note>
 * <title>Common Video Error Return Codes of MI-X video functions</title>
 * <itemizedlist>
 * <listitem>#MIX_RESULT_SUCCESS, Successfully resumed</listitem>
 * <listitem>MIX_RESULT_NULL_PTR, The pointer passed to the function was null.</listitem>
 * <listitem>MIX_RESULT_NO_MEMORY, Memory needed for the operation could not be allocated.</listitem>
 * <listitem>MIX_RESULT_INVALID_PARAM, An argument passed to the function was invalid.</listitem>
 * <listitem>MIX_RESULT_NOT_INIT, MixVideo object has not been initialized yet.</listitem>
 * <listitem>MIX_RESULT_NOT_CONFIGURED, MixVideo object has not been configured yet.</listitem>
 * <listitem>MIX_RESULT_FAIL, For any failure.</listitem>
 * </itemizedlist>
 * </note>
 */

#ifndef __MIX_VIDEO_DEF_H__
#define __MIX_VIDEO_DEF_H__
#include <mixresult.h>

/*
 * MI-X video error code
 */
typedef enum {
	MIX_RESULT_FRAME_NOTAVAIL = MIX_RESULT_ERROR_VIDEO_START + 1,
	MIX_RESULT_EOS,
	MIX_RESULT_POOLEMPTY,
	MIX_RESULT_OUTOFSURFACES,
	MIX_RESULT_DROPFRAME,
	MIX_RESULT_NOTIMPL,
	MIX_RESULT_VIDEO_LAST
} MIX_VIDEO_ERROR_CODE;

/*
 MixCodecMode
 */
typedef enum {
	MIX_CODEC_MODE_ENCODE = 0,
	MIX_CODEC_MODE_DECODE,
	MIX_CODEC_MODE_LAST
} MixCodecMode;

typedef enum {
	MIX_FRAMEORDER_MODE_DISPLAYORDER = 0,
	MIX_FRAMEORDER_MODE_DECODEORDER,
	MIX_FRAMEORDER_MODE_LAST
} MixFrameOrderMode;

typedef struct _MixIOVec {
	guchar *data;
	gint buffer_size;
    gint data_size;
} MixIOVec;

typedef struct _MixRect {
	gshort x;
	gshort y;
	gushort width;
	gushort height;
} MixRect;

typedef enum {
	MIX_STATE_UNINITIALIZED = 0,
	MIX_STATE_INITIALIZED,
	MIX_STATE_CONFIGURED,
	MIX_STATE_LAST
} MixState;


typedef enum
{
    MIX_RAW_TARGET_FORMAT_NONE = 0,
    MIX_RAW_TARGET_FORMAT_YUV420 = 1,
    MIX_RAW_TARGET_FORMAT_YUV422 = 2,
    MIX_RAW_TARGET_FORMAT_YUV444 = 4,
    MIX_RAW_TARGET_FORMAT_PROTECTED = 0x80000000,    
    MIX_RAW_TARGET_FORMAT_LAST
} MixRawTargetFormat;


typedef enum
{
    MIX_ENCODE_TARGET_FORMAT_MPEG4 = 0,
    MIX_ENCODE_TARGET_FORMAT_H263 = 2,
    MIX_ENCODE_TARGET_FORMAT_H264 = 4,
    MIX_ENCODE_TARGET_FORMAT_PREVIEW = 8,
    MIX_ENCODE_TARGET_FORMAT_LAST
} MixEncodeTargetFormat;


typedef enum
{
    MIX_RATE_CONTROL_NONE = 1,
    MIX_RATE_CONTROL_CBR = 2,
    MIX_RATE_CONTROL_VBR = 4,
    MIX_RATE_CONTROL_LAST
} MixRateControl;

typedef enum
{
    MIX_PROFILE_MPEG2SIMPLE = 0,
    MIX_PROFILE_MPEG2MAIN,
    MIX_PROFILE_MPEG4SIMPLE,
    MIX_PROFILE_MPEG4ADVANCEDSIMPLE,
    MIX_PROFILE_MPEG4MAIN,
    MIX_PROFILE_H264BASELINE,
    MIX_PROFILE_H264MAIN,
    MIX_PROFILE_H264HIGH,
    MIX_PROFILE_VC1SIMPLE,
    MIX_PROFILE_VC1MAIN,
    MIX_PROFILE_VC1ADVANCED,
    MIX_PROFILE_H263BASELINE
} MixProfile;

typedef enum
{
    MIX_DELIMITER_LENGTHPREFIX = 0,
    MIX_DELIMITER_ANNEXB
} MixDelimiterType;


#endif /*  __MIX_VIDEO_DEF_H__ */
