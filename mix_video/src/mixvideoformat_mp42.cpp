/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

#include <string.h>
#include "mixvideolog.h"
#include "mixvideoformat_mp42.h"

// Value of VOP type defined here follows MP4 spec, and has the same value of corresponding frame type
// defined in enumeration MixFrameType (except sprite (S))
enum {
    MP4_VOP_TYPE_I = 0,
    MP4_VOP_TYPE_P = 1,
    MP4_VOP_TYPE_B = 2,
    MP4_VOP_TYPE_S = 3,
};

MixVideoFormat_MP42::MixVideoFormat_MP42()
        :last_frame(NULL)
        ,last_vop_coding_type(-1)
        ,last_vop_time_increment(0)
        ,next_nvop_for_PB_frame(FALSE)
        ,iq_matrix_buf_sent(FALSE) {
    this->reference_frames[0] = NULL;
    this->reference_frames[1] = NULL;
}

MixVideoFormat_MP42::~MixVideoFormat_MP42() {
    /* clean up here. */
    int32 vbp_ret = VBP_OK;
    LOG_V("Begin\n");
    //surfacepool is deallocated by parent
    //inputbufqueue is deallocated by parent
    //parent calls vaDestroyConfig, vaDestroyContext and vaDestroySurfaces

    Lock();

    /* unref reference frames */
    for (uint32 idx = 0; idx < 2; idx++) {
        if (this->reference_frames[idx] != NULL) {
            mix_videoframe_unref(this->reference_frames[idx]);
            this->reference_frames[idx] = NULL;
        }
    }
    if (this->last_frame) {
        mix_videoframe_unref(this->last_frame);
        this->last_frame = NULL;
    }
    this->next_nvop_for_PB_frame = FALSE;
    this->iq_matrix_buf_sent = FALSE;

    /* Reset state */
    this->initialized = TRUE;
    this->end_picture_pending = FALSE;
    this->discontinuity_frame_in_progress = FALSE;
    this->current_timestamp = (uint64)-1;

    /* Close the parser */
    if (this->parser_handle) {
        vbp_ret = vbp_close(this->parser_handle);
        this->parser_handle = NULL;
    }

    Unlock();
    LOG_V("End\n");
}


MixVideoFormat_MP42 *mix_videoformat_mp42_new(void) {
    return new MixVideoFormat_MP42;
}

MixVideoFormat_MP42 * mix_videoformat_mp42_ref(MixVideoFormat_MP42 * mix) {
    if (NULL != mix)
        mix->Ref();
    return mix;
}

MixVideoFormat_MP42 *mix_videoformat_mp42_unref(MixVideoFormat_MP42 * mix) {
    if (NULL != mix)
        return MIX_VIDEOFORMAT_MP42(mix->Unref());
    else
        return mix;
}

MIX_RESULT MixVideoFormat_MP42::_update_config_params(
    vbp_data_mp42 *data) {
    if (this->picture_width == 0 ||
            this->picture_height == 0 ||
            this->picture_width < data->codec_data.video_object_layer_width ||
            this->picture_height < data->codec_data.video_object_layer_height) {
        this->picture_width = data->codec_data.video_object_layer_width;
        this->picture_height = data->codec_data.video_object_layer_height;
        mix_videoconfigparamsdec_set_picture_res(
            this->config_params, this->picture_width, this->picture_height);
    }
    // video_range has default value of 0. Y ranges from 16 to 235.
    mix_videoconfigparamsdec_set_video_range(this->config_params, data->codec_data.video_range);
    uint8  color_matrix;
    switch (data->codec_data.matrix_coefficients) {
    case 1:
        color_matrix = VA_SRC_BT709;
        break;
        // ITU-R Recommendation BT.470-6 System B, G (MP4), same as
        // SMPTE 170M/BT601
    case 5:
    case 6:
        color_matrix = VA_SRC_BT601;
        break;
    default:
        // unknown color matrix, set to 0 so color space flag will not be set.
        color_matrix = 0;
        break;
    }
    mix_videoconfigparamsdec_set_color_matrix(this->config_params, color_matrix);

    mix_videoconfigparamsdec_set_pixel_aspect_ratio(
        this->config_params, data->codec_data.par_width, data->codec_data.par_height);

    mix_videoconfigparamsdec_set_bit_rate(
        this->config_params,
        data->codec_data.bit_rate);

    return MIX_RESULT_SUCCESS;
}

