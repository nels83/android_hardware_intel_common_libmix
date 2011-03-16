/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */
#include <string.h>
#include "mixvideolog.h"
#include "mixvideoformatenc.h"

//#define MDEBUG

MixVideoFormatEnc::MixVideoFormatEnc()
        :mLock()
        ,initialized(FALSE)
        ,framemgr(NULL)
        ,surfacepool(NULL)
        ,va_display(NULL)
        ,va_context(0)
        ,va_config(0)
        ,mime_type(NULL)
        ,frame_rate_num(0)
        ,frame_rate_denom(1)
        ,picture_width(0)
        ,picture_height(0)
        ,intra_period(0)
        ,initial_qp(0)
        ,min_qp(0)
        ,bitrate(0)
        ,target_percentage(95)
        ,window_size(500)
        ,share_buf_mode(FALSE)
        ,ci_frame_id(NULL)
        ,ci_frame_num(0)
        ,force_key_frame(FALSE)
        ,new_header_required(FALSE)
        ,refresh_type(MIX_VIDEO_NONIR)
        ,CIR_frame_cnt(15)
        ,max_slice_size(0)
        ,render_mss_required(FALSE)
        ,render_QP_required (FALSE)
        ,render_AIR_required(FALSE)
        ,render_framerate_required(FALSE)
        ,render_bitrate_required(FALSE)
        ,drawable(0X0)
        ,need_display(TRUE)
        ,va_profile(VAProfileH264Baseline)
        ,va_entrypoint(VAEntrypointEncSlice)
        ,va_format(VA_RT_FORMAT_YUV420)
        ,va_rcmode(VA_RC_NONE)
        ,level(40)
        ,buffer_mode(MIX_BUFFER_ALLOC_NORMAL)
        ,buf_info(NULL)
        ,inputbufpool(NULL)
        ,inputbufqueue(NULL)
        ,ref_count(1) {
    air_params.air_MBs = 0;
    air_params.air_threshold = 0;
    air_params.air_auto = 0;
}

MixVideoFormatEnc::~MixVideoFormatEnc() {
    LOG_V( "\n");
    //MiVideo object calls the _deinitialize() for frame manager
    if (this->framemgr) {
        mix_framemanager_unref(this->framemgr);
        this->framemgr = NULL;
    }

    if (this->mime_type) {
        free(this->mime_type);
    }

    if (this->ci_frame_id)
        free (this->ci_frame_id);

    if (this->surfacepool) {
        mix_surfacepool_deinitialize(this->surfacepool);
        mix_surfacepool_unref(this->surfacepool);
        this->surfacepool = NULL;
    }

    if (this->buffer_mode == MIX_BUFFER_UPSTREAM_ALLOC_CI) {
        MixCISharedBufferInfo * ci_tmp = NULL;
        if (this->buf_info) {
            ci_tmp = reinterpret_cast<MixCISharedBufferInfo*> (this->buf_info);
            if (ci_tmp->ci_frame_id) {
                free (ci_tmp->ci_frame_id);
                ci_tmp->ci_frame_id = NULL;
            }
            free (ci_tmp);
            ci_tmp = NULL;
            this->buf_info = NULL;
        }
    }
}


MixVideoFormatEnc *
mix_videoformatenc_new(void) {
    return new MixVideoFormatEnc() ;
}


MixVideoFormatEnc *
mix_videoformatenc_ref(MixVideoFormatEnc * mix) {
    if (NULL != mix)
        return mix->Ref();
    else
        return NULL;
}

MixVideoFormatEnc *
mix_videoformatenc_unref(MixVideoFormatEnc * mix) {
    if (NULL!=mix)
        return mix->Unref();
    else
        return NULL;
}

