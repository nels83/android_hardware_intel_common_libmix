/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

#include "mixvideolog.h"

#include "mixvideoformat.h"
#include <string.h>
#include <stdlib.h>

#define MIXUNREF(obj, unref) if(obj) { unref(obj); obj = NULL; }

MixVideoFormat::MixVideoFormat()
    :mLock()
    ,initialized(FALSE)
    ,va_initialized(FALSE)
    ,framemgr(NULL)
    ,surfacepool(NULL)
    ,inputbufpool(NULL)
    ,inputbufqueue(NULL)
    ,va_display(NULL)
    ,va_context(VA_INVALID_ID)
    ,va_config(VA_INVALID_ID)
    ,va_surfaces(NULL)
    ,va_num_surfaces(0)
    ,mime_type(NULL)
    ,frame_rate_num(0)
    ,frame_rate_denom(0)
    ,picture_width(0)
    ,picture_height(0)
    ,parse_in_progress(FALSE)
    ,current_timestamp((uint64)-1)
    ,end_picture_pending(FALSE)
    ,video_frame(NULL)
    ,extra_surfaces(0)
    ,config_params(NULL)
    ,error_concealment(TRUE)
    ,ref_count(1)
{
}

MixVideoFormat::~MixVideoFormat() {
    /* clean up here. */
    VAStatus va_status;
    MixInputBufferEntry *buf_entry = NULL;

    if (this->mime_type) {
        free(this->mime_type);
    }

    //MiVideo object calls the _deinitialize() for frame manager
    MIXUNREF(this->framemgr, mix_framemanager_unref);

    if (this->surfacepool) {
        mix_surfacepool_deinitialize(this->surfacepool);
        MIXUNREF(this->surfacepool, mix_surfacepool_unref);
    }

    if (this->config_params) {
        mix_videoconfigparams_unref(this->config_params);
        this->config_params = NULL;
    }

    //libVA cleanup (vaTerminate is called from MixVideo object)
    if (this->va_display) {
        if (this->va_context != VA_INVALID_ID) {
            va_status = vaDestroyContext(this->va_display, this->va_context);
            if (va_status != VA_STATUS_SUCCESS) {
                LOG_W( "Failed vaDestroyContext\n");
            }
            this->va_context = VA_INVALID_ID;
        }
        if (this->va_config != VA_INVALID_ID) {
            va_status = vaDestroyConfig(this->va_display, this->va_config);
            if (va_status != VA_STATUS_SUCCESS) {
                LOG_W( "Failed vaDestroyConfig\n");
            }
            this->va_config = VA_INVALID_ID;
        }
        if (this->va_surfaces) {
            va_status = vaDestroySurfaces(this->va_display, this->va_surfaces, this->va_num_surfaces);
            if (va_status != VA_STATUS_SUCCESS) {
                LOG_W( "Failed vaDestroySurfaces\n");
            }
            free(this->va_surfaces);
            this->va_surfaces = NULL;
            this->va_num_surfaces = 0;
        }
    }

    if (this->video_frame) {
        mix_videoframe_unref(this->video_frame);
        this->video_frame = NULL;
    }

    //Deinit input buffer queue
    while (!j_queue_is_empty(this->inputbufqueue)) {
        buf_entry = reinterpret_cast<MixInputBufferEntry*>(j_queue_pop_head(this->inputbufqueue));
        mix_buffer_unref(buf_entry->buf);
        free(buf_entry);
    }

    j_queue_free(this->inputbufqueue);

    //MixBuffer pool is deallocated in MixVideo object
    this->inputbufpool = NULL;
}