MIX_RESULT MixVideoFormat_MP42::_initialize_va(vbp_data_mp42 *data) {
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    VAStatus vret = VA_STATUS_SUCCESS;
    VAConfigAttrib attrib;
    VAProfile va_profile;
    LOG_V( "Begin\n");
    if (this->va_initialized) {
        LOG_W("va already initialized.\n");
        return MIX_RESULT_SUCCESS;
    }

    //We are requesting RT attributes
    attrib.type = VAConfigAttribRTFormat;
    attrib.value = VA_RT_FORMAT_YUV420;

    //Initialize and save the VA config ID
    if ((data->codec_data.profile_and_level_indication & 0xF8) == 0xF0) {
        va_profile = VAProfileMPEG4AdvancedSimple;
    } else {
        va_profile = VAProfileMPEG4Simple;
    }
    vret = vaCreateConfig(
               this->va_display,
               va_profile,
               VAEntrypointVLD,
               &attrib,
               1,
               &(this->va_config));

    if (vret != VA_STATUS_SUCCESS) {
        ret = MIX_RESULT_FAIL;
        LOG_E("vaCreateConfig failed\n");
        goto CLEAN_UP;
    }

    // add 1 more surface for packed frame (PB frame), and another one
    // for partial frame handling
    this->va_num_surfaces = this->extra_surfaces + 4 + 1 + 1;
    //if (parent->va_num_surfaces > MIX_VIDEO_MP42_SURFACE_NUM)
    //	parent->va_num_surfaces = MIX_VIDEO_MP42_SURFACE_NUM;

    this->va_surfaces = reinterpret_cast<VASurfaceID*>(malloc(sizeof(VASurfaceID)*this->va_num_surfaces));
    if (this->va_surfaces == NULL) {
        ret = MIX_RESULT_FAIL;
        LOG_E( "parent->va_surfaces == NULL. \n");
        goto CLEAN_UP;
    }

    vret = vaCreateSurfaces(
               this->va_display,
               this->picture_width,
               this->picture_height,
               VA_RT_FORMAT_YUV420,
               this->va_num_surfaces,
               this->va_surfaces);

    if (vret != VA_STATUS_SUCCESS) {
        ret = MIX_RESULT_FAIL;
        LOG_E( "Error allocating surfaces\n");
        goto CLEAN_UP;
    }

    LOG_V( "Created %d libva surfaces\n", this->va_num_surfaces);

    //Initialize the surface pool
    ret = mix_surfacepool_initialize(
              this->surfacepool,
              this->va_surfaces,
              this->va_num_surfaces,
              this->va_display);

    switch (ret)
    {
    case MIX_RESULT_SUCCESS:
        break;
    case MIX_RESULT_ALREADY_INIT:  //This case is for future use when we can be  initialized multiple times.  It is to detect when we have not been reset before re-initializing.
    default:
        ret = MIX_RESULT_ALREADY_INIT;
        LOG_E( "Error init surface pool\n");
        goto CLEAN_UP;
        break;
    }

    //Initialize and save the VA context ID
    //Note: VA_PROGRESSIVE libva flag is only relevant to MPEG2
    vret = vaCreateContext(
               this->va_display,
               this->va_config,
               this->picture_width,
               this->picture_height,
               0,
               this->va_surfaces,
               this->va_num_surfaces,
               &(this->va_context));

    if (vret != VA_STATUS_SUCCESS) {
        ret = MIX_RESULT_FAIL;
        LOG_E( "Error initializing video driver\n");
        goto CLEAN_UP;
    }
    this->va_initialized = TRUE;

CLEAN_UP:
    return ret;
}

