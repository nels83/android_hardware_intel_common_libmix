/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

/**
 * SECTION:mixvideoconfigparamsenc
 * @short_description: MI-X Video Encode Configuration Parameter Base Object
 *
 * A base object of MI-X video encode configuration parameter objects.
 */


#include <string.h>
#include "mixvideolog.h"
#include "mixvideoconfigparamsenc.h"
#include <stdlib.h>

#define MDEBUG

MixVideoConfigParamsEnc::MixVideoConfigParamsEnc()
        :profile(MIX_PROFILE_H264BASELINE)
        ,level(40)
        ,raw_format(MIX_RAW_TARGET_FORMAT_YUV420)
        ,rate_control(MIX_RATE_CONTROL_NONE)
        ,bitrate(0)
        ,frame_rate_num(30)
        ,frame_rate_denom(1)
        ,initial_qp(15)
        ,min_qp(0)
        ,target_percentage(95)
        ,window_size(500)
        ,intra_period(30)
        ,picture_width(0)
        ,picture_height(0)
        ,mime_type(NULL)
        ,encode_format(MIX_ENCODE_TARGET_FORMAT_MPEG4)
        ,mixbuffer_pool_size(0)
        ,share_buf_mode(FALSE)
        ,ci_frame_id(NULL)
        ,ci_frame_num(0)
        ,CIR_frame_cnt(15)
        ,max_slice_size(0)/*Set to 0 means it won't take effect*/
        ,refresh_type(MIX_VIDEO_NONIR)
        ,buffer_mode(MIX_BUFFER_SELF_ALLOC_SURFACE)
        ,buf_info(NULL)
        ,need_display(TRUE)
        ,reserved1(NULL)
        ,reserved2(NULL)
        ,reserved3(NULL)
        ,reserved4(NULL) {
    air_params.air_MBs = 0;
    air_params.air_threshold = 0;
    air_params.air_auto = 0;
}

MixVideoConfigParamsEnc::~MixVideoConfigParamsEnc() {
    /* free mime_type */
    if (mime_type)
        free(mime_type);

    if (ci_frame_id)
        delete[] ci_frame_id;

    if (buffer_mode == MIX_BUFFER_UPSTREAM_ALLOC_CI) {
        MixCISharedBufferInfo * ci_tmp = NULL;
        if (buf_info) {
            ci_tmp = (MixCISharedBufferInfo *) buf_info;;
            if (ci_tmp->ci_frame_id) {
                delete [] ci_tmp->ci_frame_id;
                ci_tmp->ci_frame_id = NULL;
            }

            delete ci_tmp;
            ci_tmp = NULL;
            buf_info = NULL;
        }
    }
}

bool MixVideoConfigParamsEnc::copy(MixParams *target) const {
    bool ret = FALSE;
    MixVideoConfigParamsEnc *this_target = MIX_VIDEOCONFIGPARAMSENC(target);
    MIX_RESULT mix_result = MIX_RESULT_FAIL;

    LOG_V( "Begin\n");

    if (NULL != this_target) {
        /* copy properties of primitive type */
        this_target->bitrate = bitrate;
        this_target->frame_rate_num = frame_rate_num;
        this_target->frame_rate_denom = frame_rate_denom;
        this_target->initial_qp = initial_qp;
        this_target->min_qp = min_qp;
        this_target->target_percentage = target_percentage;
        this_target->window_size = window_size;
        this_target->max_slice_size = max_slice_size;
        this_target->intra_period = intra_period;
        this_target->picture_width = picture_width;
        this_target->picture_height = picture_height;
        this_target->mixbuffer_pool_size = mixbuffer_pool_size;
        this_target->share_buf_mode = share_buf_mode;
        this_target->encode_format = encode_format;
        this_target->ci_frame_num = ci_frame_num;
        this_target->draw= draw;
        this_target->need_display = need_display;
        this_target->rate_control = rate_control;
        this_target->raw_format = raw_format;
        this_target->profile = profile;
        this_target->level = level;
        this_target->CIR_frame_cnt = CIR_frame_cnt;
        this_target->refresh_type = refresh_type;
        this_target->air_params.air_MBs = air_params.air_MBs;
        this_target->air_params.air_threshold = air_params.air_threshold;
        this_target->air_params.air_auto = air_params.air_auto;
        this_target->buffer_mode = buffer_mode;

        /* copy properties of non-primitive */
        /* copy mime_type */
        if (mime_type) {
#ifdef MDEBUG
            LOG_I( "mime_type = %s  %x\n", mime_type, (unsigned int)mime_type);
#endif
            mix_result = mix_videoconfigparamsenc_set_mime_type(
                             this_target,mime_type);
        } else {
            LOG_I( "mime_type = NULL\n");
            mix_result = mix_videoconfigparamsenc_set_mime_type(this_target, NULL);
        }

        if (mix_result != MIX_RESULT_SUCCESS) {
            LOG_E( "Failed to mix_videoconfigparamsenc_set_mime_type\n");
            return FALSE;
        }

        mix_result = mix_videoconfigparamsenc_set_ci_frame_info (
                         this_target, ci_frame_id, ci_frame_num);
        mix_result = mix_videoconfigparamsenc_set_upstream_buffer_info (
                         this_target, this_target->buffer_mode, buf_info);

        /* TODO: copy other properties if there's any */
        /* Now chainup base class */
        ret = MixVideoConfigParams::copy(target);
    }

    return ret;
}

