/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

#include "mixvideolog.h"

#include "mixvideoformat_vc1.h"
#ifndef ANDROID
#include <va/va_x11.h>
#endif


#include <string.h>


#ifdef MIX_LOG_ENABLE
static int mix_video_vc1_counter = 0;
#endif


MixVideoFormat_VC1::MixVideoFormat_VC1() {
    this->reference_frames[0] = NULL;
    this->reference_frames[1] = NULL;
}

MixVideoFormat_VC1::~MixVideoFormat_VC1() {
    /* clean up here. */
    Lock();
    //surfacepool is deallocated by parent
    //inputbufqueue is deallocated by parent
    //parent calls vaDestroyConfig, vaDestroyContext and vaDestroySurfaces
    //Unref our reference frames;
    for (int i = 0; i < 2; i++) 	{
        if (this->reference_frames[i] != NULL)
        {
            mix_videoframe_unref(this->reference_frames[i]);
            this->reference_frames[i] = NULL;
        }
    }

    //Reset state
    this->initialized = TRUE;
    this->parse_in_progress = FALSE;
    this->discontinuity_frame_in_progress = FALSE;
    this->current_timestamp = (uint64)-1;

    //Close the parser
    if (this->parser_handle)
    {
        vbp_close(this->parser_handle);
        this->parser_handle = NULL;
    }

    Unlock();
}


MixVideoFormat_VC1 * mix_videoformat_vc1_new(void) {
    return new MixVideoFormat_VC1();
}

MixVideoFormat_VC1 * mix_videoformat_vc1_ref(MixVideoFormat_VC1 * mix) {
    if (NULL != mix)
        mix->Ref();
    return mix;
}
MixVideoFormat_VC1 *mix_videoformat_vc1_unref(MixVideoFormat_VC1 * mix) {
    if (NULL != mix)
        return MIX_VIDEOFORMAT_VC1(mix->Unref());
    else
        return mix;
}

MIX_RESULT MixVideoFormat_VC1::_update_seq_header(
    MixVideoConfigParamsDec* config_params,
    MixIOVec *header) {
    uint width = 0;
    uint height = 0;

    int i = 0;
    uchar* p = NULL;
    MIX_RESULT res = MIX_RESULT_SUCCESS;

    if (!config_params || !header) {
        LOG_E( "NUll pointer passed in\n");
        return (MIX_RESULT_NULL_PTR);
    }

    p = header->data;

    res = mix_videoconfigparamsdec_get_picture_res(
              config_params, &width, &height);

    if (MIX_RESULT_SUCCESS != res) {
        return res;
    }

    /* Check for start codes.  If one exist, then this is VC-1 and not WMV. */
    while (i < header->data_size - 2) {
        if ((p[i] == 0) && (p[i + 1] == 0) && (p[i + 2] == 1)) {
            return MIX_RESULT_SUCCESS;
        }
        i++;
    }

//	p = reinterpret_cast<uchar*>(g_malloc0(header->data_size + 9));
    p = reinterpret_cast<uchar*>(malloc(header->data_size + 9));

    if (!p) {
        LOG_E( "Cannot allocate memory\n");
        return MIX_RESULT_NO_MEMORY;
    }
    memset(p, 0, header->data_size + 9);

    /* If we get here we have 4+ bytes of codec data that must be formatted */
    /* to pass through as an RCV sequence header. */
    p[0] = 0;
    p[1] = 0;
    p[2] = 1;
    p[3] = 0x0f;  /* Start code. */

    p[4] = (width >> 8) & 0x0ff;
    p[5] = width & 0x0ff;
    p[6] = (height >> 8) & 0x0ff;
    p[7] = height & 0x0ff;

    memcpy(p + 8, header->data, header->data_size);
    *(p + header->data_size + 8) = 0x80;

    free(header->data);
    header->data = p;
    header->data_size = header->data_size + 9;

    return MIX_RESULT_SUCCESS;
}