MIX_RESULT MixVideoFormat_MP42::_decode_a_slice(
    vbp_data_mp42* data, vbp_picture_data_mp42* pic_data) {
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    VAStatus vret = VA_STATUS_SUCCESS;
    VADisplay vadisplay = NULL;
    VAContextID vacontext;
    uint buffer_id_cnt = 0;
    int frame_type = -1;
    // maximum 4 buffers to render a slice: picture parameter, IQMatrix, slice parameter, slice data
    VABufferID buffer_ids[4];
    VAPictureParameterBufferMPEG4* pic_params = &(pic_data->picture_param);
    vbp_slice_data_mp42* slice_data = &(pic_data->slice_data);
    VASliceParameterBufferMPEG4* slice_params = &(slice_data->slice_param);

    LOG_V( "Begin\n");

    vadisplay = this->va_display;
    vacontext = this->va_context;

    if (!this->end_picture_pending) {
        LOG_E("picture decoder is not started!\n");
        ret = MIX_RESULT_FAIL;
        goto CLEAN_UP;
    }

    // update reference pictures
    frame_type = pic_params->vop_fields.bits.vop_coding_type;

    switch (frame_type) {
    case MP4_VOP_TYPE_I:
        pic_params->forward_reference_picture = VA_INVALID_SURFACE;
        pic_params->backward_reference_picture = VA_INVALID_SURFACE;
        break;

    case MP4_VOP_TYPE_P:
        pic_params-> forward_reference_picture
        = this->reference_frames[0]->frame_id;
        pic_params-> backward_reference_picture = VA_INVALID_SURFACE;
        break;

    case MP4_VOP_TYPE_B:
        pic_params->vop_fields.bits.backward_reference_vop_coding_type
        = this->last_vop_coding_type;
        pic_params->forward_reference_picture
        = this->reference_frames[1]->frame_id;
        pic_params->backward_reference_picture
        = this->reference_frames[0]->frame_id;
        break;

    case MP4_VOP_TYPE_S:
        pic_params-> forward_reference_picture
        = this->reference_frames[0]->frame_id;
        pic_params-> backward_reference_picture = VA_INVALID_SURFACE;
        break;

    default:
        LOG_W("default, Will never reach here\n");
        ret = MIX_RESULT_FAIL;
        goto CLEAN_UP;
        break;
    }

    //Now for slices

    LOG_V( "Creating libva picture parameter buffer\n");

    //First the picture parameter buffer
    vret = vaCreateBuffer(
               vadisplay,
               vacontext,
               VAPictureParameterBufferType,
               sizeof(VAPictureParameterBufferMPEG4),
               1,
               pic_params,
               &buffer_ids[buffer_id_cnt]);

    if (vret != VA_STATUS_SUCCESS) {
        ret = MIX_RESULT_FAIL;
        LOG_E( "Video driver returned error from vaCreateBuffer\n");
        goto CLEAN_UP;
    }

    buffer_id_cnt++;

    if (pic_params->vol_fields.bits.quant_type &&
            this->iq_matrix_buf_sent == FALSE) {
        LOG_V( "Creating libva IQMatrix buffer\n");
        // only send IQ matrix for the first slice in the picture
        vret = vaCreateBuffer(
                   vadisplay,
                   vacontext,
                   VAIQMatrixBufferType,
                   sizeof(VAIQMatrixBufferMPEG4),
                   1,
                   &(data->iq_matrix_buffer),
                   &buffer_ids[buffer_id_cnt]);

        if (vret != VA_STATUS_SUCCESS) {
            ret = MIX_RESULT_FAIL;
            LOG_E( "Video driver returned error from vaCreateBuffer\n");
            goto CLEAN_UP;
        }
        this->iq_matrix_buf_sent = TRUE;
        buffer_id_cnt++;
    }

    vret = vaCreateBuffer(
               vadisplay,
               vacontext,
               VASliceParameterBufferType,
               sizeof(VASliceParameterBufferMPEG4),
               1,
               slice_params,
               &buffer_ids[buffer_id_cnt]);

    if (vret != VA_STATUS_SUCCESS) {
        ret = MIX_RESULT_FAIL;
        LOG_E( "Video driver returned error from vaCreateBuffer\n");
        goto CLEAN_UP;
    }

    buffer_id_cnt++;


    //Do slice data
    //slice data buffer pointer
    //Note that this is the original data buffer ptr;
    // offset to the actual slice data is provided in
    // slice_data_offset in VASliceParameterBufferMP42

    vret = vaCreateBuffer(
               vadisplay,
               vacontext,
               VASliceDataBufferType,
               slice_data->slice_size, //size
               1,        //num_elements
               slice_data->buffer_addr + slice_data->slice_offset,
               &buffer_ids[buffer_id_cnt]);

    buffer_id_cnt++;

    if (vret != VA_STATUS_SUCCESS) {
        ret = MIX_RESULT_FAIL;
        LOG_E( "Video driver returned error from vaCreateBuffer\n");
        goto CLEAN_UP;
    }

    LOG_V( "Calling vaRenderPicture\n");

    //Render the picture
    vret = vaRenderPicture(
               vadisplay,
               vacontext,
               buffer_ids,
               buffer_id_cnt);

    if (vret != VA_STATUS_SUCCESS) {
        ret = MIX_RESULT_FAIL;
        LOG_E( "Video driver returned error from vaRenderPicture\n");
        goto CLEAN_UP;
    }

CLEAN_UP:
    LOG_V( "End\n");
    return ret;
}