MIX_RESULT MixVideoFormat::GetCaps(char *msg) {
    LOG_V("mix_videofmt_getcaps_default\n");
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT MixVideoFormat::Initialize(
    MixVideoConfigParamsDec * config_params,
    MixFrameManager * frame_mgr,
    MixBufferPool * input_buf_pool,
    MixSurfacePool ** surface_pool,
    VADisplay va_display) {

    LOG_V(	"Begin\n");
    MIX_RESULT res = MIX_RESULT_SUCCESS;
    MixInputBufferEntry *buf_entry = NULL;

    if (!config_params || !frame_mgr || !input_buf_pool || !surface_pool || !va_display) {
        LOG_E( "NUll pointer passed in\n");
        return (MIX_RESULT_NULL_PTR);
    }

    Lock();

    //Clean up any previous framemgr
    MIXUNREF(this->framemgr, mix_framemanager_unref);
    this->framemgr = frame_mgr;
    mix_framemanager_ref(this->framemgr);
    if (this->config_params) {
        mix_videoconfigparams_unref(this->config_params);
    }
    this->config_params = config_params;
    mix_videoconfigparams_ref(reinterpret_cast<MixVideoConfigParams*>(this->config_params));

    this->va_display = va_display;

    //Clean up any previous mime_type
    if (this->mime_type) {
        free(this->mime_type);
        this->mime_type = NULL;
    }

    res = mix_videoconfigparamsdec_get_mime_type(config_params, &this->mime_type);
    if (NULL == this->mime_type) {
        res = MIX_RESULT_NO_MEMORY;
        LOG_E( "Could not duplicate mime_type\n");
        goto cleanup;
    }//else there is no mime_type; leave as NULL

    res = mix_videoconfigparamsdec_get_frame_rate(config_params, &(this->frame_rate_num), &(this->frame_rate_denom));
    if (res != MIX_RESULT_SUCCESS) {
        LOG_E( "Error getting frame_rate\n");
        goto cleanup;
    }
    res = mix_videoconfigparamsdec_get_picture_res(config_params, &(this->picture_width), &(this->picture_height));
    if (res != MIX_RESULT_SUCCESS) {
        LOG_E( "Error getting picture_res\n");
        goto cleanup;
    }

    if (this->inputbufqueue) {
        //Deinit previous input buffer queue
        while (!j_queue_is_empty(this->inputbufqueue)) {
            buf_entry = reinterpret_cast<MixInputBufferEntry*>(j_queue_pop_head(this->inputbufqueue));
            mix_buffer_unref(buf_entry->buf);
            free(buf_entry);
        }
        j_queue_free(this->inputbufqueue);
    }

    //MixBuffer pool is cleaned up in MixVideo object
    this->inputbufpool = NULL;

    this->inputbufpool = input_buf_pool;
    this->inputbufqueue = j_queue_new();
    if (NULL == this->inputbufqueue) {//New failed
        res = MIX_RESULT_NO_MEMORY;
        LOG_E( "Could not duplicate mime_type\n");
        goto cleanup;
    }

    // surface pool, VA context/config and parser handle are initialized by
    // derived classes


cleanup:
    if (res != MIX_RESULT_SUCCESS) {
        MIXUNREF(this->framemgr, mix_framemanager_unref);
        if (this->mime_type) {
            free(this->mime_type);
            this->mime_type = NULL;
        }
        Unlock();
        this->frame_rate_num = 0;
        this->frame_rate_denom = 1;
        this->picture_width = 0;
        this->picture_height = 0;
    } else {//Normal unlock
        Unlock();
    }

    LOG_V( "End\n");

    return res;

}

MIX_RESULT MixVideoFormat::Decode(
    MixBuffer * bufin[], int bufincnt,
    MixVideoDecodeParams * decode_params) {
    return MIX_RESULT_SUCCESS;
}
MIX_RESULT MixVideoFormat::Flush() {
    return MIX_RESULT_SUCCESS;
}
MIX_RESULT MixVideoFormat::EndOfStream() {
    return MIX_RESULT_SUCCESS;
}
MIX_RESULT MixVideoFormat::Deinitialize() {
    return MIX_RESULT_SUCCESS;
}


MixVideoFormat* mix_videoformat_unref(MixVideoFormat* mix) {
    if (NULL != mix)
        return mix->Unref();
    else
        return NULL;
}

MixVideoFormat * mix_videoformat_new(void) {
    return new MixVideoFormat();
}

MixVideoFormat * mix_videoformat_ref(MixVideoFormat * mix) {
    if (NULL != mix)
        mix->Ref();
    return mix;
}


/* mixvideoformat class methods implementation */
MIX_RESULT mix_videofmt_getcaps(MixVideoFormat *mix, char *msg) {
    return mix->GetCaps(msg);
}

MIX_RESULT mix_videofmt_initialize(
    MixVideoFormat *mix,
    MixVideoConfigParamsDec * config_params,
    MixFrameManager * frame_mgr,
    MixBufferPool * input_buf_pool,
    MixSurfacePool ** surface_pool,
    VADisplay va_display) {
    return mix->Initialize(config_params, frame_mgr,
                           input_buf_pool, surface_pool, va_display);
}

MIX_RESULT mix_videofmt_decode(
    MixVideoFormat *mix, MixBuffer * bufin[],
    int bufincnt, MixVideoDecodeParams * decode_params) {
    return mix->Decode(bufin, bufincnt, decode_params);
}

MIX_RESULT mix_videofmt_flush(MixVideoFormat *mix) {
    return mix->Flush();
}

MIX_RESULT mix_videofmt_eos(MixVideoFormat *mix) {
    return mix->EndOfStream();
}

MIX_RESULT mix_videofmt_deinitialize(MixVideoFormat *mix) {
    return mix->Deinitialize();
}