MIX_RESULT MixVideoFormat_VC1::_update_config_params(vbp_data_vc1 *data) {
    if (this->picture_width == 0 ||
            this->picture_height == 0) {
        this->picture_width = data->se_data->CODED_WIDTH;
        this->picture_height = data->se_data->CODED_HEIGHT;
        mix_videoconfigparamsdec_set_picture_res(
            this->config_params,
            this->picture_width,
            this->picture_height);
    }

    // scaling has been performed on the decoded image.
    mix_videoconfigparamsdec_set_video_range(this->config_params, 1);
    uint8 color_matrix;
    switch (data->se_data->MATRIX_COEF) {
    case 1:
        color_matrix = VA_SRC_BT709;
        break;
        // ITU-R BT.1700, ITU-R BT.601-5, and SMPTE 293M-1996.
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
        this->config_params,
        data->se_data->ASPECT_HORIZ_SIZE,
        data->se_data->ASPECT_VERT_SIZE);

    mix_videoconfigparamsdec_set_bit_rate(
        this->config_params,
        data->se_data->bit_rate);
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT MixVideoFormat_VC1::_initialize_va(vbp_data_vc1 *data) {
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
    switch (data->se_data->PROFILE) {
    case 0:
        va_profile = VAProfileVC1Simple;
        break;
    case 1:
        va_profile = VAProfileVC1Main;
        break;

    default:
        va_profile = VAProfileVC1Advanced;
        break;
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
        goto cleanup;
    }


    //Check for loop filtering
    if (data->se_data->LOOPFILTER == 1)
        this->loopFilter = TRUE;
    else
        this->loopFilter = FALSE;

    LOG_V( "loop filter is %d, TFCNTRFLAG is %d\n", data->se_data->LOOPFILTER, data->se_data->TFCNTRFLAG);

    if ((data->se_data->MAXBFRAMES > 0) || (data->se_data->PROFILE == 3) || (data->se_data->PROFILE == 1)) {
        //If Advanced profile, have to assume B frames may be present, since MAXBFRAMES is not valid for this prof
        this->haveBframes = TRUE;
    }
    else {
        this->haveBframes = FALSE;
    }

    //Calculate VC1 numSurfaces based on max number of B frames or
    // MIX_VIDEO_VC1_SURFACE_NUM, whichever is less

    //Adding 1 to work around VBLANK issue
    this->va_num_surfaces = 1 + this->extra_surfaces + ((3 + (this->haveBframes ? 1 : 0) <
                            MIX_VIDEO_VC1_SURFACE_NUM) ?
                            (3 + (this->haveBframes ? 1 : 0))
                                : MIX_VIDEO_VC1_SURFACE_NUM);

    this->va_surfaces = new VASurfaceID[this->va_num_surfaces];
    if (this->va_surfaces == NULL) {
        ret = MIX_RESULT_FAIL;
        LOG_E( "parent->va_surfaces == NULL. \n");
        goto cleanup;
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
        goto cleanup;
    }

    LOG_V( "Created %d libva surfaces\n", this->va_num_surfaces);

    //Initialize the surface pool
    ret = mix_surfacepool_initialize(
              this->surfacepool,
              this->va_surfaces,
              this->va_num_surfaces,
              this->va_display);

    switch (ret) {
    case MIX_RESULT_SUCCESS:
        break;
    case MIX_RESULT_ALREADY_INIT:  //This case is for future use when we can be  initialized multiple times.  It is to detect when we have not been reset before re-initializing.
    default:
        ret = MIX_RESULT_ALREADY_INIT;
        LOG_E( "Error init surface pool\n");
        goto cleanup;
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
        goto cleanup;
    }

    LOG_V( "mix_video vinfo:  Content type	%s\n", (data->se_data->INTERLACE) ? "interlaced" : "progressive");
    LOG_V( "mix_video vinfo:  Content width %d, height %d\n", this->picture_width, this->picture_height);
    LOG_V( "mix_video vinfo:  MAXBFRAMES %d (note that for Advanced profile, MAXBFRAMES can be zero and there still can be B frames in the content)\n", data->se_data->MAXBFRAMES);
    LOG_V( "mix_video vinfo:  PROFILE %d, LEVEL %d\n", data->se_data->PROFILE, data->se_data->LEVEL);

    this->va_initialized = TRUE;
cleanup:
    /* nothing to clean up */

    return ret;

}

MIX_RESULT MixVideoFormat_VC1::Initialize(
    MixVideoConfigParamsDec * config_params,
    MixFrameManager * frame_mgr,
    MixBufferPool * input_buf_pool,
    MixSurfacePool ** surface_pool,
    VADisplay va_display) {

    uint32 pret = 0;
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    enum _vbp_parser_type ptype = VBP_VC1;
    vbp_data_vc1 *data = NULL;
    MixIOVec *header = NULL;

    //TODO Partition this method into smaller methods
    if (config_params == NULL || frame_mgr == NULL ||
            !input_buf_pool || !surface_pool || !va_display) {
        LOG_E( "NUll pointer passed in\n");
        return MIX_RESULT_NULL_PTR;
    }
    LOG_V( "Begin\n");

    // chain up parent method
    MixVideoFormat::Initialize(config_params, frame_mgr, input_buf_pool,
                               surface_pool, va_display);

    if (ret != MIX_RESULT_SUCCESS) {
        return ret;
    }
    LOG_V( "Locking\n");
    //From now on, we exit this function through cleanup:
    Lock();

    this->surfacepool = mix_surfacepool_new();
    *surface_pool = this->surfacepool;

    if (this->surfacepool == NULL)
    {
        ret = MIX_RESULT_NO_MEMORY;
        LOG_E( "parent->surfacepool == NULL.\n");
        goto cleanup;
    }

    ret = mix_videoconfigparamsdec_get_extra_surface_allocation(config_params,
            &this->extra_surfaces);

    if (ret != MIX_RESULT_SUCCESS)
    {
        ret = MIX_RESULT_FAIL;
        LOG_E( "Cannot get extra surface allocation setting\n");
        goto cleanup;
    }


    //Load the bitstream parser
    pret = vbp_open(ptype, &(this->parser_handle));

    if (!(pret == VBP_OK)) {
        ret = MIX_RESULT_FAIL;
        LOG_E( "Error opening parser\n");
        goto cleanup;
    }

    LOG_V( "Opened parser\n");

    ret = mix_videoconfigparamsdec_get_header(config_params,
            &header);

    if ((ret != MIX_RESULT_SUCCESS) || (header == NULL)) {
        ret = MIX_RESULT_SUCCESS;
        LOG_W( "Codec data is not available in the configuration parameter.\n");

        goto cleanup;
    }

    LOG_V( "Calling parse on header data, handle %d\n", (int)this->parser_handle);
    LOG_V( "mix_video vinfo:  Content type %s, %s\n", (header->data_size > 8) ? "VC-1" : "WMV", (data->se_data->INTERLACE) ? "interlaced" : "progressive");

    ret = _update_seq_header(config_params, header);
    if (ret != MIX_RESULT_SUCCESS) {
        ret = MIX_RESULT_FAIL;
        LOG_E( "Error updating sequence header\n");
        goto cleanup;
    }

    pret = vbp_parse(this->parser_handle, header->data,
                     header->data_size, TRUE);

    if ((pret != VBP_OK)) {
        ret = MIX_RESULT_FAIL;
        LOG_E( "Error parsing header data, size %d\n", header->data_size);
        goto cleanup;
    }


    LOG_V( "Parsed header\n");
    //Get the header data and save
    pret = vbp_query(this->parser_handle, (void **)&data);

    if ((pret != VBP_OK) || (data == NULL)) {
        ret = MIX_RESULT_FAIL;
        LOG_E( "Error reading parsed header data\n");
        goto cleanup;
    }
    LOG_V( "Queried parser for header data\n");

    _update_config_params(data);

    ret = _initialize_va(data);
    if (ret != MIX_RESULT_SUCCESS) {
        LOG_E( "Error initializing va. \n");
        goto cleanup;
    }

cleanup:
    if (ret != MIX_RESULT_SUCCESS) {
        pret = vbp_close(this->parser_handle);
        this->parser_handle = NULL;
        this->initialized = FALSE;
    } else {
        this->initialized = TRUE;
    }

    if (header != NULL) {
        if (header->data != NULL)
            delete[](header->data);
        delete(header);
        header = NULL;
    }

    this->lastFrame = NULL;
    LOG_V( "Unlocking\n");
    Unlock();
    LOG_V( "End\n");
    return ret;
}

MIX_RESULT MixVideoFormat_VC1::Decode(
    MixBuffer * bufin[], int bufincnt,
    MixVideoDecodeParams * decode_params) {

    uint32 pret = 0;
    int i = 0;
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    uint64 ts = 0;
    vbp_data_vc1 *data = NULL;
    bool discontinuity = FALSE;
    if (bufin == NULL || decode_params == NULL) {
        LOG_E( "NUll pointer passed in\n");
        return MIX_RESULT_NULL_PTR;
    }
    //TODO remove iovout and iovoutcnt; they are not used (need to remove from MixVideo/MI-X API too)
    LOG_V( "Begin\n");
    /* Chainup parent method.
    	We are not chaining up to parent method for now.
     */
#if 0
    if (parent_class->decode) {
        return parent_class->decode(mix, bufin, bufincnt,
                                    decode_params);
    }
#endif

    ret = mix_videodecodeparams_get_timestamp(decode_params, &ts);
    if (ret != MIX_RESULT_SUCCESS) {
        return MIX_RESULT_FAIL;
    }

    ret = mix_videodecodeparams_get_discontinuity(decode_params, &discontinuity);
    if (ret != MIX_RESULT_SUCCESS) {
        return MIX_RESULT_FAIL;
    }

    //From now on, we exit this function through cleanup:
    LOG_V( "Locking\n");
    Lock();

    this->current_timestamp = ts;
    this->discontinuity_frame_in_progress = discontinuity;
    LOG_V( "Starting current frame %d, timestamp %"UINT64_FORMAT"\n", mix_video_vc1_counter++, ts);

    for (i = 0; i < bufincnt; i++) {
        LOG_V( "Calling parse for current frame, parse handle %d, buf %x, size %d\n",
               (int)this->parser_handle, (uint)bufin[i]->data, bufin[i]->size);
        pret = vbp_parse(this->parser_handle, bufin[i]->data, bufin[i]->size, FALSE);
        LOG_V( "Called parse for current frame\n");
        if (pret != VBP_OK) {
            ret = MIX_RESULT_FAIL;
            LOG_E( "Error parsing data\n");
            goto CLEAN_UP;
        }
        //query for data
        pret = vbp_query(this->parser_handle, (void **) &data);
        if ((pret != VBP_OK) || (data == NULL)) {
            ret = MIX_RESULT_FAIL;
            LOG_E( "Error getting parser data\n");
            goto CLEAN_UP;
        }
        if (this->va_initialized == FALSE) {
            _update_config_params(data);

            LOG_V("try initializing VA...\n");
            ret = _initialize_va(data);
            if (ret != MIX_RESULT_SUCCESS) {
                LOG_V("mix_videofmt_vc1_initialize_va failed.\n");
                goto CLEAN_UP;
            }
        }

        LOG_V( "Called query for current frame\n");

        //process and decode data
        ret = _process_decode(data, ts, discontinuity);
        if (ret != MIX_RESULT_SUCCESS)
        {
            //We log this but continue since we need to complete our processing of input buffers
            LOG_E( "Process_decode failed.\n");
            goto CLEAN_UP;
        }

    }

CLEAN_UP:
    LOG_V( "Unlocking\n");
    Unlock();
    LOG_V( "End\n");
    return ret;
}


MIX_RESULT MixVideoFormat_VC1::_decode_a_picture(
    vbp_data_vc1 *data, int pic_index, MixVideoFrame *frame) {
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    VAStatus vret = VA_STATUS_SUCCESS;
    VADisplay vadisplay = NULL;
    VAContextID vacontext;
    uint buffer_id_cnt = 0;
    VABufferID *buffer_ids = NULL;
    vbp_picture_data_vc1* pic_data = &(data->pic_data[pic_index]);
    VAPictureParameterBufferVC1 *pic_params = pic_data->pic_parms;
    enum _picture_type frame_type = VC1_PTYPE_I;
    ulong surface = 0;

    if (pic_params == NULL) {
        ret = MIX_RESULT_NULL_PTR;
        LOG_E( "Error reading parser data\n");
        goto CLEAN_UP;
    }

    LOG_V( "num_slices is %d, allocating %d buffer_ids\n", pic_data->num_slices, (pic_data->num_slices * 2) + 2);

    //Set up reference frames for the picture parameter buffer
    //Set the picture type (I, B or P frame)
    frame_type = (_picture_type)pic_params->picture_fields.bits.picture_type;

    //Check for B frames after a seek
    //We need to have both reference frames in hand before we can decode a B frame
    //If we don't have both reference frames, we must return MIX_RESULT_DROPFRAME
    //Note:  demuxer should do the right thing and only seek to I frame, so we should
    //  not get P frame first, but may get B frames after the first I frame
    if (frame_type == VC1_PTYPE_B) {
        if (this->reference_frames[1] == NULL) {
            LOG_E( "Insufficient reference frames for B frame\n");
            ret = MIX_RESULT_DROPFRAME;
            goto CLEAN_UP;
        }
    }

    buffer_ids = reinterpret_cast<VABufferID*>(malloc(sizeof(VABufferID) * ((pic_data->num_slices * 2) + 2)));
    if (buffer_ids == NULL) {
        LOG_E( "Cannot allocate buffer IDs\n");
        ret = MIX_RESULT_NO_MEMORY;
        goto CLEAN_UP;
    }

    LOG_V( "Getting a new surface\n");
    LOG_V( "frame type is %d\n", frame_type);

    //Get our surface ID from the frame object
    ret = mix_videoframe_get_frame_id(frame, &surface);
    if (ret != MIX_RESULT_SUCCESS) {
        LOG_E( "Error getting surface ID from frame object\n");
        goto CLEAN_UP;
    }

    //Get a frame from the surface pool
    if (0 == pic_index) {
        //Set the frame type for the frame object (used in reordering by frame manager)
        switch (frame_type) {
        case VC1_PTYPE_I:  // I frame type
        case VC1_PTYPE_P:  // P frame type
        case VC1_PTYPE_B:  // B frame type
            ret = mix_videoframe_set_frame_type(frame, (MixFrameType)frame_type);
            break;
        case VC1_PTYPE_BI: // BI frame type
            ret = mix_videoframe_set_frame_type(frame, TYPE_B);
            break;
            //Not indicated here
        case VC1_PTYPE_SKIPPED:
        default:
            break;
        }
    }

    if (ret != MIX_RESULT_SUCCESS) {
        LOG_E( "Error setting frame type on frame\n");
        goto CLEAN_UP;
    }

    LOG_V( "Setting reference frames in picparams, frame_type = %d\n", frame_type);
    //TODO Check if we need to add more handling of B or P frames when reference frames are not set up (such as after flush/seek)

    switch (frame_type) {
    case VC1_PTYPE_I:  // I frame type
        /* forward and backward reference pictures are not used but just set to current
        surface to be in consistence with test suite
        */
        pic_params->forward_reference_picture = surface;
        pic_params->backward_reference_picture = surface;
        LOG_V( "I frame, surface ID %u\n", (uint)frame->frame_id);
        LOG_V( "mix_video vinfo:  Frame type is I\n");
        break;
    case VC1_PTYPE_P:  // P frame type
        // check REFDIST in the picture parameter buffer
        if (0 != pic_params->reference_fields.bits.reference_distance_flag &&
                0 != pic_params->reference_fields.bits.reference_distance) {
            /* The previous decoded frame (distance is up to 16 but not 0) is used
            for reference, as we don't allocate that many surfaces so the reference picture
            could have been overwritten and hence not avaiable for reference.
            */
            LOG_E( "reference distance is not 0!");
            ret = MIX_RESULT_DROPFRAME;
            goto CLEAN_UP;
        }
        if (1 == pic_index) {
            // handle interlace field coding case
            if (1 == pic_params->reference_fields.bits.num_reference_pictures ||
                    1 == pic_params->reference_fields.bits.reference_field_pic_indicator) {
                /* two reference fields or the second closest I/P field is used for
                 prediction. Set forward reference picture to INVALID so it will be
                updated to a valid previous reconstructed reference frame later.
                */
                pic_params->forward_reference_picture  = VA_INVALID_SURFACE;
            } else {
                /* the closest I/P is used for reference so it must be the
                 complementary field in the same surface.
                */
                pic_params->forward_reference_picture  = surface;
            }
        }
        if (VA_INVALID_SURFACE == pic_params->forward_reference_picture) {
            if (this->reference_frames[1]) {
                pic_params->forward_reference_picture = this->reference_frames[1]->frame_id;
            } else if (this->reference_frames[0]) {
                pic_params->forward_reference_picture = this->reference_frames[0]->frame_id;
            } else {
                ret = MIX_RESULT_DROPFRAME;
                LOG_E( "Error could not find reference frames for P frame\n");
                goto CLEAN_UP;
            }
        }
        pic_params->backward_reference_picture = VA_INVALID_SURFACE;

#ifdef MIX_LOG_ENABLE	/* this is to fix a crash when MIX_LOG_ENABLE is set */
        if (this->reference_frames[0] && frame) {
            LOG_V( "P frame, surface ID %u, forw ref frame is %u\n",
                   (uint)frame->frame_id, (uint)this->reference_frames[0]->frame_id);
        }
#endif
        LOG_V( "mix_video vinfo:  Frame type is P\n");
        break;

    case VC1_PTYPE_B:  // B frame type
        LOG_V( "B frame, forw ref %d, back ref %d\n",
               (uint)this->reference_frames[0]->frame_id,
               (uint)this->reference_frames[1]->frame_id);

        if (!this->haveBframes) {//We don't expect B frames and have not allocated a surface
            // for the extra ref frame so this is an error
            ret = MIX_RESULT_DROPFRAME;
            LOG_E( "Unexpected B frame, cannot process\n");
            goto CLEAN_UP;
        }

        pic_params->forward_reference_picture = this->reference_frames[0]->frame_id;
        pic_params->backward_reference_picture = this->reference_frames[1]->frame_id;

        LOG_V( "B frame, surface ID %u, forw ref %d, back ref %d\n",
               (uint)frame->frame_id, (uint)this->reference_frames[0]->frame_id,
               (uint)this->reference_frames[1]->frame_id);
        LOG_V( "mix_video vinfo:  Frame type is B\n");
        break;
    case VC1_PTYPE_BI:
        pic_params->forward_reference_picture = VA_INVALID_SURFACE;
        pic_params->backward_reference_picture = VA_INVALID_SURFACE;
        LOG_V( "BI frame\n");
        LOG_V( "mix_video vinfo:  Frame type is BI\n");
        break;
    case VC1_PTYPE_SKIPPED:
        //Will never happen here
        break;
    default:
        LOG_V( "Hit default\n");
        break;
    }

    //Loop filter handling
    if (this->loopFilter) {
        LOG_V( "Setting in loop decoded picture to current frame\n");
        LOG_V( "Double checking picparams inloop filter is %d\n",
               pic_params->entrypoint_fields.bits.loopfilter);
        pic_params->inloop_decoded_picture = frame->frame_id;
    } else {
        LOG_V( "Setting in loop decoded picture to invalid\n");
        pic_params->inloop_decoded_picture = VA_INVALID_SURFACE;
    }
    //Libva buffer set up
    vadisplay = this->va_display;
    vacontext = this->va_context;
    LOG_V( "Calling vaBeginPicture\n");

    //Now we can begin the picture
    vret = vaBeginPicture(vadisplay, vacontext, surface);

    if (vret != VA_STATUS_SUCCESS)
    {
        ret = MIX_RESULT_FAIL;
        LOG_E( "Video driver returned error from vaBeginPicture\n");
        goto CLEAN_UP;
    }

    LOG_V( "Creating libva picture parameter buffer\n");

    //First the picture parameter buffer
    vret = vaCreateBuffer(
               vadisplay,
               vacontext,
               VAPictureParameterBufferType,
               sizeof(VAPictureParameterBufferVC1),
               1,
               pic_params,
               &buffer_ids[buffer_id_cnt]);

    buffer_id_cnt++;

    if (vret != VA_STATUS_SUCCESS) {
        ret = MIX_RESULT_FAIL;
        LOG_E( "Video driver returned error from vaCreateBuffer\n");
        goto CLEAN_UP;
    }
    LOG_V( "Creating libva bitplane buffer\n");

    if (pic_params->bitplane_present.value) {
        //Then the bitplane buffer
        vret = vaCreateBuffer(
                   vadisplay,
                   vacontext,
                   VABitPlaneBufferType,
                   pic_data->size_bitplanes,
                   1,
                   pic_data->packed_bitplanes,
                   &buffer_ids[buffer_id_cnt]);
        buffer_id_cnt++;
        if (vret != VA_STATUS_SUCCESS) {
            ret = MIX_RESULT_FAIL;
            LOG_E( "Video driver returned error from vaCreateBuffer\n");
            goto CLEAN_UP;
        }
    }

    //Now for slices
    for (uint32 i = 0; i < pic_data->num_slices; i++) {
        LOG_V( "Creating libva slice parameter buffer, for slice %d\n", i);

        //Do slice parameters
        vret = vaCreateBuffer(
                   vadisplay,
                   vacontext,
                   VASliceParameterBufferType,
                   sizeof(VASliceParameterBufferVC1),
                   1,
                   &(pic_data->slc_data[i].slc_parms),
                   &buffer_ids[buffer_id_cnt]);

        if (vret != VA_STATUS_SUCCESS) {
            ret = MIX_RESULT_FAIL;
            LOG_E( "Video driver returned error from vaCreateBuffer\n");
            goto CLEAN_UP;
        }

        buffer_id_cnt++;

        LOG_V( "Creating libva slice data buffer for slice %d, using slice address %x, with offset %d and size %u\n", i, (uint)pic_data->slc_data[i].buffer_addr, pic_data->slc_data[i].slc_parms.slice_data_offset, pic_data->slc_data[i].slice_size);


        //Do slice data
        vret = vaCreateBuffer(
                   vadisplay,
                   vacontext,
                   VASliceDataBufferType,
                   //size
                   pic_data->slc_data[i].slice_size,
                   //num_elements
                   1,
                   //slice data buffer pointer
                   //Note that this is the original data buffer ptr;
                   // offset to the actual slice data is provided in
                   // slice_data_offset in VASliceParameterBufferVC1
                   pic_data->slc_data[i].buffer_addr + pic_data->slc_data[i].slice_offset,
                   &buffer_ids[buffer_id_cnt]);

        buffer_id_cnt++;

        if (vret != VA_STATUS_SUCCESS) {
            ret = MIX_RESULT_FAIL;
            LOG_E( "Video driver returned error from vaCreateBuffer\n");
            goto CLEAN_UP;
        }
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

    LOG_V( "Calling vaEndPicture\n");

    //End picture
    vret = vaEndPicture(vadisplay, vacontext);

    if (vret != VA_STATUS_SUCCESS) {
        ret = MIX_RESULT_FAIL;
        LOG_E( "Video driver returned error from vaEndPicture\n");
        goto CLEAN_UP;
    }

#if 0   /* we don't call vaSyncSurface here, the call is moved to mix_video_render() */
    LOG_V( "Calling vaSyncSurface\n");

    //Decode the picture
    vret = vaSyncSurface(vadisplay, surface);

    if (vret != VA_STATUS_SUCCESS) {
        ret = MIX_RESULT_FAIL;
        LOG_E( "Video driver returned error from vaSyncSurface\n");
        goto CLEAN_UP;
    }
#endif

CLEAN_UP:
    if (NULL != buffer_ids)
        free(buffer_ids);
    return ret;
}


MIX_RESULT MixVideoFormat_VC1::Flush() {
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    LOG_V( "Begin\n");
    uint32 pret = 0;
    /* Chainup parent method.
    	We are not chaining up to parent method for now.
     */
#if 0
    if (parent_class->flush)
    {
        return parent_class->flush(mix, msg);
    }
#endif
    Lock();

    //Clear the contents of inputbufqueue

    this->discontinuity_frame_in_progress = FALSE;
    this->current_timestamp = (uint64)-1;

    int i = 0;
    for (; i < 2; i++) {
        if (this->reference_frames[i] != NULL) {
            mix_videoframe_unref(this->reference_frames[i]);
            this->reference_frames[i] = NULL;
        }
    }

    //Call parser flush
    pret = vbp_flush(this->parser_handle);
    if (pret != VBP_OK)
        ret = MIX_RESULT_FAIL;

    Unlock();
    LOG_V( "End\n");
    return ret;
}

MIX_RESULT MixVideoFormat_VC1::EndOfStream() {
    MIX_RESULT ret = MIX_RESULT_SUCCESS;

    LOG_V( "Begin\n");
    /* Chainup parent method.
    	We are not chaining up to parent method for now.
     */
#if 0
    if (parent_class->eos)
    {
        return parent_class->eos(mix, msg);
    }
#endif

    //Call Frame Manager with _eos()
    ret = mix_framemanager_eos(this->framemgr);
    LOG_V( "End\n");
    return ret;
}


MIX_RESULT MixVideoFormat_VC1::_handle_ref_frames(
    enum _picture_type frame_type, MixVideoFrame * current_frame) {
    LOG_V( "Begin\n");
    if (NULL == current_frame) {
        LOG_E( "Null pointer passed in\n");
        return MIX_RESULT_NULL_PTR;
    }
    switch (frame_type) {
    case VC1_PTYPE_I:  // I frame type
    case VC1_PTYPE_P:  // P frame type
        LOG_V( "Refing reference frame %x\n", (uint) current_frame);
        mix_videoframe_ref(current_frame);

        //If we have B frames, we need to keep forward and backward reference frames
        if (this->haveBframes) {
            if (this->reference_frames[0] == NULL) { //should only happen on first frame
                this->reference_frames[0] = current_frame;
                //this->reference_frames[1] = NULL;
            } else if (this->reference_frames[1] == NULL) {//should only happen on second frame
                this->reference_frames[1] = current_frame;
            } else {
                LOG_V( "Releasing reference frame %x\n", (uint) this->reference_frames[0]);
                mix_videoframe_unref(this->reference_frames[0]);
                this->reference_frames[0] = this->reference_frames[1];
                this->reference_frames[1] = current_frame;
            }
        } else {//No B frames in this content, only need to keep the forward reference frame
            LOG_V( "Releasing reference frame %x\n", (uint) this->reference_frames[0]);
            if (this->reference_frames[0] != NULL)
                mix_videoframe_unref(this->reference_frames[0]);
            this->reference_frames[0] = current_frame;
        }
        break;
    case VC1_PTYPE_B:  // B or BI frame type (should not happen)
    case VC1_PTYPE_BI:
    default:
        LOG_E( "Wrong frame type for handling reference frames\n");
        return MIX_RESULT_FAIL;
        break;

    }
    LOG_V( "End\n");
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT MixVideoFormat_VC1::_process_decode(
    vbp_data_vc1 *data, uint64 timestamp, bool discontinuity) {

    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    bool unrefVideoFrame = FALSE;
    MixVideoFrame *frame = NULL;
    int num_pictures = 0;
    enum _picture_type frame_type = VC1_PTYPE_I;

    //TODO Partition this method into smaller methods
    LOG_V( "Begin\n");
    if (NULL == data) {
        LOG_E( "Null pointer passed in\n");
        return MIX_RESULT_NULL_PTR;
    }

    if (0 == data->num_pictures || NULL == data->pic_data) {
        return MIX_RESULT_INVALID_PARAM;
    }

    //Check for skipped frame
    //For skipped frames, we will reuse the last P or I frame surface and treat as P frame
    if (data->pic_data[0].picture_is_skipped == VC1_PTYPE_SKIPPED) {
        LOG_V( "mix_video vinfo:  Frame type is SKIPPED\n");
        if (this->lastFrame == NULL) {
            //we shouldn't get a skipped frame before we are able to get a real frame
            LOG_E( "Error for skipped frame, prev frame is NULL\n");
            ret = MIX_RESULT_DROPFRAME;
            goto CLEAN_UP;
        }

        //We don't worry about this memory allocation because SKIPPED is not a common case
        //Doing the allocation on the fly is a more efficient choice than trying to manage yet another pool
        MixVideoFrame *skip_frame = mix_videoframe_new();
        if (skip_frame == NULL) {
            ret = MIX_RESULT_NO_MEMORY;
            LOG_E( "Error allocating new video frame object for skipped frame\n");
            goto CLEAN_UP;
        }

        mix_videoframe_set_is_skipped(skip_frame, TRUE);
        //mix_videoframe_ref(skip_frame);
        mix_videoframe_ref(this->lastFrame);
        ulong frameid = VA_INVALID_SURFACE;
        mix_videoframe_get_frame_id(this->lastFrame, &frameid);
        mix_videoframe_set_frame_id(skip_frame, frameid);
        mix_videoframe_set_frame_type(skip_frame, (MixFrameType)VC1_PTYPE_P);
        mix_videoframe_set_real_frame(skip_frame, this->lastFrame);
        mix_videoframe_set_timestamp(skip_frame, timestamp);
        mix_videoframe_set_discontinuity(skip_frame, FALSE);
        LOG_V( "Processing skipped frame %x, frame_id set to %d, ts %"UINT64_FORMAT"\n",
               (uint)skip_frame, (uint)frameid, timestamp);
        //Process reference frames
        LOG_V( "Updating skipped frame forward/backward references for libva\n");
        _handle_ref_frames(VC1_PTYPE_P, skip_frame);
        //Enqueue the skipped frame using frame manager
        ret = mix_framemanager_enqueue(this->framemgr, skip_frame);
        goto CLEAN_UP;
    }

    ret = mix_surfacepool_get(this->surfacepool, &frame);
    if (ret != MIX_RESULT_SUCCESS) {
        LOG_E( "Error getting frame from surfacepool\n");
        goto CLEAN_UP;
    }
    unrefVideoFrame = TRUE;

    // TO DO: handle multiple frames parsed from a sample buffer
    num_pictures = (data->num_pictures > 1) ? 2 : 1;
    for (int index = 0; index < num_pictures; index++) {
        ret = _decode_a_picture(data, index, frame);
        if (ret != MIX_RESULT_SUCCESS) {
            LOG_E( "Failed to decode a picture.\n");
            goto CLEAN_UP;
        }
    }

    //Set the discontinuity flag
    mix_videoframe_set_discontinuity(frame, discontinuity);

    //Set the timestamp
    mix_videoframe_set_timestamp(frame, timestamp);

    // setup frame structure
    if (data->num_pictures > 1) {
        if (data->pic_data[0].pic_parms->picture_fields.bits.is_first_field)
            mix_videoframe_set_frame_structure(frame, VA_TOP_FIELD);
        else
            mix_videoframe_set_frame_structure(frame, VA_BOTTOM_FIELD);
    } else {
        mix_videoframe_set_frame_structure(frame, VA_FRAME_PICTURE);
    }

    frame_type = (_picture_type)data->pic_data[0].pic_parms->picture_fields.bits.picture_type;

    //For I or P frames
    //Save this frame off for skipped frame handling
    if ((frame_type == VC1_PTYPE_I) || (frame_type == VC1_PTYPE_P)) {
        if (this->lastFrame != NULL) {
            mix_videoframe_unref(this->lastFrame);
        }
        this->lastFrame = frame;
        mix_videoframe_ref(frame);
    }

    //Update the references frames for the current frame
    if ((frame_type == VC1_PTYPE_I) || (frame_type == VC1_PTYPE_P)) {//If I or P frame, update the reference array
        LOG_V( "Updating forward/backward references for libva\n");
        _handle_ref_frames(frame_type, frame);
    }


    LOG_V( "Enqueueing the frame with frame manager, timestamp %"UINT64_FORMAT"\n", timestamp);

    //Enqueue the decoded frame using frame manager
    ret = mix_framemanager_enqueue(this->framemgr, frame);

    if (ret != MIX_RESULT_SUCCESS) {
        LOG_E( "Error enqueuing frame object\n");
        goto CLEAN_UP;
    }
    unrefVideoFrame = FALSE;

CLEAN_UP:

    if (unrefVideoFrame)
        mix_videoframe_unref(frame);
    LOG_V( "End\n");
    return ret;
}

MIX_RESULT MixVideoFormat_VC1::_release_input_buffers(uint64 timestamp) {

    LOG_V( "Begin\n");

    // Nothing to release. Deprecated.

    LOG_V( "End\n");
    return MIX_RESULT_SUCCESS;
}