MIX_RESULT MixVideoFormat_MP42::_decode_end(bool drop_picture) {
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    VAStatus vret = VA_STATUS_SUCCESS;

    if (!this->end_picture_pending) {
        if (this->video_frame) {
            ret = MIX_RESULT_FAIL;
            LOG_E("Unexpected: video_frame is not unreferenced.\n");
        }
        goto CLEAN_UP;
    }

    if (this->video_frame == NULL) {
        ret = MIX_RESULT_FAIL;
        LOG_E("Unexpected: video_frame has been unreferenced.\n");
        goto CLEAN_UP;
    }

    vret = vaEndPicture(this->va_display, this->va_context);

    if (vret != VA_STATUS_SUCCESS) {
        ret = MIX_RESULT_DROPFRAME;
        LOG_E( "Video driver returned error from vaEndPicture\n");
        goto CLEAN_UP;
    }

#if 0	/* we don't call vaSyncSurface here, the call is moved to mix_video_render() */

    LOG_V( "Calling vaSyncSurface\n");

    //Decode the picture
    vret = vaSyncSurface(vadisplay, surface);

    if (vret != VA_STATUS_SUCCESS)
    {
        ret = MIX_RESULT_FAIL;
        LOG_E( "Video driver returned error from vaSyncSurface\n");
        CLEAN_UP;
    }
#endif

    if (drop_picture) {
        // we are asked to drop this decoded picture
        mix_videoframe_unref(this->video_frame);
        this->video_frame = NULL;
        goto CLEAN_UP;
    }

    //Enqueue the decoded frame using frame manager
    ret = mix_framemanager_enqueue(this->framemgr, this->video_frame);
    if (ret != MIX_RESULT_SUCCESS) {
        LOG_E( "Error enqueuing frame object\n");
        goto CLEAN_UP;
    } else {
        // video frame is passed to frame manager
        this->video_frame = NULL;
    }

CLEAN_UP:
    if (this->video_frame) {
        /* this always indicates an error */
        mix_videoframe_unref(this->video_frame);
        this->video_frame = NULL;
    }
    this->end_picture_pending = FALSE;
    return ret;
}

