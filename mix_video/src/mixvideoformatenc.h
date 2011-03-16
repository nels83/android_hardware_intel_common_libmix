/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

#ifndef __MIX_VIDEOFORMATENC_H__
#define __MIX_VIDEOFORMATENC_H__

#include <va/va.h>
#include "mixvideodef.h"
#include <mixdrmparams.h>
#include "mixvideoconfigparamsenc.h"
#include "mixvideoframe.h"
#include "mixframemanager.h"
#include "mixsurfacepool.h"
#include "mixbuffer.h"
#include "mixbufferpool.h"
#include "mixvideoformatqueue.h"
#include "mixvideoencodeparams.h"
#include <j_queue.h>
class MixVideoFormatEnc;



#define MIX_VIDEOFORMATENC(obj)                  (reinterpret_cast<MixVideoFormatEnc*>(obj))
#define MIX_IS_VIDEOFORMATENC(obj)               (NULL != MIX_VIDEOFORMATENC(obj))

/* vmethods typedef */

/* TODO: change return type and method parameters */
typedef MIX_RESULT (*MixVideoFmtEncGetCapsFunc)(MixVideoFormatEnc *mix, char *msg);
typedef MIX_RESULT (*MixVideoFmtEncInitializeFunc)(MixVideoFormatEnc *mix,
        MixVideoConfigParamsEnc* config_params_enc,
        MixFrameManager * frame_mgr,
        MixBufferPool * input_buf_pool,
        MixSurfacePool ** surface_pool,
        MixUsrReqSurfacesInfo * requested_surface_info,
        VADisplay va_display);
typedef MIX_RESULT (*MixVideoFmtEncodeFunc)(MixVideoFormatEnc *mix, MixBuffer * bufin[],
        int bufincnt, MixIOVec * iovout[], int iovoutcnt,
        MixVideoEncodeParams * encode_params);
typedef MIX_RESULT (*MixVideoFmtEncFlushFunc)(MixVideoFormatEnc *mix);
typedef MIX_RESULT (*MixVideoFmtEncEndOfStreamFunc)(MixVideoFormatEnc *mix);
typedef MIX_RESULT (*MixVideoFmtEncDeinitializeFunc)(MixVideoFormatEnc *mix);
typedef MIX_RESULT (*MixVideoFmtEncGetMaxEncodedBufSizeFunc) (MixVideoFormatEnc *mix, uint *max_size);
typedef MIX_RESULT (*MixVideoFmtEncSetDynamicEncConfigFunc) (MixVideoFormatEnc * mix,
        MixVideoConfigParamsEnc * config_params,
        MixEncParamsType params_type);

class MixVideoFormatEnc {
public:
    MixVideoFormatEnc();
    virtual ~MixVideoFormatEnc();

    virtual MIX_RESULT GetCaps(char *msg);
    virtual MIX_RESULT Initialize(
        MixVideoConfigParamsEnc* config_params_enc,
        MixFrameManager * frame_mgr,
        MixBufferPool * input_buf_pool,
        MixSurfacePool ** surface_pool,
        MixUsrReqSurfacesInfo * requested_surface_info,
        VADisplay va_display);
    virtual MIX_RESULT Encode( MixBuffer * bufin[],
                               int bufincnt, MixIOVec * iovout[], int iovoutcnt,
                               MixVideoEncodeParams * encode_params);
    virtual MIX_RESULT Flush();
    virtual MIX_RESULT EndOfStream();
    virtual MIX_RESULT Deinitialize();
    virtual MIX_RESULT GetMaxEncodedBufSize (uint *max_size);
    virtual MIX_RESULT SetDynamicEncConfig (
        MixVideoConfigParamsEnc * config_params, MixEncParamsType params_type);

    void Lock() {
        mLock.lock();
    }
    void Unlock() {
        mLock.unlock();
    }

    MixVideoFormatEnc* Ref() {
        ++ref_count;
        return this;
    }