bool MixVideoConfigParamsEnc::equal(MixParams* obj) const {
    bool ret = TRUE;
    MixVideoConfigParamsEnc *this_obj = MIX_VIDEOCONFIGPARAMSENC(obj);
    if (NULL != this_obj) {
        /* check the equalitiy of the primitive type properties */
        if (bitrate != this_obj->bitrate) {
            goto not_equal;
        }

        if (frame_rate_num != this_obj->frame_rate_num) {
            goto not_equal;
        }

        if (frame_rate_denom != this_obj->frame_rate_denom) {
            goto not_equal;
        }

        if (initial_qp != this_obj->initial_qp) {
            goto not_equal;
        }

        if (min_qp != this_obj->min_qp) {
            goto not_equal;
        }

        if (target_percentage != this_obj->target_percentage) {
            goto not_equal;
        }

        if (window_size != this_obj->window_size) {
            goto not_equal;
        }

        if (max_slice_size != this_obj->max_slice_size) {
            goto not_equal;
        }

        if (intra_period != this_obj->intra_period) {
            goto not_equal;
        }

        if (picture_width != this_obj->picture_width &&
                picture_height != this_obj->picture_height) {
            goto not_equal;
        }

        if (encode_format != this_obj->encode_format) {
            goto not_equal;
        }

        if (mixbuffer_pool_size != this_obj->mixbuffer_pool_size) {
            goto not_equal;
        }

        if (share_buf_mode != this_obj->share_buf_mode) {
            goto not_equal;
        }

        if (ci_frame_id != this_obj->ci_frame_id) {
            goto not_equal;
        }

        if (ci_frame_num != this_obj->ci_frame_num) {
            goto not_equal;
        }

        if (draw != this_obj->draw) {
            goto not_equal;
        }

        if (need_display!= this_obj->need_display) {
            goto not_equal;
        }

        if (rate_control != this_obj->rate_control) {
            goto not_equal;
        }

        if (raw_format != this_obj->raw_format) {
            goto not_equal;
        }

        if (profile != this_obj->profile) {
            goto not_equal;
        }

        if (level != this_obj->level) {
            goto not_equal;
        }

        if (CIR_frame_cnt != this_obj->CIR_frame_cnt) {
            goto not_equal;
        }

        if (refresh_type != this_obj->refresh_type) {
            goto not_equal;
        }

        if (air_params.air_MBs != this_obj->air_params.air_MBs) {
            goto not_equal;
        }

        if (air_params.air_threshold != this_obj->air_params.air_threshold) {
            goto not_equal;
        }

        if (air_params.air_auto != this_obj->air_params.air_auto) {
            goto not_equal;
        }

        if (buffer_mode != this_obj->buffer_mode) {
            goto not_equal;
        }

        /* check the equalitiy of the none-primitive type properties */

        /* compare mime_type */
        if (mime_type && this_obj->mime_type) {
            if (strcmp(mime_type, this_obj->mime_type) != 0) {
                goto not_equal;
            }
        } else if (!(!mime_type && !this_obj->mime_type)) {
            goto not_equal;
        }

        /*
        * TODO: Check the data inside data info
        */
        ret = TRUE;

not_equal:

        if (ret != TRUE) {
            return ret;
        }

        /* chaining up. */
        ret = MixVideoConfigParams::equal(obj);

    }
    return ret;

}