MIX_RESULT MixVideoFormat_MP42::_decode_continue(vbp_data_mp42 *data) {
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    VAStatus vret = VA_STATUS_SUCCESS;
    uint32 i;
    int frame_type = -1;
    vbp_picture_data_mp42* pic_data = NULL;
    VAPictureParameterBufferMPEG4* pic_params = NULL;
    /*
     Packed Frame Assumption:

      1. In one packed frame, there's only one P or I frame and only one B frame.
     2. In packed frame, there's no skipped frame (vop_coded = 0)
     3. For one packed frame, there will be one N-VOP frame to follow the packed frame (may not immediately).
     4. N-VOP frame is the frame with vop_coded = 0.
     5. The timestamp of  N-VOP frame will be used for P or I frame in the packed frame


     I, P, {P, B}, B, N, P, N, I, ...
     I, P, {P, B}, N, P, N, I, ...

     The first N is placeholder for P frame in the packed frame
     The second N is a skipped frame
     */

    pic_data = data->picture_data;
    for (i = 0; i < data->number_picture_data; i++, pic_data = pic_data->next_picture_data) {
        pic_params = &(pic_data->picture_param);
        frame_type = pic_params->vop_fields.bits.vop_coding_type;
        if (frame_type == MP4_VOP_TYPE_S &&
                pic_params->no_of_sprite_warping_points > 1) {
            // hardware only support up to one warping point (stationary or translation)
            LOG_E("sprite with %d warping points is not supported by HW.\n",
                  pic_params->no_of_sprite_warping_points);
            return MIX_RESULT_DROPFRAME;
        }

        if (pic_data->vop_coded == 0) {
            // this should never happen
            LOG_E("VOP is not coded.\n");
            return MIX_RESULT_DROPFRAME;
        }

        if (pic_data->new_picture_flag == 1 ||
                this->end_picture_pending == FALSE) {
            if (pic_data->new_picture_flag == 0) {
                LOG_W("First slice of picture is lost!\n");
            }

            ulong surface = 0;
            if (this->end_picture_pending)
            {
                // this indicates the start of a new frame in the packed frame
                LOG_V("packed frame is found.\n");

                // Update timestamp for packed frame as timestamp is for the B frame!
                if (this->video_frame && pic_params->vop_time_increment_resolution) {
                    uint64 ts, ts_inc;
                    mix_videoframe_get_timestamp(this->video_frame, &ts);
                    ts_inc= this->last_vop_time_increment - pic_data->vop_time_increment +
                            pic_params->vop_time_increment_resolution;
                    ts_inc = ts_inc % pic_params->vop_time_increment_resolution;
                    LOG_V("timestamp is incremented by %"UINT64_FORMAT" at %d resolution.\n",
                          ts_inc, pic_params->vop_time_increment_resolution);
                    // convert to macrosecond, timestamp takes microsecond as basic unit.
                    ts_inc = ts_inc * 1e6 / pic_params->vop_time_increment_resolution;
                    LOG_V("timestamp of P frame in packed frame is updated from %"UINT64_FORMAT"  to %"UINT64_FORMAT".\n",
                          ts, ts + ts_inc);
                    ts += ts_inc;
                    mix_videoframe_set_timestamp(this->video_frame, ts);
                }

                _decode_end(FALSE);
                this->next_nvop_for_PB_frame = TRUE;
            }

            if (this->next_nvop_for_PB_frame == TRUE &&
                    frame_type != MP4_VOP_TYPE_B) {
                LOG_E("The second frame in the packed frame is not B frame.\n");
                this->next_nvop_for_PB_frame = FALSE;
                return MIX_RESULT_DROPFRAME;
            }

            //Get a frame from the surface pool
            ret = mix_surfacepool_get(this->surfacepool, &(this->video_frame));
            if (ret != MIX_RESULT_SUCCESS) {
                LOG_E( "Error getting frame from surfacepool\n");
                return MIX_RESULT_FAIL;
            }

            /* the following calls will always succeed */

            // set frame type
            if (frame_type == MP4_VOP_TYPE_S) {
                // sprite is treated as P frame in the display order
                mix_videoframe_set_frame_type(this->video_frame, (MixFrameType)MP4_VOP_TYPE_P);
            } else {
                mix_videoframe_set_frame_type(this->video_frame, (MixFrameType)frame_type);
            }

            // set frame structure
            if (pic_data->picture_param.vol_fields.bits.interlaced) {
                // only MPEG-4 studio profile can have field coding. All other profiles
                // use frame coding only, i.e, no field VOP.  (see vop_structure in MP4 spec)
                mix_videoframe_set_frame_structure(
                    this->video_frame,
                    VA_BOTTOM_FIELD | VA_TOP_FIELD);
                LOG_W("Interlaced content, set frame structure to 3 (TOP | BOTTOM field) !\n");
            } else {
                mix_videoframe_set_frame_structure(this->video_frame, VA_FRAME_PICTURE);
            }

            //Set the discontinuity flag
            mix_videoframe_set_discontinuity(
                this->video_frame,
                this->discontinuity_frame_in_progress);

            //Set the timestamp
            mix_videoframe_set_timestamp(this->video_frame, this->current_timestamp);

            //Get our surface ID from the frame object
            ret = mix_videoframe_get_frame_id(this->video_frame, &surface);
            if (ret != MIX_RESULT_SUCCESS) {
                LOG_E( "Error getting surface ID from frame object\n");
                goto CLEAN_UP;
            }

            /* If I or P frame, update the reference array */
            if ((frame_type == MP4_VOP_TYPE_I) || (frame_type == MP4_VOP_TYPE_P)) {
                LOG_V("Updating forward/backward references for libva\n");
                this->last_vop_coding_type = frame_type;
                this->last_vop_time_increment = pic_data->vop_time_increment;
                _handle_ref_frames((_picture_type)frame_type, this->video_frame);
                if (this->last_frame != NULL) {
                    mix_videoframe_unref(this->last_frame);
                }
                this->last_frame = this->video_frame;
                mix_videoframe_ref(this->last_frame);
            }

            //Now we can begin the picture
            vret = vaBeginPicture(this->va_display, this->va_context, surface);
            if (vret != VA_STATUS_SUCCESS) {
                ret = MIX_RESULT_FAIL;
                LOG_E( "Video driver returned error from vaBeginPicture\n");
                goto CLEAN_UP;
            }

            // vaBeginPicture needs a matching vaEndPicture
            this->end_picture_pending = TRUE;
            this->iq_matrix_buf_sent = FALSE;
        }


        ret = _decode_a_slice(data, pic_data);
        if (ret != MIX_RESULT_SUCCESS) {
            LOG_E( "_decode_a_slice failed, error =  %#X.", ret);
            goto CLEAN_UP;
        }
    }

CLEAN_UP:
    return ret;
}