    MixVideoFormatEnc* Unref() {
        if (0 == (--ref_count)) {
            delete this;
            return NULL;
        } else {
            return this;
        }
    }

public:

    MixVideoMutex mLock;
    bool initialized;
    MixFrameManager *framemgr;
    MixSurfacePool *surfacepool;
    VADisplay va_display;
    VAContextID va_context;
    VAConfigID va_config;
    char *mime_type;
    MixRawTargetFormat raw_format;
    uint frame_rate_num;
    uint frame_rate_denom;
    uint picture_width;
    uint picture_height;
    uint intra_period;
    /*
    * Following is for bitrate control
    */
    uint initial_qp;
    uint min_qp;
    uint bitrate;
    uint target_percentage;
    uint window_size;

    bool share_buf_mode;
    ulong * ci_frame_id;
    uint ci_frame_num;

    bool force_key_frame;
    bool new_header_required;

    MixVideoIntraRefreshType refresh_type;

    uint CIR_frame_cnt;

    MixAIRParams air_params;

    uint	max_slice_size;
    bool render_mss_required;
    bool render_QP_required;
    bool render_AIR_required;
    bool render_framerate_required;
    bool render_bitrate_required;

    ulong    drawable;
    bool need_display;

    VAProfile va_profile;
    VAEntrypoint va_entrypoint;
    uint va_format;
    uint va_rcmode;
    uint8 level;

    MixBufferAllocationMode buffer_mode;
    void * buf_info;

    MixBufferPool *inputbufpool;
    JQueue *inputbufqueue;
    uint ref_count ;
};


/**
 * mix_videoformatenc_new:
 * @returns: A newly allocated instance of #MixVideoFormatEnc
 *
 * Use this method to create new instance of #MixVideoFormatEnc
 */
MixVideoFormatEnc *mix_videoformatenc_new(void);

/**
 * mix_videoformatenc_ref:
 * @mix: object to add reference
 * @returns: the MixVideoFormatEnc instance where reference count has been increased.
 *
 * Add reference count.
 */
MixVideoFormatEnc *mix_videoformatenc_ref(MixVideoFormatEnc * mix);

/**
 * mix_videoformatenc_unref:
 * @obj: object to unref.
 *
 * Decrement reference count of the object.
 */
MixVideoFormatEnc *mix_videoformatenc_unref(MixVideoFormatEnc * mix);


/* Class Methods */

/* TODO: change method parameter list */
MIX_RESULT mix_videofmtenc_getcaps(MixVideoFormatEnc *mix, char *msg);

MIX_RESULT mix_videofmtenc_initialize(MixVideoFormatEnc *mix,
                                      MixVideoConfigParamsEnc * enc_config_params,
                                      MixFrameManager * frame_mgr,
                                      MixBufferPool * input_buf_pool,
                                      MixSurfacePool ** surface_pool,
                                      MixUsrReqSurfacesInfo * requested_surface_info,
                                      VADisplay va_display);

MIX_RESULT mix_videofmtenc_encode(
    MixVideoFormatEnc *mix, MixBuffer * bufin[],
    int bufincnt, MixIOVec * iovout[], int iovoutcnt,
    MixVideoEncodeParams * encode_params);

MIX_RESULT mix_videofmtenc_flush(MixVideoFormatEnc *mix);

MIX_RESULT mix_videofmtenc_eos(MixVideoFormatEnc *mix);

MIX_RESULT mix_videofmtenc_deinitialize(MixVideoFormatEnc *mix);

MIX_RESULT mix_videofmtenc_get_max_coded_buffer_size(MixVideoFormatEnc *mix,
        uint *max_size);

MIX_RESULT mix_videofmtenc_set_dynamic_enc_config (MixVideoFormatEnc * mix,
        MixVideoConfigParamsEnc * config_params,
        MixEncParamsType params_type);


#endif /* __MIX_VIDEOFORMATENC_H__ */
