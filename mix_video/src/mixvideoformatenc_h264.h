/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

#ifndef __MIX_VIDEOFORMATENC_H264_H__
#define __MIX_VIDEOFORMATENC_H264_H__

#include "mixvideoformatenc.h"
#include "mixvideoframe_private.h"

#define MIX_VIDEO_ENC_H264_SURFACE_NUM       20

#define min(X,Y) (((X) < (Y)) ? (X) : (Y))
#define max(X,Y) (((X) > (Y)) ? (X) : (Y))

/*
 * Type macros.
 */
#define MIX_VIDEOFORMATENC_H264(obj)                  (reinterpret_cast<MixVideoFormatEnc_H264*>(obj))
#define MIX_IS_VIDEOFORMATENC_H264(obj)            ((NULL !=MIX_VIDEOFORMATENC_H264(obj)) ? TRUE : FALSE)

class MixVideoFormatEnc_H264 : public MixVideoFormatEnc {

public:
    MixVideoFormatEnc_H264();
    ~MixVideoFormatEnc_H264();

    virtual MIX_RESULT Initialize(
        MixVideoConfigParamsEnc* config_params_enc,
        MixFrameManager * frame_mgr,
        MixBufferPool * input_buf_pool,
        MixSurfacePool ** surface_pool,
        MixUsrReqSurfacesInfo * requested_surface_info,
        VADisplay va_display);

    virtual MIX_RESULT Encode(
        MixBuffer * bufin[], int bufincnt,
        MixIOVec * iovout[], int iovoutcnt,
        MixVideoEncodeParams * encode_params);

    virtual MIX_RESULT Flush();

    virtual MIX_RESULT Deinitialize();

    virtual MIX_RESULT GetMaxEncodedBufSize (uint *max_size);

    virtual MIX_RESULT SetDynamicEncConfig (
        MixVideoConfigParamsEnc * config_params, MixEncParamsType params_type);

private:
    /* Local Methods */
    MIX_RESULT _process_encode (MixBuffer * bufin, MixIOVec * iovout);
    MIX_RESULT _AnnexB_to_length_prefixed (
        uint8 * bufin, uint bufin_len,
        uint8* bufout, uint *bufout_len);
    MIX_RESULT _send_encode_command ();
    MIX_RESULT _send_dynamic_bitrate ();
    MIX_RESULT _send_max_slice_size ();
    MIX_RESULT _send_dynamic_framerate ();
    MIX_RESULT _send_AIR ();
    MIX_RESULT _send_seq_params ();
    MIX_RESULT _send_picture_parameter ();
    MIX_RESULT _send_slice_parameter ();
    int _calc_level(int nummbs);


public:
    VABufferID coded_buf[2];
    VABufferID last_coded_buf;
    VABufferID seq_param_buf;
	VABufferID rc_param_buf;
    VABufferID pic_param_buf;
    VABufferID slice_param_buf;
    //VASurfaceID *   ci_shared_surfaces;
    VASurfaceID * shared_surfaces;
    VASurfaceID * surfaces;
    uint surface_num;
    uint shared_surfaces_cnt;
    uint precreated_surfaces_cnt;

    MixVideoFrame *cur_frame;	//current input frame to be encoded;
    MixVideoFrame *ref_frame;  //reference frame
    MixVideoFrame *rec_frame;	//reconstructed frame;
    MixVideoFrame *last_frame;	//last frame;
    MixVideoFrame *lookup_frame;
    MixBuffer *last_mix_buffer;

    uint basic_unit_size;  //for rate control
    uint disable_deblocking_filter_idc;
    uint8 vui_flag;
    MixDelimiterType delimiter_type;
    uint idr_interval;
    uint slice_num;
    uint I_slice_num;
    uint P_slice_num;
    uint va_rcmode_h264;

    uint encoded_frames;
    uint frame_num;
    bool pic_skipped;

    bool is_intra;

    uint coded_buf_size;
    uint coded_buf_index;
    uint8 ** usrptr;
    uint alloc_surface_cnt;

};



/**
 * mix_videoformatenc_h264_new:
 * @returns: A newly allocated instance of #MixVideoFormatEnc_H264
 *
 * Use this method to create new instance of #MixVideoFormatEnc_H264
 */
MixVideoFormatEnc_H264 *mix_videoformatenc_h264_new(void);

/**
 * mix_videoformatenc_h264_ref:
 * @mix: object to add reference
 * @returns: the MixVideoFormatEnc_H264 instance where reference count has been increased.
 *
 * Add reference count.
 */
MixVideoFormatEnc_H264 *mix_videoformatenc_h264_ref(MixVideoFormatEnc_H264 * mix);

/**
 * mix_videoformatenc_h264_unref:
 * @obj: object to unref.
 *
 * Decrement reference count of the object.
 */
MixVideoFormatEnc_H264 *mix_videoformatenc_h264_unref(MixVideoFormatEnc_H264 * mix);

#endif /* __MIX_VIDEOFORMATENC_H264_H__ */