MIX_RESULT MixVideoFormat_MP42::_decode_begin(vbp_data_mp42* data) {
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    int frame_type = -1;
    VAPictureParameterBufferMPEG4* pic_params = NULL;
    vbp_picture_data_mp42 *pic_data = NULL;
    pic_data = data->picture_data;
    pic_params = &(pic_data->picture_param);
    frame_type = pic_params->vop_fields.bits.vop_coding_type;

    if (this->next_nvop_for_PB_frame) {
        // if we are waiting for n-vop for packed frame, and the new frame is coded, the coding type
        // of this frame must be B.
        // for example: {PB} B N P B B P...
        if (pic_data->vop_coded == 1 && frame_type != MP4_VOP_TYPE_B) {
            LOG_E("Invalid coding type while waiting for n-vop for packed frame.\n");
            // timestamp of P frame in the queue is not correct
            mix_framemanager_flush(this->framemgr);
            this->next_nvop_for_PB_frame = FALSE;
        }
    }

    if (pic_data->vop_coded == 0) {
        if (this->last_frame == NULL) {
            LOG_E("The forward reference frame is NULL, couldn't reconstruct skipped frame.\n");
            mix_framemanager_flush(this->framemgr);
            this->next_nvop_for_PB_frame = FALSE;
            return MIX_RESULT_DROPFRAME;
        }

        if (this->next_nvop_for_PB_frame) {
            // P frame is already in queue, just need to update time stamp.
            mix_videoframe_set_timestamp(this->last_frame, this->current_timestamp);
            this->next_nvop_for_PB_frame = FALSE;
        } else {
            // handle skipped frame
            MixVideoFrame *skip_frame = NULL;
            ulong frame_id = VA_INVALID_SURFACE;

            skip_frame = mix_videoframe_new();
            ret = mix_videoframe_set_is_skipped(skip_frame, TRUE);
            ret = mix_videoframe_get_frame_id(this->last_frame, &frame_id);
            ret = mix_videoframe_set_frame_id(skip_frame, frame_id);
            ret = mix_videoframe_set_frame_type(skip_frame, (MixFrameType)MP4_VOP_TYPE_P);
            ret = mix_videoframe_set_real_frame(skip_frame, this->last_frame);
            // add a reference as skip_frame holds the last_frame.
            mix_videoframe_ref(this->last_frame);
            ret = mix_videoframe_set_timestamp(skip_frame, this->current_timestamp);
            ret = mix_videoframe_set_discontinuity(skip_frame, FALSE);

            LOG_V("Processing skipped frame %x, frame_id set to %d, ts %"UINT64_FORMAT"\n",
                  (uint)skip_frame, (uint)frame_id, this->current_timestamp);

            /* Enqueue the skipped frame using frame manager */
            ret = mix_framemanager_enqueue(this->framemgr, skip_frame);
        }

        if (data->number_picture_data > 1) {
            LOG_E("Unexpected to have more picture data following a not-coded VOP.\n");
            //picture data is thrown away. No issue if picture data is for N-VOP. if picture data is for
            // coded picture, a frame is lost.
        }
        return MIX_RESULT_SUCCESS;
    } else {
        /*
        * Check for B frames after a seek
        * We need to have both reference frames in hand before we can decode a B frame
        * If we don't have both reference frames, we must return MIX_RESULT_DROPFRAME
        */
        if (frame_type == MP4_VOP_TYPE_B) {
            if (this->reference_frames[0] == NULL ||
                    this->reference_frames[1] == NULL) {
                LOG_W("Insufficient reference frames for B frame\n");
                return MIX_RESULT_DROPFRAME;
            }
        } else if (frame_type == MP4_VOP_TYPE_P || frame_type == MP4_VOP_TYPE_S) {
#if 0
        /*
        * For special clips using P frame (special P frame with all MB intra coded) as key frame
        * Need to skip the reference check to enable the seek
        */
            if (this->reference_frames[0] == NULL) {
                LOG_W("Reference frames for P/S frame is missing\n");
                return MIX_RESULT_DROPFRAME;
            }
#endif
        }
        // all sanity check passes, continue decoding through mix_videofmt_mp42_decode_continue
        ret = _decode_continue(data);
    }
    return ret;
}

MIX_RESULT MixVideoFormat_MP42::_decode_a_buffer(
    MixBuffer * bufin, uint64 ts, bool discontinuity,bool complete_frame) {
    uint32 pret = 0;
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    vbp_data_mp42 *data = NULL;
    uint64 last_ts = 0;

    LOG_V( "Begin\n");
    pret = vbp_parse(this->parser_handle,
                     bufin->data,
                     bufin->size,
                     FALSE);

    if (pret != VBP_OK) {
        ret = MIX_RESULT_DROPFRAME;
        LOG_E( "vbp_parse failed.\n");
        goto CLEAN_UP;
    }
    else {
        LOG_V("vbp_parse succeeded.\n");
    }
    //query for data
    pret = vbp_query(this->parser_handle, (void **) &data);

    if ((pret != VBP_OK) || (data == NULL)) {
        // never happen!
        ret = MIX_RESULT_FAIL;
        LOG_E( "vbp_query failed.\n");
        goto CLEAN_UP;
    } else {
        LOG_V("vbp_query succeeded.\n");
    }

    if (this->va_initialized == FALSE) {
        _update_config_params(data);
        LOG_V("try initializing VA...\n");
        ret = _initialize_va(data);
        if (ret != MIX_RESULT_SUCCESS) {
            LOG_V("mix_videofmt_mp42_initialize_va failed.\n");
            goto CLEAN_UP;
        }
    }

    // check if any slice is parsed, we may just receive configuration data
    if (data->number_picture_data == 0) {
        ret = MIX_RESULT_SUCCESS;
        LOG_V("slice is not available.\n");
        goto CLEAN_UP;
    }

    last_ts = this->current_timestamp;
    this->current_timestamp = ts;
    this->discontinuity_frame_in_progress = discontinuity;

    if (last_ts != ts) {
        // finish decoding the last frame
        ret = _decode_end(FALSE);
        if (ret != MIX_RESULT_SUCCESS) {
            LOG_V("mix_videofmt_mp42_decode_end failed.\n");
            goto CLEAN_UP;
        }

        // start decoding a new frame
        ret = _decode_begin(data);
        if (ret != MIX_RESULT_SUCCESS) {
            LOG_V("mix_videofmt_mp42_decode_begin failed.\n");
            goto CLEAN_UP;
        }
    } else {
        ret = _decode_continue(data);
        if (ret != MIX_RESULT_SUCCESS) {
            LOG_V("mix_videofmt_mp42_decode_continue failed.\n");
            goto CLEAN_UP;
        }
    }
    if (complete_frame)
    {
        // finish decoding current frame
        ret = _decode_end(FALSE);
        if (ret != MIX_RESULT_SUCCESS)
        {
            LOG_V("mix_videofmt_mp42_decode_end failed.\n");
            goto CLEAN_UP;
        }
    }

CLEAN_UP:
    LOG_V( "End\n");
    return ret;
}