MixParams* MixVideoConfigParamsEnc::dup() const {
    MixParams *ret = new MixVideoConfigParamsEnc();
    if (NULL != ret) {
        if (FALSE == copy(ret)) {
            ret->Unref();
            ret = NULL;
        }
    }
    return ret;
}


MixVideoConfigParamsEnc *
mix_videoconfigparamsenc_new(void) {
    return new MixVideoConfigParamsEnc();
}

MixVideoConfigParamsEnc *
mix_videoconfigparamsenc_ref(MixVideoConfigParamsEnc * mix) {
    if (NULL != mix)
        mix->Ref();
    return mix;
}


#define MIX_VIDEOCONFIGPARAMSENC_SETTER_CHECK_INPUT(obj) \
	if(!obj) return MIX_RESULT_NULL_PTR; \
	if(!MIX_IS_VIDEOCONFIGPARAMSENC(obj)) return MIX_RESULT_FAIL; \
 
#define MIX_VIDEOCONFIGPARAMSENC_GETTER_CHECK_INPUT(obj, prop) \
	if(!obj || !prop) return MIX_RESULT_NULL_PTR; \
	if(!MIX_IS_VIDEOCONFIGPARAMSENC(obj)) return MIX_RESULT_FAIL; \
 
#define MIX_VIDEOCONFIGPARAMSENC_GETTER_CHECK_INPUT_PAIR(obj, prop, prop2) \
	if(!obj || !prop || !prop2 ) return MIX_RESULT_NULL_PTR; \
	if(!MIX_IS_VIDEOCONFIGPARAMSENC(obj)) return MIX_RESULT_FAIL; \
 
/* TODO: Add getters and setters for other properties. The following is incomplete */


