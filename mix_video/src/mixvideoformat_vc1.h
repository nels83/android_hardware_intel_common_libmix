/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

#ifndef __MIX_VIDEOFORMAT_VC1_H__
#define __MIX_VIDEOFORMAT_VC1_H__

#include "mixvideoformat.h"
#include "mixvideoframe_private.h"


//Note: this is only a max limit.  Actual number of surfaces allocated is calculated in mix_videoformat_vc1_initialize()
#define MIX_VIDEO_VC1_SURFACE_NUM	8

/*
 * Type macros.
 */
#define MIX_VIDEOFORMAT_VC1(obj)		(reinterpret_cast<MixVideoFormat_VC1*>(obj))
#define MIX_IS_VIDEOFORMAT_VC1(obj)		(NULL != MIX_VIDEOFORMAT_VC1(obj))

class MixVideoFormat_VC1 : public MixVideoFormat {
public:
    MixVideoFormat_VC1();
    virtual ~MixVideoFormat_VC1();

    virtual MIX_RESULT Initialize(
        MixVideoConfigParamsDec * config_params,
        MixFrameManager * frame_mgr,
        MixBufferPool * input_buf_pool,
        MixSurfacePool ** surface_pool,
        VADisplay va_display);
    virtual MIX_RESULT Decode(
        MixBuffer * bufin[], int bufincnt,
        MixVideoDecodeParams * decode_params);
    virtual MIX_RESULT Flush();
    virtual MIX_RESULT EndOfStream();

private:
    MIX_RESULT _handle_ref_frames(
        enum _picture_type frame_type, MixVideoFrame * current_frame);
    MIX_RESULT _process_decode(
        vbp_data_vc1 *data, uint64 timestamp, bool discontinuity);
    MIX_RESULT _release_input_buffers(uint64 timestamp);
    MIX_RESULT _update_seq_header(
        MixVideoConfigParamsDec* config_params, MixIOVec *header);
    MIX_RESULT _update_config_params(vbp_data_vc1 *data);
    MIX_RESULT _decode_a_picture(
        vbp_data_vc1 *data, int pic_index, MixVideoFrame *frame);
#ifdef YUVDUMP
    MIX_RESULT _get_Img_from_surface (MixVideoFrame * frame);
#endif


public:
    /*< public > */

    /*< private > */
    MixVideoFrame * reference_frames[2];
    bool haveBframes;
    bool loopFilter;
    MixVideoFrame * lastFrame;
};

/**
 * mix_videoformat_vc1_new:
 * @returns: A newly allocated instance of #MixVideoFormat_VC1
 *
 * Use this method to create new instance of #MixVideoFormat_VC1
 */
MixVideoFormat_VC1 *mix_videoformat_vc1_new(void);

/**
 * mix_videoformat_vc1_ref:
 * @mix: object to add reference
 * @returns: the MixVideoFormat_VC1 instance where reference count has been increased.
 *
 * Add reference count.
 */
MixVideoFormat_VC1 *mix_videoformat_vc1_ref(MixVideoFormat_VC1 * mix);

/**
 * mix_videoformat_vc1_unref:
 * @obj: object to unref.
 *
 * Decrement reference count of the object.
 */
MixVideoFormat_VC1 *mix_videoformat_vc1_unref(MixVideoFormat_VC1 * mix);


#endif /* __MIX_VIDEOFORMAT_VC1_H__ */