MIX_RESULT MixVideoFormat_MP42::Initialize(
    MixVideoConfigParamsDec * config_params,
    MixFrameManager * frame_mgr,
    MixBufferPool * input_buf_pool,
    MixSurfacePool ** surface_pool,
    VADisplay va_display ) {

    uint32 pret = 0;
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    enum _vbp_parser_type ptype = VBP_MPEG4;
    vbp_data_mp42 *data = NULL;
    MixIOVec *header = NULL;

    if (config_params == NULL || frame_mgr == NULL || input_buf_pool == NULL || va_display == NULL) {
        LOG_E( "NUll pointer passed in\n");
        return MIX_RESULT_NULL_PTR;
    }

    LOG_V( "Begin\n");

    // chain up parent method
    MixVideoFormat::Initialize(config_params, frame_mgr, input_buf_pool,
                               surface_pool, va_display);

    if (ret != MIX_RESULT_SUCCESS) {
        LOG_E( "Error initializing\n");
        return ret;
    }

    LOG_V( "Locking\n");
    //From now on, we exit this function through cleanup:
    Lock();

    this->surfacepool = mix_surfacepool_new();
    *surface_pool = this->surfacepool;

    if (this->surfacepool == NULL) {
        ret = MIX_RESULT_NO_MEMORY;
        LOG_E( "parent->surfacepool == NULL.\n");
        goto CLEAN_UP;
    }

    ret = mix_videoconfigparamsdec_get_extra_surface_allocation(config_params,
            &this->extra_surfaces);

    if (ret != MIX_RESULT_SUCCESS) {
        ret = MIX_RESULT_FAIL;
        LOG_E( "Cannot get extra surface allocation setting\n");
        goto CLEAN_UP;
    }

    //Load the bitstream parser
    pret = vbp_open(ptype, &(this->parser_handle));

    if (!(pret == VBP_OK)) {
        ret = MIX_RESULT_FAIL;
        LOG_E( "Error opening parser\n");
        goto CLEAN_UP;
    }
    LOG_V( "Opened parser\n");


    ret = mix_videoconfigparamsdec_get_header(config_params, &header);
    if ((ret != MIX_RESULT_SUCCESS) || (header == NULL)) {
        // Delay initializing VA if codec configuration data is not ready, but don't return an error.
        ret = MIX_RESULT_SUCCESS;
        LOG_W( "Codec data is not available in the configuration parameter.\n");
        goto CLEAN_UP;
    }

    LOG_V( "Calling parse on header data, handle %d\n", (int)this->parser_handle);

    pret = vbp_parse(this->parser_handle, header->data,
                     header->data_size, TRUE);

    if (pret != VBP_OK) {
        ret = MIX_RESULT_FAIL;
        LOG_E( "Error parsing header data\n");
        goto CLEAN_UP;
    }

    LOG_V( "Parsed header\n");

    //Get the header data and save
    pret = vbp_query(this->parser_handle, (void **)&data);

    if ((pret != VBP_OK) || (data == NULL)) {
        ret = MIX_RESULT_FAIL;
        LOG_E( "Error reading parsed header data\n");
        goto CLEAN_UP;
    }

    LOG_V( "Queried parser for header data\n");

    _update_config_params(data);

    ret = _initialize_va(data);
    if (ret != MIX_RESULT_SUCCESS) {
        LOG_E( "Error initializing va. \n");
        goto CLEAN_UP;
    }

CLEAN_UP:
    if (ret != MIX_RESULT_SUCCESS) {
        if (this->parser_handle) {
            pret = vbp_close(this->parser_handle);
            this->parser_handle = NULL;
        }
        this->initialized = FALSE;
    } else {
        this->initialized = TRUE;
    }
    if (header != NULL) {
        if (header->data != NULL)
            free(header->data);
        free(header);
        header = NULL;
    }
    LOG_V( "Unlocking\n");
    Unlock();
    return ret;
}