MIX_RESULT mix_videoconfigparamsenc_set_mime_type(MixVideoConfigParamsEnc * obj,
        const char * mime_type) {

    MIX_VIDEOCONFIGPARAMSENC_SETTER_CHECK_INPUT (obj);

    if (!mime_type) {
        return MIX_RESULT_NULL_PTR;
    }

    LOG_I( "mime_type = %s  %x\n",
           mime_type, (unsigned int)mime_type);

    if (obj->mime_type) {
        free(obj->mime_type);
        obj->mime_type = NULL;
    }


    LOG_I( "mime_type = %s  %x\n",
           mime_type, (unsigned int)mime_type);

    obj->mime_type = strdup(mime_type);
    if (!obj->mime_type) {
        return MIX_RESULT_NO_MEMORY;
    }


    LOG_I( "mime_type = %s obj->mime_type = %s\n",
           mime_type, obj->mime_type);

    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_mime_type(MixVideoConfigParamsEnc * obj,
        char ** mime_type) {
    MIX_VIDEOCONFIGPARAMSENC_GETTER_CHECK_INPUT (obj, mime_type);

    if (!obj->mime_type) {
        *mime_type = NULL;
        return MIX_RESULT_SUCCESS;
    }
    *mime_type = strdup(obj->mime_type);
    if (!*mime_type) {
        return MIX_RESULT_NO_MEMORY;
    }

    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_set_frame_rate(MixVideoConfigParamsEnc * obj,
        uint frame_rate_num, uint frame_rate_denom) {
    MIX_VIDEOCONFIGPARAMSENC_SETTER_CHECK_INPUT (obj);
    obj->frame_rate_num = frame_rate_num;
    obj->frame_rate_denom = frame_rate_denom;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_frame_rate(MixVideoConfigParamsEnc * obj,
        uint * frame_rate_num, uint * frame_rate_denom) {
    MIX_VIDEOCONFIGPARAMSENC_GETTER_CHECK_INPUT_PAIR (obj, frame_rate_num, frame_rate_denom);
    *frame_rate_num = obj->frame_rate_num;
    *frame_rate_denom = obj->frame_rate_denom;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_set_picture_res(MixVideoConfigParamsEnc * obj,
        uint picture_width, uint picture_height) {
    MIX_VIDEOCONFIGPARAMSENC_SETTER_CHECK_INPUT (obj);
    obj->picture_width = picture_width;
    obj->picture_height = picture_height;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_picture_res(MixVideoConfigParamsEnc * obj,
        uint * picture_width, uint * picture_height) {
    MIX_VIDEOCONFIGPARAMSENC_GETTER_CHECK_INPUT_PAIR (obj, picture_width, picture_height);
    *picture_width = obj->picture_width;
    *picture_height = obj->picture_height;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_set_encode_format(MixVideoConfigParamsEnc * obj,
        MixEncodeTargetFormat encode_format) {
    MIX_VIDEOCONFIGPARAMSENC_SETTER_CHECK_INPUT (obj);
    obj->encode_format = encode_format;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_encode_format (MixVideoConfigParamsEnc * obj,
        MixEncodeTargetFormat* encode_format) {
    MIX_VIDEOCONFIGPARAMSENC_GETTER_CHECK_INPUT (obj, encode_format);
    *encode_format = obj->encode_format;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_set_bit_rate (MixVideoConfigParamsEnc * obj,
        uint bitrate) {
    MIX_VIDEOCONFIGPARAMSENC_SETTER_CHECK_INPUT (obj);
    obj->bitrate= bitrate;
    return MIX_RESULT_SUCCESS;

}

MIX_RESULT mix_videoconfigparamsenc_get_bit_rate (MixVideoConfigParamsEnc * obj,
        uint *bitrate) {
    MIX_VIDEOCONFIGPARAMSENC_GETTER_CHECK_INPUT (obj, bitrate);
    *bitrate = obj->bitrate;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_set_init_qp (MixVideoConfigParamsEnc * obj,
        uint initial_qp) {
    MIX_VIDEOCONFIGPARAMSENC_SETTER_CHECK_INPUT (obj);
    obj->initial_qp = initial_qp;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_init_qp (MixVideoConfigParamsEnc * obj,
        uint *initial_qp) {
    MIX_VIDEOCONFIGPARAMSENC_GETTER_CHECK_INPUT (obj, initial_qp);
    *initial_qp = obj->initial_qp;
    return MIX_RESULT_SUCCESS;

}

MIX_RESULT mix_videoconfigparamsenc_set_min_qp (MixVideoConfigParamsEnc * obj,
        uint min_qp) {
    MIX_VIDEOCONFIGPARAMSENC_SETTER_CHECK_INPUT (obj);
    obj->min_qp = min_qp;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_min_qp(MixVideoConfigParamsEnc * obj,
        uint *min_qp) {
    MIX_VIDEOCONFIGPARAMSENC_GETTER_CHECK_INPUT (obj, min_qp);
    *min_qp = obj->min_qp;

    return MIX_RESULT_SUCCESS;
}


MIX_RESULT mix_videoconfigparamsenc_set_target_percentage (MixVideoConfigParamsEnc * obj,
        uint target_percentage) {

    MIX_VIDEOCONFIGPARAMSENC_SETTER_CHECK_INPUT (obj);
    obj->target_percentage = target_percentage;
    return MIX_RESULT_SUCCESS;
}


MIX_RESULT mix_videoconfigparamsenc_get_target_percentage(MixVideoConfigParamsEnc * obj,
        uint *target_percentage) {

    MIX_VIDEOCONFIGPARAMSENC_GETTER_CHECK_INPUT (obj, target_percentage);
    *target_percentage = obj->target_percentage;

    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_set_window_size (MixVideoConfigParamsEnc * obj,
        uint window_size) {

    MIX_VIDEOCONFIGPARAMSENC_SETTER_CHECK_INPUT (obj);
    obj->window_size = window_size;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_window_size (MixVideoConfigParamsEnc * obj,
        uint *window_size) {

    MIX_VIDEOCONFIGPARAMSENC_GETTER_CHECK_INPUT (obj, window_size);
    *window_size = obj->window_size;

    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_set_intra_period (MixVideoConfigParamsEnc * obj,
        uint intra_period) {
    MIX_VIDEOCONFIGPARAMSENC_SETTER_CHECK_INPUT (obj);
    obj->intra_period = intra_period;

    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_intra_period (MixVideoConfigParamsEnc * obj,
        uint *intra_period) {
    MIX_VIDEOCONFIGPARAMSENC_GETTER_CHECK_INPUT (obj, intra_period);
    *intra_period = obj->intra_period;

    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_set_buffer_pool_size(
    MixVideoConfigParamsEnc * obj, uint bufpoolsize) {

    MIX_VIDEOCONFIGPARAMSENC_SETTER_CHECK_INPUT (obj);

    obj->mixbuffer_pool_size = bufpoolsize;
    return MIX_RESULT_SUCCESS;

}

MIX_RESULT mix_videoconfigparamsenc_get_buffer_pool_size(
    MixVideoConfigParamsEnc * obj, uint *bufpoolsize) {

    MIX_VIDEOCONFIGPARAMSENC_GETTER_CHECK_INPUT (obj, bufpoolsize);
    *bufpoolsize = obj->mixbuffer_pool_size;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_set_share_buf_mode (
    MixVideoConfigParamsEnc * obj, bool share_buf_mod) {
    MIX_VIDEOCONFIGPARAMSENC_SETTER_CHECK_INPUT (obj);

    obj->share_buf_mode = share_buf_mod;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_share_buf_mode(MixVideoConfigParamsEnc * obj,
        bool *share_buf_mod) {
    MIX_VIDEOCONFIGPARAMSENC_GETTER_CHECK_INPUT (obj, share_buf_mod);

    *share_buf_mod = obj->share_buf_mode;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_set_ci_frame_info(
    MixVideoConfigParamsEnc * obj, ulong * ci_frame_id, uint ci_frame_num) {
    MIX_VIDEOCONFIGPARAMSENC_SETTER_CHECK_INPUT (obj);

    if (!ci_frame_id || !ci_frame_num) {
        obj->ci_frame_id = NULL;
        obj->ci_frame_num = 0;
        return MIX_RESULT_SUCCESS;
    }

    if (obj->ci_frame_id)
        delete [] obj->ci_frame_id;

    uint size = ci_frame_num * sizeof (ulong);
    obj->ci_frame_num = ci_frame_num;

    obj->ci_frame_id = new ulong[ci_frame_num];
    if (!(obj->ci_frame_id)) {
        return MIX_RESULT_NO_MEMORY;
    }

    memcpy (obj->ci_frame_id, ci_frame_id, size);

    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_ci_frame_info (MixVideoConfigParamsEnc * obj,
        ulong * *ci_frame_id, uint *ci_frame_num) {
    MIX_VIDEOCONFIGPARAMSENC_GETTER_CHECK_INPUT_PAIR (obj, ci_frame_id, ci_frame_num);

    *ci_frame_num = obj->ci_frame_num;

    if (!obj->ci_frame_id) {
        *ci_frame_id = NULL;
        return MIX_RESULT_SUCCESS;
    }

    if (obj->ci_frame_num) {
        *ci_frame_id = new ulong[obj->ci_frame_num];

        if (!*ci_frame_id) {
            return MIX_RESULT_NO_MEMORY;
        }

        memcpy (*ci_frame_id, obj->ci_frame_id, obj->ci_frame_num * sizeof (ulong));

    } else {
        *ci_frame_id = NULL;
    }

    return MIX_RESULT_SUCCESS;
}


MIX_RESULT mix_videoconfigparamsenc_set_drawable (MixVideoConfigParamsEnc * obj,
        ulong draw) {

    MIX_VIDEOCONFIGPARAMSENC_SETTER_CHECK_INPUT (obj);
    obj->draw = draw;
    return MIX_RESULT_SUCCESS;

}

MIX_RESULT mix_videoconfigparamsenc_get_drawable (MixVideoConfigParamsEnc * obj,
        ulong *draw) {

    MIX_VIDEOCONFIGPARAMSENC_GETTER_CHECK_INPUT (obj, draw);
    *draw = obj->draw;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_set_need_display (
    MixVideoConfigParamsEnc * obj, bool need_display) {
    MIX_VIDEOCONFIGPARAMSENC_SETTER_CHECK_INPUT (obj);

    obj->need_display = need_display;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_need_display(MixVideoConfigParamsEnc * obj,
        bool *need_display) {
    MIX_VIDEOCONFIGPARAMSENC_GETTER_CHECK_INPUT (obj, need_display);

    *need_display = obj->need_display;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_set_rate_control(MixVideoConfigParamsEnc * obj,
        MixRateControl rate_control) {
    MIX_VIDEOCONFIGPARAMSENC_SETTER_CHECK_INPUT (obj);
    obj->rate_control = rate_control;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_rate_control(MixVideoConfigParamsEnc * obj,
        MixRateControl * rate_control) {
    MIX_VIDEOCONFIGPARAMSENC_GETTER_CHECK_INPUT (obj, rate_control);
    *rate_control = obj->rate_control;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_set_raw_format (MixVideoConfigParamsEnc * obj,
        MixRawTargetFormat raw_format) {
    MIX_VIDEOCONFIGPARAMSENC_SETTER_CHECK_INPUT (obj);
    obj->raw_format = raw_format;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_raw_format (MixVideoConfigParamsEnc * obj,
        MixRawTargetFormat * raw_format) {
    MIX_VIDEOCONFIGPARAMSENC_GETTER_CHECK_INPUT (obj, raw_format);
    *raw_format = obj->raw_format;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_set_profile (MixVideoConfigParamsEnc * obj,
        MixProfile profile) {
    MIX_VIDEOCONFIGPARAMSENC_SETTER_CHECK_INPUT (obj);
    obj->profile = profile;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_profile (MixVideoConfigParamsEnc * obj,
        MixProfile * profile) {
    MIX_VIDEOCONFIGPARAMSENC_GETTER_CHECK_INPUT (obj, profile);
    *profile = obj->profile;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_set_level (MixVideoConfigParamsEnc * obj,
        uint8 level) {
    MIX_VIDEOCONFIGPARAMSENC_SETTER_CHECK_INPUT (obj);
    obj->level = level;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_level (MixVideoConfigParamsEnc * obj,
        uint8 * level) {
    MIX_VIDEOCONFIGPARAMSENC_GETTER_CHECK_INPUT (obj, level);
    *level = obj->level;
    return MIX_RESULT_SUCCESS;
}


MIX_RESULT mix_videoconfigparamsenc_set_CIR_frame_cnt (MixVideoConfigParamsEnc * obj,
        uint CIR_frame_cnt) {
    MIX_VIDEOCONFIGPARAMSENC_SETTER_CHECK_INPUT (obj);
    obj->CIR_frame_cnt = CIR_frame_cnt;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_CIR_frame_cnt (MixVideoConfigParamsEnc * obj,
        uint * CIR_frame_cnt) {
    MIX_VIDEOCONFIGPARAMSENC_GETTER_CHECK_INPUT (obj, CIR_frame_cnt);
    *CIR_frame_cnt = obj->CIR_frame_cnt;
    return MIX_RESULT_SUCCESS;
}


MIX_RESULT mix_videoconfigparamsenc_set_max_slice_size (MixVideoConfigParamsEnc * obj,
        uint max_slice_size) {
    MIX_VIDEOCONFIGPARAMSENC_SETTER_CHECK_INPUT (obj);
    obj->max_slice_size = max_slice_size;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_max_slice_size (MixVideoConfigParamsEnc * obj,
        uint * max_slice_size) {
    MIX_VIDEOCONFIGPARAMSENC_GETTER_CHECK_INPUT (obj, max_slice_size);
    *max_slice_size = obj->max_slice_size;
    return MIX_RESULT_SUCCESS;
}


MIX_RESULT mix_videoconfigparamsenc_set_refresh_type(MixVideoConfigParamsEnc * obj,
        MixVideoIntraRefreshType refresh_type) {
    MIX_VIDEOCONFIGPARAMSENC_SETTER_CHECK_INPUT (obj);
    obj->refresh_type = refresh_type;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_refresh_type (MixVideoConfigParamsEnc * obj,
        MixVideoIntraRefreshType * refresh_type) {
    MIX_VIDEOCONFIGPARAMSENC_GETTER_CHECK_INPUT (obj, refresh_type);
    *refresh_type = obj->refresh_type;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_set_AIR_params (MixVideoConfigParamsEnc * obj,
        MixAIRParams air_params) {
    MIX_VIDEOCONFIGPARAMSENC_SETTER_CHECK_INPUT (obj);
    obj->air_params.air_MBs = air_params.air_MBs;
    obj->air_params.air_threshold = air_params.air_threshold;
    obj->air_params.air_auto = air_params.air_auto;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_AIR_params (MixVideoConfigParamsEnc * obj,
        MixAIRParams * air_params) {
    MIX_VIDEOCONFIGPARAMSENC_GETTER_CHECK_INPUT (obj, air_params);
    air_params->air_MBs = obj->air_params.air_MBs;
    air_params->air_threshold = obj->air_params.air_threshold;
    air_params->air_auto = obj->air_params.air_auto;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_set_buffer_mode (MixVideoConfigParamsEnc * obj,
        MixBufferAllocationMode buffer_mode) {
    MIX_VIDEOCONFIGPARAMSENC_SETTER_CHECK_INPUT (obj);
    obj->buffer_mode = buffer_mode;
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_buffer_mode (MixVideoConfigParamsEnc * obj,
        MixBufferAllocationMode * buffer_mode) {
    MIX_VIDEOCONFIGPARAMSENC_GETTER_CHECK_INPUT (obj, buffer_mode);
    *buffer_mode = obj->buffer_mode;
    return MIX_RESULT_SUCCESS;
}

/*
* Currently we use void* for buf_info, and will change to use union later which has been defined in mixvideodef.h
*/
MIX_RESULT mix_videoconfigparamsenc_set_upstream_buffer_info (MixVideoConfigParamsEnc * obj,
        MixBufferAllocationMode buffer_mode, void * buf_info) {

    MIX_VIDEOCONFIGPARAMSENC_SETTER_CHECK_INPUT (obj);

    if (!buf_info) {
        return MIX_RESULT_NULL_PTR;
    }

    switch (buffer_mode) {
    case MIX_BUFFER_UPSTREAM_ALLOC_CI:
    {
        MixCISharedBufferInfo * ci_tmp = NULL;
        MixCISharedBufferInfo * ci_info_in = (MixCISharedBufferInfo *) buf_info;

        if (obj->buf_info) {
            ci_tmp = (MixCISharedBufferInfo *) obj->buf_info;;
            if (ci_tmp->ci_frame_id) {
                delete [] ci_tmp->ci_frame_id;
                ci_tmp->ci_frame_id = NULL;
            }

            delete ci_tmp;
            ci_tmp = NULL;
            obj->buf_info = NULL;
        }

        ci_tmp = new MixCISharedBufferInfo;
        if (!ci_tmp) {
            return MIX_RESULT_NO_MEMORY;
        }

        ci_tmp->ci_frame_cnt = ci_info_in->ci_frame_cnt;
        ci_tmp->ci_frame_id = NULL;

        ci_tmp->ci_frame_id = new ulong[ci_tmp->ci_frame_cnt];
        if (!ci_tmp->ci_frame_id) {
            return MIX_RESULT_NO_MEMORY;
        }

        memcpy (ci_tmp->ci_frame_id, ci_info_in->ci_frame_id, ci_tmp->ci_frame_cnt * sizeof (ulong));
        obj->buf_info = (void *) ci_tmp;
    }
    break;
    case MIX_BUFFER_UPSTREAM_ALLOC_V4L2:
        break;
    case MIX_BUFFER_UPSTREAM_ALLOC_SURFACE:
        break;
    default:
        return MIX_RESULT_FAIL;  //FIXEME
    }
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videoconfigparamsenc_get_upstream_buffer_info (MixVideoConfigParamsEnc * obj,
        MixBufferAllocationMode buffer_mode, void ** buf_info) {
    MIX_VIDEOCONFIGPARAMSENC_GETTER_CHECK_INPUT (obj, buf_info);

    switch (buffer_mode) {
    case MIX_BUFFER_UPSTREAM_ALLOC_CI:
    {
        MixCISharedBufferInfo * ci_tmp = (MixCISharedBufferInfo *) (obj->buf_info);
        MixCISharedBufferInfo * ci_info_out = NULL;

        if (!ci_tmp) {
            return MIX_RESULT_NULL_PTR;
        }

        if (!(ci_tmp->ci_frame_id) || !(ci_tmp->ci_frame_cnt)) {
            return MIX_RESULT_NULL_PTR;
        }

        ci_info_out = new MixCISharedBufferInfo;
        if (!ci_info_out) {
            return MIX_RESULT_NO_MEMORY;
        }

        ci_info_out->ci_frame_cnt = ci_tmp->ci_frame_cnt;
        ci_info_out->ci_frame_id = NULL;

        ci_info_out->ci_frame_id = new ulong[ci_info_out->ci_frame_cnt];
        if (!ci_info_out->ci_frame_id) {
            return MIX_RESULT_NO_MEMORY;
        }

        memcpy (ci_info_out->ci_frame_id, ci_tmp->ci_frame_id, ci_info_out->ci_frame_cnt * sizeof (ulong));
        *buf_info = (MixCISharedBufferInfo *) ci_info_out;
    }
    break;
    case MIX_BUFFER_UPSTREAM_ALLOC_V4L2:
        break;
    case MIX_BUFFER_UPSTREAM_ALLOC_SURFACE:
        break;
    default:
        return MIX_RESULT_FAIL;  //FIXME
    }
    return MIX_RESULT_SUCCESS;
}

