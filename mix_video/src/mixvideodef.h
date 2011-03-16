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


#define MAX_ENC_SURFACE_COUNT 20
#define MIX_STRING_TO_FOURCC(format)               ((uint32)(((format)[0])|((format)[1]<<8)|((format)[2]<<16)|((format)[3]<<24)))

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
    MIX_RESULT_VIDEO_ENC_SLICESIZE_OVERFLOW,
    MIX_RESULT_NOT_PERMITTED,
    MIX_RESULT_ERROR_PROCESS_STREAM,
    MIX_RESULT_MISSING_CONFIG,
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
    uchar *data;
    int buffer_size;
    int data_size;
} MixIOVec;

typedef struct _MixRect {
    short x;
    short y;
    ushort width;
    ushort height;
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
    MIX_RAW_TARGET_FORMAT_NV12 = 8,
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
    MIX_RATE_CONTROL_VCM = 8,
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

typedef enum {
    MIX_VIDEO_NONIR,
    MIX_VIDEO_CIR, 		/*Cyclic intra refresh*/
    MIX_VIDEO_AIR, 		/*Adaptive intra refresh*/
    MIX_VIDEO_BOTH,
    MIX_VIDEO_LAST
} MixVideoIntraRefreshType;

typedef struct _MixAIRParams
{
    uint air_MBs;
    uint air_threshold;
    uint air_auto;
} MixAIRParams;

typedef enum {
    MIX_ENC_PARAMS_START_UNUSED = 0x01000000,
    MIX_ENC_PARAMS_BITRATE,
    MIX_ENC_PARAMS_INIT_QP,
    MIX_ENC_PARAMS_MIN_QP,
    MIX_ENC_PARAMS_WINDOW_SIZE,
    MIX_ENC_PARAMS_TARGET_PERCENTAGE,
    MIX_ENC_PARAMS_SLICE_NUM,
    MIX_ENC_PARAMS_I_SLICE_NUM,
    MIX_ENC_PARAMS_P_SLICE_NUM,
    MIX_ENC_PARAMS_RESOLUTION,
    MIX_ENC_PARAMS_GOP_SIZE,
    MIX_ENC_PARAMS_FRAME_RATE,
    MIX_ENC_PARAMS_FORCE_KEY_FRAME,
    MIX_ENC_PARAMS_IDR_INTERVAL,
    MIX_ENC_PARAMS_RC_MODE,
    MIX_ENC_PARAMS_MTU_SLICE_SIZE,
    MIX_ENC_PARAMS_REFRESH_TYPE,
    MIX_ENC_PARAMS_AIR,
    MIX_ENC_PARAMS_CIR_FRAME_CNT,
    MIX_ENC_PARAMS_LAST
} MixEncParamsType;

typedef struct _MixEncDynamicParams {
    uint bitrate;
    uint init_QP;
    uint min_QP;
    uint window_size;
    uint target_percentage;
    uint slice_num;
    uint I_slice_num;
    uint P_slice_num;
    uint width;
    uint height;
    uint frame_rate_num;
    uint frame_rate_denom;
    uint intra_period;
    uint idr_interval;
    uint CIR_frame_cnt;
    uint max_slice_size;
    bool force_idr;
    MixRateControl rc_mode;
    MixVideoIntraRefreshType refresh_type;
    MixAIRParams air_params;
} MixEncDynamicParams;



typedef enum
{
    MIX_BUFFER_ALLOC_NORMAL = 1, //Means non shared buffer mode
    MIX_BUFFER_UPSTREAM_ALLOC_CI = 2,
    MIX_BUFFER_UPSTREAM_ALLOC_V4L2 = 4,
    MIX_BUFFER_UPSTREAM_ALLOC_SURFACE = 8,
    MIX_BUFFER_SELF_ALLOC_SURFACE = 16,
    MIX_BUFFER_LAST
} MixBufferAllocationMode;

typedef enum
{
    MIX_OUTPUT_BUFFER_NORMAL = 0,  //Output whatever driver generates
    MIX_OUTPUT_BUFFER_SEPARATED_HEADER = 1, //Applications need to pass at least two buffers, one for header and the other for data. And once this output mode is used, we will generate a new header
    MIX_OUTPUT_BUFFER_LAST
} MixOutputEncBufferMode;

typedef struct _MixCISharedBufferInfo {
    uint ci_frame_cnt;
    ulong *	ci_frame_id;
} MixCISharedBufferInfo ;

typedef struct _MixV4l2SharedBufferInfo {
    int v4l2_fd;
    uint v4l2_buf_cnt;
    void * v4l2_fmt;
    void ** v4l2_buf;
} MixV4l2SharedBufferInfo;

typedef struct _MixSurfaceSharedBufferInfo {
    uint surface_cnt;
    ulong *surface;
} MixSurfaceSharedBufferInfo;

/*
 * This union is defined for upstreamer buffer info Set/Get
 */
typedef union _MixSharedBufferInfo {
    MixCISharedBufferInfo ci_buffer_info;
    MixV4l2SharedBufferInfo v4l2_buffer_info;
    MixSurfaceSharedBufferInfo surface_buffer_info;
} MixSharedBufferInfo;


typedef struct _MixUserReqSurfacesInfo {
    uint surface_allocated[MAX_ENC_SURFACE_COUNT];
    uint8 * usrptr[MAX_ENC_SURFACE_COUNT];
    uint surface_cnt;
} MixUsrReqSurfacesInfo;
#endif /*  __MIX_VIDEO_DEF_H__ */