MIX_RESULT MixVideoFormat_MP42::Decode(
    MixBuffer * bufin[], int bufincnt, MixVideoDecodeParams * decode_params) {

    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    uint64 ts = 0;
    bool discontinuity = FALSE;

    LOG_V( "Begin\n");

    if (bufin == NULL || decode_params == NULL ) {
        LOG_E( "NUll pointer passed in\n");
        return MIX_RESULT_NULL_PTR;
    }

    /* Chainup parent method.
    	We are not chaining up to parent method for now.
     */
#if 0
    if (parent_class->decode) {
        return parent_class->decode(mix, bufin, bufincnt, decode_params);
    }
#endif
    ret = mix_videodecodeparams_get_timestamp(decode_params, &ts);
    if (ret != MIX_RESULT_SUCCESS) {
        // never happen
        return MIX_RESULT_FAIL;
    }

    ret = mix_videodecodeparams_get_discontinuity(decode_params, &discontinuity);
    if (ret != MIX_RESULT_SUCCESS) {
        // never happen
        return MIX_RESULT_FAIL;
    }

    //From now on, we exit this function through cleanup:
    LOG_V( "Locking\n");
    Lock();

    LOG_I("ts after mix_videodecodeparams_get_timestamp() = %"UINT64_FORMAT"\n", ts);

    for (int i = 0; i < bufincnt; i++) {
        LOG_V("decode buffer %d in total %d \n", i, bufincnt);
        // decode a buffer at a time
        ret = _decode_a_buffer(
                  bufin[i],
                  ts,
                  discontinuity,
                  ((i == bufincnt - 1) ? decode_params->complete_frame : 0));
        if (ret != MIX_RESULT_SUCCESS) {
            LOG_E("mix_videofmt_mp42_decode_a_buffer failed.\n");
            break;
        }
    }

    LOG_V( "Unlocking\n");
    Unlock();
    LOG_V( "End\n");
    return ret;
}


MIX_RESULT MixVideoFormat_MP42::Flush() {
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    LOG_V("Begin\n");

    Lock();
    // drop any decode-pending picture, and ignore return value
    _decode_end(TRUE);

    /*
     * Clear parse_in_progress flag and current timestamp
     */
    this->parse_in_progress = FALSE;
    this->discontinuity_frame_in_progress = FALSE;
    this->current_timestamp = (uint64)-1;
    this->next_nvop_for_PB_frame = FALSE;

    for (int idx = 0; idx < 2; idx++) {
        if (this->reference_frames[idx] != NULL) {
            mix_videoframe_unref(this->reference_frames[idx]);
            this->reference_frames[idx] = NULL;
        }
    }
    if (this->last_frame) {
        mix_videoframe_unref(this->last_frame);
        this->last_frame = NULL;
    }

    /* Call parser flush */
    vbp_flush(this->parser_handle);
    Unlock();
    LOG_V("End\n");
    return ret;
}

MIX_RESULT MixVideoFormat_MP42::EndOfStream() {
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    LOG_V("Begin\n");
    Lock();
    _decode_end(FALSE);
    ret = mix_framemanager_eos(this->framemgr);
    Unlock();
    LOG_V("End\n");
    return ret;
}


MIX_RESULT MixVideoFormat_MP42::_handle_ref_frames(
    enum _picture_type frame_type, MixVideoFrame * current_frame) {
    LOG_V("Begin\n");
    if (current_frame == NULL) {
        return MIX_RESULT_NULL_PTR;
    }
    switch (frame_type) {
    case MP4_VOP_TYPE_I:
    case MP4_VOP_TYPE_P:
        LOG_V("Refing reference frame %x\n", (uint) current_frame);
        mix_videoframe_ref(current_frame);

        /* should only happen on first frame */
        if (this->reference_frames[0] == NULL) {
            this->reference_frames[0] = current_frame;
            /* should only happen on second frame */
        } else if (this->reference_frames[1] == NULL) {
            this->reference_frames[1] = current_frame;
        } else {
            LOG_V("Releasing reference frame %x\n",
                  (uint) this->reference_frames[0]);
            mix_videoframe_unref(this->reference_frames[0]);
            this->reference_frames[0] = this->reference_frames[1];
            this->reference_frames[1] = current_frame;
        }
        break;
    case MP4_VOP_TYPE_B:
    case MP4_VOP_TYPE_S:
    default:
        break;

    }

    LOG_V("End\n");

    return MIX_RESULT_SUCCESS;
}