MIX_RESULT
MixVideoFormatEnc::GetCaps(char *msg) {
    LOG_V( "Begin\n");
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT
MixVideoFormatEnc::Initialize(
    MixVideoConfigParamsEnc* config_params_enc,
    MixFrameManager * frame_mgr,
    MixBufferPool * input_buf_pool,
    MixSurfacePool ** surface_pool,
    MixUsrReqSurfacesInfo * requested_surface_info,
    VADisplay va_display) {

    LOG_V( "Begin\n");

    if (config_params_enc == NULL) {
        LOG_E("config_params_enc == NULL\n");
        return MIX_RESULT_NULL_PTR;
    }

    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    //TODO check return values of getter fns for config_params

    this->Lock();
    this->framemgr = frame_mgr;
    mix_framemanager_ref(this->framemgr);

    this->va_display = va_display;

    LOG_V("Start to get properities from parent params\n");

    /* get properties from param (parent) Object*/
    ret = mix_videoconfigparamsenc_get_bit_rate (
              config_params_enc, &(this->bitrate));

    if (ret != MIX_RESULT_SUCCESS) {
        //TODO cleanup
        LOG_E("Failed to mix_videoconfigparamsenc_get_bps\n");
        this->Unlock();
        return MIX_RESULT_FAIL;
    }

    ret = mix_videoconfigparamsenc_get_frame_rate (
              config_params_enc, &(this->frame_rate_num), &(this->frame_rate_denom));

    if (ret != MIX_RESULT_SUCCESS) {
        //TODO cleanup
        LOG_E("Failed to mix_videoconfigparamsenc_get_frame_rate\n");
        this->Unlock();
        return MIX_RESULT_FAIL;
    }

    ret = mix_videoconfigparamsenc_get_init_qp (
              config_params_enc, &(this->initial_qp));

    if (ret != MIX_RESULT_SUCCESS) {
        //TODO cleanup
        LOG_E("Failed to mix_videoconfigparamsenc_get_init_qp\n");
        this->Unlock();
        return MIX_RESULT_FAIL;
    }

    ret = mix_videoconfigparamsenc_get_min_qp (
              config_params_enc, &(this->min_qp));

    if (ret != MIX_RESULT_SUCCESS) {
        //TODO cleanup
        LOG_E("Failed to mix_videoconfigparamsenc_get_min_qp\n");
        this->Unlock();
        return MIX_RESULT_FAIL;
    }

    ret = mix_videoconfigparamsenc_get_target_percentage(
              config_params_enc, &(this->target_percentage));

    if (ret != MIX_RESULT_SUCCESS) {
        //TODO cleanup
        LOG_E("Failed to mix_videoconfigparamsenc_get_target_percentage\n");
        this->Unlock();
        return MIX_RESULT_FAIL;
    }

    ret = mix_videoconfigparamsenc_get_window_size (
              config_params_enc, &(this->window_size));

    if (ret != MIX_RESULT_SUCCESS) {
        //TODO cleanup
        LOG_E("Failed to mix_videoconfigparamsenc_get_window_size\n");
        this->Unlock();
        return MIX_RESULT_FAIL;
    }

    ret = mix_videoconfigparamsenc_get_intra_period (
              config_params_enc, &(this->intra_period));

    if (ret != MIX_RESULT_SUCCESS) {
        //TODO cleanup
        LOG_E("Failed to mix_videoconfigparamsenc_get_intra_period\n");
        this->Unlock();
        return MIX_RESULT_FAIL;
    }

    ret = mix_videoconfigparamsenc_get_picture_res (
              config_params_enc, &(this->picture_width), &(this->picture_height));

    if (ret != MIX_RESULT_SUCCESS) {
        //TODO cleanup
        LOG_E("Failed to mix_videoconfigparamsenc_get_picture_res\n");
        this->Unlock();
        return MIX_RESULT_FAIL;
    }

    ret = mix_videoconfigparamsenc_get_share_buf_mode (
              config_params_enc, &(this->share_buf_mode));

    if (ret != MIX_RESULT_SUCCESS) {
        //TODO cleanup
        LOG_E("Failed to mix_videoconfigparamsenc_get_share_buf_mode\n");
        this->Unlock();
        return MIX_RESULT_FAIL;
    }


    ret = mix_videoconfigparamsenc_get_ci_frame_info (
              config_params_enc, &(this->ci_frame_id),  &(this->ci_frame_num));

    if (ret != MIX_RESULT_SUCCESS) {
        //TODO cleanup
        LOG_E("Failed to mix_videoconfigparamsenc_get_ci_frame_info\n");
        this->Unlock();
        return MIX_RESULT_FAIL;
    }


    /*
      *  temporarily code here for compatibility with old CI shared buffer solution
      */

    if (this->share_buf_mode) {
        ret = mix_videoconfigparamsenc_set_buffer_mode (config_params_enc, MIX_BUFFER_UPSTREAM_ALLOC_CI);
        if (ret != MIX_RESULT_SUCCESS) {
            //TODO cleanup

            LOG_E("Failed to mix_videoconfigparamsenc_set_buffer_mode\n");
            this->Unlock();
            return MIX_RESULT_FAIL;
        }
    }

    if (this->share_buf_mode && this->ci_frame_id && this->ci_frame_num) {

        MixCISharedBufferInfo * ci_tmp = NULL;
        //ci_tmp = (MixCISharedBufferInfo *) g_malloc (sizeof (MixCISharedBufferInfo));
        ci_tmp = (MixCISharedBufferInfo *) new MixCISharedBufferInfo;
        if (!ci_tmp) {
            return MIX_RESULT_NO_MEMORY;
        }
        ci_tmp->ci_frame_cnt = this->ci_frame_num;
        //ci_tmp->ci_frame_id = g_malloc (ci_tmp->ci_frame_cnt * sizeof (gulong));
        ci_tmp->ci_frame_id = new ulong [ci_tmp->ci_frame_cnt];
        if (!ci_tmp->ci_frame_id) {
            return MIX_RESULT_NO_MEMORY;
        }

        memcpy (ci_tmp->ci_frame_id, this->ci_frame_id, ci_tmp->ci_frame_cnt * sizeof (ulong));
        ret = mix_videoconfigparamsenc_set_upstream_buffer_info (config_params_enc, MIX_BUFFER_UPSTREAM_ALLOC_CI, (void*)ci_tmp);
        if (ret != MIX_RESULT_SUCCESS) {
            //TODO cleanup
            LOG_E("Failed to mix_videoconfigparamsenc_set_upstream_buffer_info\n");
            this->Unlock();
            return MIX_RESULT_FAIL;
        }

        free (ci_tmp->ci_frame_id);
        ci_tmp->ci_frame_id = NULL;
        free (ci_tmp);
        ci_tmp = NULL;

    }

    /*
    * temporarily code done
    */

    ret = mix_videoconfigparamsenc_get_drawable (
              config_params_enc, &(this->drawable));

    if (ret != MIX_RESULT_SUCCESS) {
        //TODO cleanup
        LOG_E("Failed to mix_videoconfigparamsenc_get_drawable\n");
        this->Unlock();
        return MIX_RESULT_FAIL;
    }

    ret = mix_videoconfigparamsenc_get_need_display (
              config_params_enc, &(this->need_display));

    if (ret != MIX_RESULT_SUCCESS) {
        //TODO cleanup
        LOG_E("Failed to mix_videoconfigparamsenc_get_drawable\n");
        this->Unlock();
        return MIX_RESULT_FAIL;
    }

    ret = mix_videoconfigparamsenc_get_rate_control (
              config_params_enc,(MixRateControl*)&(this->va_rcmode));

    if (ret != MIX_RESULT_SUCCESS) {
        //TODO cleanup
        LOG_E("Failed to mix_videoconfigparamsenc_get_rc_mode\n");
        this->Unlock();
        return MIX_RESULT_FAIL;
    }

    ret = mix_videoconfigparamsenc_get_raw_format (
              config_params_enc, &(this->raw_format));

    if (ret != MIX_RESULT_SUCCESS) {
        //TODO cleanup
        LOG_E("Failed to mix_videoconfigparamsenc_get_format\n");
        this->Unlock();
        return MIX_RESULT_FAIL;
    }

    ret = mix_videoconfigparamsenc_get_profile (
              config_params_enc, (MixProfile *) &(this->va_profile));

    if (ret != MIX_RESULT_SUCCESS) {
        //TODO cleanup
        LOG_E("Failed to mix_videoconfigparamsenc_get_profile\n");
        this->Unlock();
        return MIX_RESULT_FAIL;
    }

    ret = mix_videoconfigparamsenc_get_level (
              config_params_enc, &(this->level));

    if (ret != MIX_RESULT_SUCCESS) {
        //TODO cleanup
        LOG_E("Failed to mix_videoconfigparamsenc_get_level\n");
        this->Unlock();
        return MIX_RESULT_FAIL;
    }

    ret = mix_videoconfigparamsenc_get_CIR_frame_cnt(
              config_params_enc, &(this->CIR_frame_cnt));

    if (ret != MIX_RESULT_SUCCESS) {
        //TODO cleanup
        LOG_E("Failed to mix_videoconfigparamsenc_get_CIR_frame_cnt\n");
        this->Unlock();
        return MIX_RESULT_FAIL;
    }


    ret = mix_videoconfigparamsenc_get_max_slice_size(
              config_params_enc, &(this->max_slice_size));

    if (ret != MIX_RESULT_SUCCESS) {
        //TODO cleanup
        LOG_E("Failed to mix_videoconfigparamsenc_get_max_slice_size\n");
        this->Unlock();
        return MIX_RESULT_FAIL;
    }

    ret = mix_videoconfigparamsenc_get_refresh_type(
              config_params_enc, &(this->refresh_type));

    if (ret != MIX_RESULT_SUCCESS) {
        //TODO cleanup
        LOG_E("Failed to mix_videoconfigparamsenc_get_refresh_type\n");
        this->Unlock();
        return MIX_RESULT_FAIL;
    }

    ret = mix_videoconfigparamsenc_get_AIR_params(
              config_params_enc, &(this->air_params));

    if (ret != MIX_RESULT_SUCCESS) {
        //TODO cleanup
        LOG_E("Failed to mix_videoconfigparamsenc_get_AIR_params\n");
        this->Unlock();
        return MIX_RESULT_FAIL;
    }


    ret = mix_videoconfigparamsenc_get_buffer_mode(
              config_params_enc, &(this->buffer_mode));

    if (ret != MIX_RESULT_SUCCESS) {
        //TODO cleanup
        LOG_E("Failed to mix_videoconfigparamsenc_get_buffer_mode\n");
        this->Unlock();
        return MIX_RESULT_FAIL;
    }

    if (this->buffer_mode == MIX_BUFFER_UPSTREAM_ALLOC_CI) {
        ret = mix_videoconfigparamsenc_get_upstream_buffer_info (
                  config_params_enc, this->buffer_mode, &(this->buf_info));
        if (ret != MIX_RESULT_SUCCESS) {
            LOG_V ("ret = %d\n", ret);
            LOG_E("Failed to mix_videoconfigparamsenc_get_upstream_buffer_info\n");
            this->Unlock();
            return MIX_RESULT_FAIL;
        }
    }

    LOG_V("======Video Encode Parent Object properities======:\n");
    LOG_I( "mix->bitrate = %d\n", this->bitrate);
    LOG_I( "mix->frame_rate = %d\n", this->frame_rate_denom / this->frame_rate_denom);
    LOG_I( "mix->initial_qp = %d\n", this->initial_qp);
    LOG_I( "mix->min_qp = %d\n", this->min_qp);
    LOG_I( "mix->intra_period = %d\n", this->intra_period);
    LOG_I( "mix->picture_width = %d\n", this->picture_width);
    LOG_I( "mix->picture_height = %d\n", this->picture_height);
    LOG_I( "mix->share_buf_mode = %d\n", this->share_buf_mode);
    LOG_I( "mix->ci_frame_id = 0x%08x\n", this->ci_frame_id);
    LOG_I( "mix->ci_frame_num = %d\n", this->ci_frame_num);
    LOG_I( "mix->drawable = 0x%08x\n", this->drawable);
    LOG_I( "mix->need_display = %d\n", this->need_display);
    LOG_I( "mix->va_format = %d\n", this->va_format);
    LOG_I( "mix->va_profile = %d\n", this->va_profile);
    LOG_I( "mix->va_rcmode = %d\n\n", this->va_rcmode);
    LOG_I( "mix->CIR_frame_cnt = %d\n\n", this->CIR_frame_cnt);
    LOG_I( "mix->max_slice_size = %d\n\n", this->max_slice_size);

    //g_mutex_unlock(mix->objectlock);
    this->Unlock();
    LOG_V( "end\n");

    return MIX_RESULT_SUCCESS;
}

MIX_RESULT
MixVideoFormatEnc:: Encode(
    MixBuffer * bufin[], int bufincnt, MixIOVec * iovout[],
    int iovoutcnt, MixVideoEncodeParams * encode_params) {
    return MIX_RESULT_SUCCESS;
}


MIX_RESULT
MixVideoFormatEnc::Flush() {
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT
MixVideoFormatEnc::EndOfStream() {
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT MixVideoFormatEnc::Deinitialize() {
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT  MixVideoFormatEnc::GetMaxEncodedBufSize (uint *max_size) {
    return MIX_RESULT_SUCCESS;
}


MIX_RESULT MixVideoFormatEnc::SetDynamicEncConfig (
    MixVideoConfigParamsEnc * config_params_enc,
    MixEncParamsType params_type) {

    MIX_RESULT ret = MIX_RESULT_SUCCESS;

    if (config_params_enc == NULL) {
        LOG_E(" config_params_enc == NULL\n");
        return MIX_RESULT_NULL_PTR;
    }

    this->Lock();
    this->new_header_required = FALSE;

    switch (params_type) {
    case MIX_ENC_PARAMS_BITRATE:
    {
        ret = mix_videoconfigparamsenc_get_bit_rate (config_params_enc, &(this->bitrate));
        if (ret != MIX_RESULT_SUCCESS) {
            //TODO cleanup
            LOG_E("Failed to mix_videoconfigparamsenc_get_bit_rate\n");
            this->Unlock();
            return MIX_RESULT_FAIL;
        }
        this->render_bitrate_required = TRUE;
    }
    break;

    case MIX_ENC_PARAMS_INIT_QP:
    {
        ret = mix_videoconfigparamsenc_get_init_qp (config_params_enc, &(this->initial_qp));
        if (ret != MIX_RESULT_SUCCESS) {
            //TODO cleanup

            LOG_E("Failed to mix_videoconfigparamsenc_get_init_qp\n");
            this->Unlock();
            return MIX_RESULT_FAIL;
        }

        this->render_bitrate_required = TRUE;
    }
    break;

    case MIX_ENC_PARAMS_MIN_QP:
    {
        ret = mix_videoconfigparamsenc_get_min_qp (config_params_enc, &(this->min_qp));
        if (ret != MIX_RESULT_SUCCESS) {
            //TODO cleanup

            LOG_E("Failed to mix_videoconfigparamsenc_get_min_qp\n");
            this->Unlock();
            return MIX_RESULT_FAIL;
        }

        this->render_bitrate_required = TRUE;
    }
    break;

    case MIX_ENC_PARAMS_WINDOW_SIZE:
    {
        ret = mix_videoconfigparamsenc_get_window_size (config_params_enc, &(this->window_size));
        if (ret != MIX_RESULT_SUCCESS) {
            //TODO cleanup

            LOG_E("Failed to MIX_ENC_PARAMS_WINDOW_SIZE\n");
            this->Unlock();
            return MIX_RESULT_FAIL;
        }

        this->render_bitrate_required = TRUE;
    }
    break;

    case MIX_ENC_PARAMS_TARGET_PERCENTAGE:
    {
        ret = mix_videoconfigparamsenc_get_target_percentage (config_params_enc, &(this->target_percentage));
        if (ret != MIX_RESULT_SUCCESS) {
            //TODO cleanup

            LOG_E("Failed to MIX_ENC_PARAMS_TARGET_PERCENTAGE\n");
            this->Unlock();
            return MIX_RESULT_FAIL;
        }

        this->render_bitrate_required = TRUE;
    }
    break;

    case MIX_ENC_PARAMS_MTU_SLICE_SIZE:
    {
        ret = mix_videoconfigparamsenc_get_max_slice_size(config_params_enc, &(this->max_slice_size));
        if (ret != MIX_RESULT_SUCCESS) {
            LOG_E("Failed mix_videoconfigparamsenc_get_max_slice_size\n");
            this->Unlock();
            return MIX_RESULT_FAIL;
        }

        this->render_mss_required = TRUE;

    }

    case MIX_ENC_PARAMS_SLICE_NUM:
    {
        /*
        * This type of dynamic control will be handled in H.264 override method
        */
    }
    break;

    case MIX_ENC_PARAMS_RC_MODE:
    {
        ret = mix_videoconfigparamsenc_get_rate_control (config_params_enc, (MixRateControl*)&(this->va_rcmode));
        if (ret != MIX_RESULT_SUCCESS) {
            //TODO cleanup

            LOG_E("Failed to mix_videoconfigparamsenc_get_rate_control\n");
            this->Unlock();
            return MIX_RESULT_FAIL;
        }

        /*
        * We only can change the RC mode to re-start encoding session
        */

    }
    break;

    case MIX_ENC_PARAMS_RESOLUTION:
    {

        ret = mix_videoconfigparamsenc_get_picture_res (config_params_enc, &(this->picture_width), &(this->picture_height));
        if (ret != MIX_RESULT_SUCCESS) {
            //TODO cleanup

            LOG_E("Failed to mix_videoconfigparamsenc_get_picture_res\n");
            this->Unlock();
            return MIX_RESULT_FAIL;
        }

        this->new_header_required = TRUE;
    }
    break;
    case MIX_ENC_PARAMS_GOP_SIZE:
    {

        ret = mix_videoconfigparamsenc_get_intra_period (config_params_enc, &(this->intra_period));
        if (ret != MIX_RESULT_SUCCESS) {
            //TODO cleanup

            LOG_E("Failed to mix_videoconfigparamsenc_get_intra_period\n");
            this->Unlock();
            return MIX_RESULT_FAIL;
        }

        this->new_header_required = TRUE;

    }
    break;
    case MIX_ENC_PARAMS_FRAME_RATE:
    {
        ret = mix_videoconfigparamsenc_get_frame_rate (config_params_enc, &(this->frame_rate_num),  &(this->frame_rate_denom));
        if (ret != MIX_RESULT_SUCCESS) {
            //TODO cleanup

            LOG_E("Failed to mix_videoconfigparamsenc_get_frame_rate\n");
            this->Unlock();
            return MIX_RESULT_FAIL;
        }

        this->render_framerate_required = TRUE;
    }
    break;

    case MIX_ENC_PARAMS_FORCE_KEY_FRAME:
    {
        this->new_header_required = TRUE;

    }
    break;

    case MIX_ENC_PARAMS_REFRESH_TYPE:
    {
        ret = mix_videoconfigparamsenc_get_refresh_type(config_params_enc, &(this->refresh_type));
        if (ret != MIX_RESULT_SUCCESS) {
            //TODO cleanup

            LOG_E("Failed to mix_videoconfigparamsenc_get_refresh_type\n");
            this->Unlock();
            return MIX_RESULT_FAIL;
        }
    }
    break;

    case MIX_ENC_PARAMS_AIR:
    {
        ret = mix_videoconfigparamsenc_get_AIR_params(config_params_enc, &(this->air_params));
        if (ret != MIX_RESULT_SUCCESS) {
            //TODO cleanup

            LOG_E("Failed to mix_videoconfigparamsenc_get_AIR_params\n");
            this->Unlock();
            return MIX_RESULT_FAIL;
        }

        this->render_AIR_required = TRUE;
    }
    break;

    case MIX_ENC_PARAMS_CIR_FRAME_CNT:
    {
        ret = mix_videoconfigparamsenc_get_CIR_frame_cnt (config_params_enc, &(this->CIR_frame_cnt));
        if (ret != MIX_RESULT_SUCCESS) {
            //TODO cleanup

            LOG_E("Failed to mix_videoconfigparamsenc_get_CIR_frame_cnt\n");
            this->Unlock();
            return MIX_RESULT_FAIL;
        }
    }
    break;

    default:
        break;
    }
    this->Unlock();
    return MIX_RESULT_SUCCESS;
}

/* mixvideoformatenc class methods implementation */

MIX_RESULT mix_videofmtenc_getcaps(MixVideoFormatEnc *mix, char *msg) {
    LOG_V( "Begin\n");
    if (NULL != mix)
        return mix->GetCaps(msg);
    else
        return MIX_RESULT_NOTIMPL;
}

MIX_RESULT mix_videofmtenc_initialize(
    MixVideoFormatEnc *mix,
    MixVideoConfigParamsEnc * config_params_enc,
    MixFrameManager * frame_mgr,
    MixBufferPool * input_buf_pool,
    MixSurfacePool ** surface_pool,
    MixUsrReqSurfacesInfo * requested_surface_info,
    VADisplay va_display) {

    if (NULL != mix)
        return mix->Initialize(
                   config_params_enc,
                   frame_mgr,
                   input_buf_pool,
                   surface_pool,
                   requested_surface_info,
                   va_display);
    else
        return MIX_RESULT_FAIL;
}

MIX_RESULT mix_videofmtenc_encode(
    MixVideoFormatEnc *mix, MixBuffer * bufin[],
    int bufincnt, MixIOVec * iovout[], int iovoutcnt,
    MixVideoEncodeParams * encode_params) {
    if (NULL != mix)
        return mix->Encode(bufin, bufincnt, iovout, iovoutcnt, encode_params);
    else
        return MIX_RESULT_FAIL;
}

MIX_RESULT mix_videofmtenc_flush(MixVideoFormatEnc *mix) {
    if (NULL != mix)
        return mix->Flush();
    else
        return MIX_RESULT_FAIL;
}

MIX_RESULT mix_videofmtenc_eos(MixVideoFormatEnc *mix) {
    if (NULL != mix)
        return mix->EndOfStream();
    else
        return MIX_RESULT_FAIL;
}

MIX_RESULT mix_videofmtenc_deinitialize(MixVideoFormatEnc *mix) {
    if (NULL != mix)
        return mix->Deinitialize();
    else
        return MIX_RESULT_FAIL;
}

MIX_RESULT mix_videofmtenc_get_max_coded_buffer_size(
    MixVideoFormatEnc *mix, uint * max_size) {
    if (NULL != mix)
        return mix->GetMaxEncodedBufSize(max_size);
    else
        return MIX_RESULT_FAIL;
}

MIX_RESULT mix_videofmtenc_set_dynamic_enc_config (
    MixVideoFormatEnc * mix,
    MixVideoConfigParamsEnc * config_params_enc,
    MixEncParamsType params_type) {
    if (NULL != mix)
        return mix->SetDynamicEncConfig(config_params_enc, params_type);
    else
        return MIX_RESULT_FAIL;
}
