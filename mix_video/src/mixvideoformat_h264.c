/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */
#include <glib.h>
#include <math.h>
#ifndef ANDROID
#include <va/va_x11.h>
#endif

#include "mixvideolog.h"
#include "mixvideoformat_h264.h"

#ifdef MIX_LOG_ENABLE
static int mix_video_h264_counter = 0;
#endif /* MIX_LOG_ENABLE */

/* The parent class. The pointer will be saved
 * in this class's initialization. The pointer
 * can be used for chaining method call if needed.
 */
static MixVideoFormatClass *parent_class = NULL;

static void mix_videoformat_h264_finalize(GObject * obj);

/*
 * Please note that the type we pass to G_DEFINE_TYPE is MIX_TYPE_VIDEOFORMAT
 */
G_DEFINE_TYPE (MixVideoFormat_H264, mix_videoformat_h264, MIX_TYPE_VIDEOFORMAT);

static void mix_videoformat_h264_init(MixVideoFormat_H264 * self) {
	MixVideoFormat *parent = MIX_VIDEOFORMAT(self);

	/* public member initialization */
	/* These are all public because MixVideoFormat objects are completely internal to MixVideo,
		no need for private members  */
	self->dpb_surface_table = NULL;
#ifdef DECODER_ROBUSTNESS
	self->last_decoded_frame = NULL;
#endif

	/* NOTE: we don't need to do this here.
	 * This just demostrates how to access
	 * member varibles beloned to parent
	 */
	parent->initialized = FALSE;
}

static void mix_videoformat_h264_class_init(
		MixVideoFormat_H264Class * klass) {

	/* root class */
	GObjectClass *gobject_class = (GObjectClass *) klass;

	/* direct parent class */
	MixVideoFormatClass *video_format_class =
			MIX_VIDEOFORMAT_CLASS(klass);

	/* parent class for later use */
	parent_class = g_type_class_peek_parent(klass);

	/* setup finializer */
	gobject_class->finalize = mix_videoformat_h264_finalize;

	/* setup vmethods with base implementation */
	/* This is where we can override base class methods if needed */
	video_format_class->getcaps = mix_videofmt_h264_getcaps;
	video_format_class->initialize = mix_videofmt_h264_initialize;
	video_format_class->decode = mix_videofmt_h264_decode;
	video_format_class->flush = mix_videofmt_h264_flush;
	video_format_class->eos = mix_videofmt_h264_eos;
	video_format_class->deinitialize = mix_videofmt_h264_deinitialize;
}

MixVideoFormat_H264 *
mix_videoformat_h264_new(void) {
	MixVideoFormat_H264 *ret =
			g_object_new(MIX_TYPE_VIDEOFORMAT_H264, NULL);

	return ret;
}

void mix_videoformat_h264_finalize(GObject * obj) {
	gint32 pret = VBP_OK;

	/* clean up here. */

	MixVideoFormat *parent = NULL;
	MixVideoFormat_H264 *self = MIX_VIDEOFORMAT_H264(obj);
	GObjectClass *root_class = (GObjectClass *) parent_class;

	parent = MIX_VIDEOFORMAT(self);

	//surfacepool is deallocated by parent
	//inputbufqueue is deallocated by parent
	//parent calls vaDestroyConfig, vaDestroyContext and vaDestroySurfaces

	if (self->dpb_surface_table) {
		//Free the DPB surface table
		//First remove all the entries (frames will be unrefed)
		g_hash_table_remove_all(self->dpb_surface_table);
		//Then unref the table
		g_hash_table_unref(self->dpb_surface_table);
		self->dpb_surface_table = NULL;
	}

	g_mutex_lock(parent->objectlock);
	parent->initialized = TRUE;
	parent->parse_in_progress = FALSE;

	//Close the parser
    pret = vbp_close(parent->parser_handle);
	parent->parser_handle = NULL;
	if (pret != VBP_OK)
	{
		LOG_E( "Error closing parser\n");
	}

	g_mutex_unlock(parent->objectlock);

	/* Chain up parent */
	if (root_class->finalize) {
		root_class->finalize(obj);
	}
}

MixVideoFormat_H264 *
mix_videoformat_h264_ref(MixVideoFormat_H264 * mix) {
	return (MixVideoFormat_H264 *) g_object_ref(G_OBJECT(mix));
}

/*  H.264 vmethods implementation */
MIX_RESULT mix_videofmt_h264_getcaps(MixVideoFormat *mix, GString *msg) {

    MIX_RESULT ret = MIX_RESULT_SUCCESS;

	if (mix == NULL || msg == NULL)
	{
		LOG_E( "NUll pointer passed in\n");
		return MIX_RESULT_NULL_PTR;
	}

	LOG_V( "Begin\n");

	/* Chainup parent method.
	 */

	if (parent_class->getcaps) {
		ret = parent_class->getcaps(mix, msg);
	}

	LOG_V( "End\n");

	return ret;
}

MIX_RESULT mix_videofmt_h264_initialize_va(
    MixVideoFormat *mix,
    vbp_data_h264 *data)
{
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    VAStatus vret = VA_STATUS_SUCCESS;
    VAConfigAttrib attrib;

    MixVideoFormat *parent = MIX_VIDEOFORMAT(mix);
    MixVideoFormat_H264 *self = MIX_VIDEOFORMAT_H264(mix);

    if (parent->va_initialized)
    {
        LOG_W("va already initialized.\n");
        return MIX_RESULT_SUCCESS;
    }


    LOG_V( "Begin\n");

    //We are requesting RT attributes
    attrib.type = VAConfigAttribRTFormat;
    attrib.value = VA_RT_FORMAT_YUV420;

    //Initialize and save the VA config ID
    //We use high profile for all kinds of H.264 profiles (baseline, main and high)
    vret = vaCreateConfig(
        parent->va_display,
        VAProfileH264High,
        VAEntrypointVLD,
        &attrib,
        1,
        &(parent->va_config));

    if (vret != VA_STATUS_SUCCESS)
    {
        ret = MIX_RESULT_FAIL;
        LOG_E("vaCreateConfig failed\n");
        goto cleanup;
    }

    LOG_V( "Codec data says num_ref_frames is %d\n", data->codec_data->num_ref_frames);


    // handle both frame and field coding for interlaced content
    int num_ref_pictures = data->codec_data->num_ref_frames;


    //Adding 1 to work around VBLANK issue, and another 1 to compensate cached frame that
    // will not start decoding until a new frame is received.
    parent->va_num_surfaces = 1 + 1 + parent->extra_surfaces + (((num_ref_pictures + 3) <
        MIX_VIDEO_H264_SURFACE_NUM) ?
        (num_ref_pictures + 3)
        : MIX_VIDEO_H264_SURFACE_NUM);

    parent->va_surfaces = g_malloc(sizeof(VASurfaceID)*parent->va_num_surfaces);
    if (parent->va_surfaces == NULL)
    {
        ret = MIX_RESULT_FAIL;
        LOG_E( "parent->va_surfaces == NULL. \n");
        goto cleanup;
    }

    LOG_V( "Codec data says picture size is %d x %d\n",
        (data->pic_data[0].pic_parms->picture_width_in_mbs_minus1 + 1) * 16,
        (data->pic_data[0].pic_parms->picture_height_in_mbs_minus1 + 1) * 16);
    LOG_V( "getcaps says picture size is %d x %d\n", parent->picture_width, parent->picture_height);

    vret = vaCreateSurfaces(
        parent->va_display,
        (data->pic_data[0].pic_parms->picture_width_in_mbs_minus1 + 1) * 16,
        (data->pic_data[0].pic_parms->picture_height_in_mbs_minus1 + 1) * 16,
        VA_RT_FORMAT_YUV420,
        parent->va_num_surfaces,
        parent->va_surfaces);

    if (vret != VA_STATUS_SUCCESS)
    {
        ret = MIX_RESULT_FAIL;
        LOG_E( "Error allocating surfaces\n");
        goto cleanup;
    }

    LOG_V( "Created %d libva surfaces\n", parent->va_num_surfaces);

    //Initialize the surface pool
    ret = mix_surfacepool_initialize(
        parent->surfacepool,
        parent->va_surfaces,
        parent->va_num_surfaces,
        parent->va_display);

    switch (ret)
    {
    case MIX_RESULT_SUCCESS:
        break;
    case MIX_RESULT_ALREADY_INIT:  //This case is for future use when we can be  initialized multiple times.  It is to detect when we have not been reset before re-initializing.
    default:
        ret = MIX_RESULT_ALREADY_INIT;
        LOG_E( "Error init surface pool\n");
        goto cleanup;
        break;
    }

    if (data->codec_data->pic_order_cnt_type == 0)
    {
        int max = (int)pow(2, data->codec_data->log2_max_pic_order_cnt_lsb_minus4 + 4);
        mix_framemanager_set_max_picture_number(parent->framemgr, max);
    }

    //Initialize and save the VA context ID
    //Note: VA_PROGRESSIVE libva flag is only relevant to MPEG2
    vret = vaCreateContext(
        parent->va_display,
        parent->va_config,
        parent->picture_width,
        parent->picture_height,
        0,  // no flag set
        parent->va_surfaces,
        parent->va_num_surfaces,
        &(parent->va_context));

    if (vret != VA_STATUS_SUCCESS)
    {
        ret = MIX_RESULT_FAIL;
        LOG_E( "Error initializing video driver\n");
        goto cleanup;
    }

    parent->va_initialized = TRUE;

cleanup:
    /* nothing to clean up */

    return ret;

}


MIX_RESULT mix_videofmt_h264_update_ref_pic_list(
    MixVideoFormat *mix,
    VAPictureParameterBufferH264* picture_params,
    VASliceParameterBufferH264* slice_params)
{
    MixVideoFormat_H264 *self = MIX_VIDEOFORMAT_H264(mix);

    //Do slice parameters

    //First patch up the List0 and List1 surface IDs
    int j = 0;
    guint poc = 0;
    gpointer video_frame = NULL;
    MIX_RESULT ret = MIX_RESULT_SUCCESS;

    for (; j <= slice_params->num_ref_idx_l0_active_minus1; j++)
    {
        if (!(slice_params->RefPicList0[j].flags & VA_PICTURE_H264_INVALID))
        {
            poc = mix_videofmt_h264_get_poc(&(slice_params->RefPicList0[j]));
            video_frame = g_hash_table_lookup(self->dpb_surface_table, (gpointer)poc);
            if (video_frame == NULL)
            {
                LOG_E("unable to find surface of picture %d (current picture %d).",
                    poc, mix_videofmt_h264_get_poc(&(picture_params->CurrPic)));
                ret = MIX_RESULT_DROPFRAME;  //return non-fatal error
                goto cleanup;
            }
            else
            {
                slice_params->RefPicList0[j].picture_id =
                    ((MixVideoFrame *)video_frame)->frame_id;
            }
        }

    }

    if ((slice_params->slice_type == 1) || (slice_params->slice_type == 6))
    {
        for (j = 0; j <= slice_params->num_ref_idx_l1_active_minus1; j++)
        {
            if (!(slice_params->RefPicList1[j].flags & VA_PICTURE_H264_INVALID))
            {
                poc = mix_videofmt_h264_get_poc(&(slice_params->RefPicList1[j]));
                video_frame = g_hash_table_lookup(self->dpb_surface_table, (gpointer)poc);
                if (video_frame == NULL)
                {
                    LOG_E("unable to find surface of picture %d (current picture %d).",
                    poc, mix_videofmt_h264_get_poc(&(picture_params->CurrPic)));
                    ret = MIX_RESULT_DROPFRAME;  //return non-fatal error
                    goto cleanup;
                }
                else
                {
                    slice_params->RefPicList1[j].picture_id =
                        ((MixVideoFrame *)video_frame)->frame_id;
                }
            }
        }
    }

cleanup:
    // do nothing

    return ret;
}


MIX_RESULT mix_videofmt_h264_decode_a_slice(
    MixVideoFormat *mix,
    vbp_data_h264 *data,
    int picture_index,
    int slice_index)
{
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    VAStatus vret = VA_STATUS_SUCCESS;
    VADisplay vadisplay = NULL;
    VAContextID vacontext;
    guint buffer_id_cnt = 0;

    // maximum 4 buffers to render a slice: picture parameter, IQMatrix, slice parameter, slice data
    VABufferID buffer_ids[4];


    LOG_V( "Begin\n");

    //MixVideoFormat_H264 *self = MIX_VIDEOFORMAT_H264(mix);

    vbp_picture_data_h264* pic_data = &(data->pic_data[picture_index]);
    vbp_slice_data_h264* slice_data = &(pic_data->slc_data[slice_index]);
    VAPictureParameterBufferH264 *pic_params = pic_data->pic_parms;
    VASliceParameterBufferH264* slice_params = &(slice_data->slc_parms);
    vadisplay = mix->va_display;
    vacontext = mix->va_context;

#ifdef DECODER_ROBUSTNESS
    if ((slice_params->first_mb_in_slice == 0) || (!mix->end_picture_pending))
#else
    if (slice_params->first_mb_in_slice == 0)
#endif
    {
        // this is the first slice of the picture
        if (mix->end_picture_pending)
        {
            // interlace content, decoding the first field
            vret = vaEndPicture(vadisplay, vacontext);
            if (vret != VA_STATUS_SUCCESS)
            {
                ret = MIX_RESULT_FAIL;
                LOG_E("vaEndPicture failed.\n");
                goto cleanup;
            }

            // for interlace content, top field may be valid only after the second field is parsed
            mix_videoframe_set_displayorder(mix->video_frame, pic_params->CurrPic.TopFieldOrderCnt);
        }

        gulong surface = 0;

        LOG_V("mix->video_frame = 0x%x\n", mix->video_frame);

        //Get our surface ID from the frame object
        ret = mix_videoframe_get_frame_id(mix->video_frame, &surface);
        if (ret != MIX_RESULT_SUCCESS)
        {
            LOG_E( "Error getting surface ID from frame object\n");
            goto cleanup;
        }

#ifdef DECODER_ROBUSTNESS
        LOG_V( "Updating DPB for libva\n");

        //Now handle the reference frames and surface IDs for DPB and current frame
        mix_videofmt_h264_handle_ref_frames(mix, pic_params, mix->video_frame);

#ifdef HACK_DPB
        //We have to provide a hacked DPB rather than complete DPB for libva as workaround
        ret = mix_videofmt_h264_hack_dpb(mix, pic_data);
        if (ret != MIX_RESULT_SUCCESS)
        {
		LOG_E( "Error reference frame not found\n");
		//Need to remove the frame we inserted in _handle_ref_frames above, since we are not going to decode it
		mix_videofmt_h264_cleanup_ref_frame(mix, pic_params, mix->frame);
		goto cleanup;
        }
#endif

        LOG_V( "Calling vaBeginPicture\n");

        //Now we can begin the picture
        vret = vaBeginPicture(vadisplay, vacontext, surface);
        if (vret != VA_STATUS_SUCCESS)
        {
            ret = MIX_RESULT_FAIL;
            LOG_E( "Video driver returned error from vaBeginPicture\n");
            goto cleanup;
        }

        // vaBeginPicture needs a matching vaEndPicture
        mix->end_picture_pending = TRUE;

#else
        LOG_V( "Calling vaBeginPicture\n");

        //Now we can begin the picture
        vret = vaBeginPicture(vadisplay, vacontext, surface);
        if (vret != VA_STATUS_SUCCESS)
        {
            ret = MIX_RESULT_FAIL;
            LOG_E( "Video driver returned error from vaBeginPicture\n");
            goto cleanup;
        }

        // vaBeginPicture needs a matching vaEndPicture
        mix->end_picture_pending = TRUE;

        LOG_V( "Updating DPB for libva\n");

        //Now handle the reference frames and surface IDs for DPB and current frame
        mix_videofmt_h264_handle_ref_frames(mix, pic_params, mix->video_frame);

#ifdef HACK_DPB
        //We have to provide a hacked DPB rather than complete DPB for libva as workaround
        ret = mix_videofmt_h264_hack_dpb(mix, pic_data);
        if (ret != MIX_RESULT_SUCCESS)
        {
		LOG_E( "Error reference frame not found\n");
		goto cleanup;
        }
#endif

#endif

        //Libva buffer set up


        LOG_V( "Creating libva picture parameter buffer\n");
        LOG_V( "picture parameter buffer shows num_ref_frames is %d\n", pic_params->num_ref_frames);

        //First the picture parameter buffer
        vret = vaCreateBuffer(
            vadisplay,
            vacontext,
            VAPictureParameterBufferType,
            sizeof(VAPictureParameterBufferH264),
            1,
            pic_params,
            &buffer_ids[buffer_id_cnt]);

        if (vret != VA_STATUS_SUCCESS)
        {
            ret = MIX_RESULT_FAIL;
            LOG_E( "Video driver returned error from vaCreateBuffer\n");
            goto cleanup;
        }

        buffer_id_cnt++;

        LOG_V( "Creating libva IQMatrix buffer\n");


        //Then the IQ matrix buffer
        vret = vaCreateBuffer(
            vadisplay,
            vacontext,
            VAIQMatrixBufferType,
            sizeof(VAIQMatrixBufferH264),
            1,
            data->IQ_matrix_buf,
            &buffer_ids[buffer_id_cnt]);

        if (vret != VA_STATUS_SUCCESS)
        {
            ret = MIX_RESULT_FAIL;
            LOG_E( "Video driver returned error from vaCreateBuffer\n");
            goto cleanup;
        }
        buffer_id_cnt++;
    }

#ifndef DECODER_ROBUSTNESS
    if (!mix->end_picture_pending)
    {
        LOG_E("first slice is lost??????????\n");
        ret = MIX_RESULT_DROPFRAME;
        goto cleanup;
    }
#endif

    //Now for slices

    ret = mix_videofmt_h264_update_ref_pic_list(mix, pic_params, slice_params);
    if (ret != MIX_RESULT_SUCCESS)
    {
        LOG_E("mix_videofmt_h264_update_ref_pic_list failed.\n");
        goto cleanup;
    }

    LOG_V( "Creating libva slice parameter buffer\n");

    vret = vaCreateBuffer(
        vadisplay,
        vacontext,
        VASliceParameterBufferType,
        sizeof(VASliceParameterBufferH264),
        1,
        slice_params,
        &buffer_ids[buffer_id_cnt]);

    if (vret != VA_STATUS_SUCCESS)
    {
        ret = MIX_RESULT_FAIL;
        LOG_E( "Video driver returned error from vaCreateBuffer\n");
        goto cleanup;
    }

    buffer_id_cnt++;


    //Do slice data

    //slice data buffer pointer
    //Note that this is the original data buffer ptr;
    // offset to the actual slice data is provided in
    // slice_data_offset in VASliceParameterBufferH264

    LOG_V( "Creating libva slice data buffer, using slice address %x, with offset %d and size %u\n",
        (guint)slice_data->buffer_addr, slice_params->slice_data_offset, slice_data->slice_size);

    vret = vaCreateBuffer(
        vadisplay,
        vacontext,
        VASliceDataBufferType,
        slice_data->slice_size, //size
        1,        //num_elements
        slice_data->buffer_addr + slice_data->slice_offset,
        &buffer_ids[buffer_id_cnt]);

    buffer_id_cnt++;

    if (vret != VA_STATUS_SUCCESS)
    {
        ret = MIX_RESULT_FAIL;
        LOG_E( "Video driver returned error from vaCreateBuffer\n");
        goto cleanup;
    }


    LOG_V( "Calling vaRenderPicture\n");

    //Render the picture
    vret = vaRenderPicture(
        vadisplay,
        vacontext,
        buffer_ids,
        buffer_id_cnt);

    if (vret != VA_STATUS_SUCCESS)
    {
        ret = MIX_RESULT_FAIL;
        LOG_E( "Video driver returned error from vaRenderPicture\n");
        goto cleanup;
    }


cleanup:
    LOG_V( "End\n");

    return ret;

}


MIX_RESULT mix_videofmt_h264_decode_end(
    MixVideoFormat *mix,
    gboolean drop_picture)
{
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    VAStatus vret = VA_STATUS_SUCCESS;
    MixVideoFormat* parent = MIX_VIDEOFORMAT(mix);
#ifdef DECODER_ROBUSTNESS
    MixVideoFormat_H264 *self = MIX_VIDEOFORMAT_H264(mix);
#else
    //MixVideoFormat_H264 *self = MIX_VIDEOFORMAT_H264(mix);
#endif

    LOG_V("Begin\n");

    if (!parent->end_picture_pending)
    {
        if (parent->video_frame)
        {
            ret = MIX_RESULT_FAIL;
            LOG_E("Unexpected: video_frame is not unreferenced.\n");
        }
        goto cleanup;
    }

    if (parent->video_frame == NULL)
    {
        ret = MIX_RESULT_FAIL;
        LOG_E("Unexpected: video_frame has been unreferenced.\n");
        goto cleanup;
    }

	LOG_V( "Calling vaEndPicture\n");
    vret = vaEndPicture(parent->va_display, parent->va_context);

    if (vret != VA_STATUS_SUCCESS)
    {
        ret = MIX_RESULT_FAIL;
        LOG_E( "Video driver returned error from vaEndPicture\n");
        goto cleanup;
    }

#if 0	/* we don't call vaSyncSurface here, the call is moved to mix_video_render() */

    LOG_V( "Calling vaSyncSurface\n");

    //Decode the picture
    vret = vaSyncSurface(parent->va_display, parent->video_frame->frame_id);

    if (vret != VA_STATUS_SUCCESS)
    {
        ret = MIX_RESULT_FAIL;
        LOG_E( "Video driver returned error from vaSyncSurface\n");
        goto cleanup;
    }
#endif

    if (drop_picture)
    {
        // we are asked to drop this decoded picture
        mix_videoframe_unref(parent->video_frame);
        parent->video_frame = NULL;
        goto cleanup;
    }

	LOG_V( "Enqueueing the frame with frame manager, timestamp %"G_GINT64_FORMAT"\n",
	    parent->video_frame->timestamp);

#ifdef DECODER_ROBUSTNESS
	if (self->last_decoded_frame)
		mix_videoframe_unref(self->last_decoded_frame);
	self->last_decoded_frame = parent->video_frame;
	mix_videoframe_ref(self->last_decoded_frame);
#endif

    //Enqueue the decoded frame using frame manager
    ret = mix_framemanager_enqueue(parent->framemgr, parent->video_frame);
    if (ret != MIX_RESULT_SUCCESS)
    {
        LOG_E( "Error enqueuing frame object\n");
        goto cleanup;
    }
    else
    {
        // video frame is passed to frame manager
        parent->video_frame = NULL;
        LOG_V("video_frame is assigned to be NULL !\n");
    }

cleanup:
    if (parent->video_frame)
    {
        /* this always indicates an error */
        mix_videoframe_unref(parent->video_frame);
        parent->video_frame = NULL;
    }
    parent->end_picture_pending = FALSE;
    LOG_V("End\n");
    return ret;

}


MIX_RESULT mix_videofmt_h264_decode_continue(
    MixVideoFormat *mix,
    vbp_data_h264 *data)
{
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    int i, j;
    vbp_picture_data_h264* pic_data = NULL;

    LOG_V("Begin\n");

    for (i = 0; i < data->num_pictures; i++)
    {
        pic_data = &(data->pic_data[i]);
        if (pic_data->pic_parms == NULL)
        {
            ret = MIX_RESULT_FAIL;
            LOG_E("pic_data->pic_parms is NULL.\n");
            goto cleanup;
        }

        if (pic_data->slc_data == NULL)
        {
            ret = MIX_RESULT_FAIL;
            LOG_E("pic_data->slc_data is NULL.\n");
            goto cleanup;
        }

        if (pic_data->num_slices == 0)
        {
            ret = MIX_RESULT_FAIL;
            LOG_E("pic_data->num_slices == 0.\n");
            goto cleanup;
        }

	    LOG_V( "num_slices is %d\n", pic_data->num_slices);
        for (j = 0; j < pic_data->num_slices; j++)
        {
    	    LOG_V( "Decoding slice %d\n", j);
	        ret = mix_videofmt_h264_decode_a_slice(mix, data, i, j);
            if (ret != 	MIX_RESULT_SUCCESS)
            {
                LOG_E( "mix_videofmt_h264_decode_a_slice failed, error =  %#X.", ret);
                goto cleanup;
           }
        }
    }

cleanup:
    // nothing to cleanup;

    LOG_V("End\n");
    return ret;
}


MIX_RESULT mix_videofmt_h264_set_frame_type(
    MixVideoFormat *mix,
    vbp_data_h264 *data)
{
    MIX_RESULT ret = MIX_RESULT_SUCCESS;

    //Set the picture type (I, B or P frame)
    //For H.264 we use the first encountered slice type for this (check - may need to change later to search all slices for B type)
    MixFrameType frame_type = TYPE_INVALID;

    switch (data->pic_data[0].slc_data[0].slc_parms.slice_type)
    {
    case 0:
    case 3:
    case 5:
    case 8:
        frame_type = TYPE_P;
        break;
    case 1:
    case 6:
        frame_type = TYPE_B;
        break;
    case 2:
    case 4:
    case 7:
    case 9:
        frame_type = TYPE_I;
        break;
    default:
        break;
    }

    //Do not have to check for B frames after a seek
    //Note:  Demux should seek to IDR (instantaneous decoding refresh) frame, otherwise
    //  DPB will not be correct and frames may come in with invalid references
    //  This will be detected when DPB is checked for valid mapped surfaces and
    //  error returned from there.

	LOG_V( "frame type is %d\n", frame_type);

    //Set the frame type for the frame object (used in reordering by frame manager)
    ret = mix_videoframe_set_frame_type(mix->video_frame, frame_type);

    if (ret != MIX_RESULT_SUCCESS)
    {
        LOG_E( "Error setting frame type on frame\n");
    }

    return ret;
}


MIX_RESULT mix_videofmt_h264_set_frame_structure(
    MixVideoFormat *mix,
    vbp_data_h264 *data)
{
    MIX_RESULT ret = MIX_RESULT_SUCCESS;

    if (data->pic_data[0].pic_parms->CurrPic.flags & VA_PICTURE_H264_TOP_FIELD)
    {
        mix_videoframe_set_frame_structure(mix->video_frame, VA_BOTTOM_FIELD | VA_TOP_FIELD);
    }
    else
    {
        mix_videoframe_set_frame_structure(mix->video_frame, VA_FRAME_PICTURE);
    }

    return ret;
}


MIX_RESULT mix_videofmt_h264_decode_begin(
    MixVideoFormat *mix,
    vbp_data_h264 *data)
{
    MIX_RESULT ret = MIX_RESULT_SUCCESS;

    //Get a frame from the surface pool
    LOG_V("Begin\n");
    ret = mix_surfacepool_get(mix->surfacepool, &(mix->video_frame));

    if (ret != MIX_RESULT_SUCCESS)
    {
        LOG_E( "Error getting frame from surfacepool\n");
        return ret;
    }

    /* the following calls will always succeed */

    // set frame type
    ret = mix_videofmt_h264_set_frame_type(mix, data);

    // set frame structure
    ret = mix_videofmt_h264_set_frame_structure(mix, data);

    //Set the discontinuity flag
    mix_videoframe_set_discontinuity(mix->video_frame, mix->discontinuity_frame_in_progress);

    //Set the timestamp
    mix_videoframe_set_timestamp(mix->video_frame, mix->current_timestamp);

    // Set displayorder
    ret = mix_videoframe_set_displayorder(mix->video_frame,
        data->pic_data[0].pic_parms->CurrPic.TopFieldOrderCnt);
    if(ret != MIX_RESULT_SUCCESS)
    {
        LOG_E("Error setting displayorder\n");
        return ret;
    }

    ret = mix_videofmt_h264_decode_continue(mix, data);

    LOG_V("End\n");
    return ret;

}


MIX_RESULT mix_videofmt_h264_decode_a_buffer(
    MixVideoFormat *mix,
    MixBuffer * bufin,
    guint64 ts,
    gboolean discontinuity)
{
    uint32 pret = 0;
    MixVideoFormat *parent = NULL;
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    vbp_data_h264 *data = NULL;

    LOG_V( "Begin\n");

    parent = MIX_VIDEOFORMAT(mix);

    LOG_V( "Calling parse for current frame, parse handle %d\n", (int)parent->parser_handle);
    pret = vbp_parse(parent->parser_handle,
        bufin->data,
        bufin->size,
        FALSE);

    LOG_V( "Called parse for current frame\n");
    if ((pret != VBP_DONE) &&(pret != VBP_OK))
    {
        ret = MIX_RESULT_DROPFRAME;
        LOG_E( "vbp_parse failed.\n");
        goto cleanup;
    }

    //query for data
    pret = vbp_query(parent->parser_handle, (void *) &data);

    if ((pret != VBP_OK) || (data == NULL))
    {
        ret = MIX_RESULT_FAIL;
        LOG_E( "vbp_query failed.\n");
        goto cleanup;
    }
    LOG_V( "Called query for current frame\n");


    if (data->has_sps == 0 || data->has_pps == 0)
    {
        ret = MIX_RESULT_SUCCESS;
        LOG_V("SPS or PPS is not available.\n");
        goto cleanup;
    }

    if (parent->va_initialized == FALSE)
    {
        LOG_V("try initializing VA...\n");
        ret = mix_videofmt_h264_initialize_va(parent, data);
        if (ret != MIX_RESULT_SUCCESS)
        {
            LOG_V("mix_videofmt_h264_initialize_va failed.\n");
            goto cleanup;
        }
    }

    // first pic_data always exists, check if any slice is parsed
    if (data->pic_data[0].num_slices == 0)
    {
        ret = MIX_RESULT_SUCCESS;
        LOG_V("slice is not available.\n");
        goto cleanup;
    }

    guint64 last_ts = parent->current_timestamp;
    parent->current_timestamp = ts;
    parent->discontinuity_frame_in_progress = discontinuity;

    LOG_V("ts = %lli last_ts = %lli\n", ts, last_ts);

    if (last_ts != ts)
    {
        // finish decoding the last frame
        ret = mix_videofmt_h264_decode_end(parent, FALSE);
        if (ret != MIX_RESULT_SUCCESS)
        {
            LOG_V("mix_videofmt_h264_decode_end failed.\n");
            goto cleanup;
        }

        // start decoding a new frame
        ret = mix_videofmt_h264_decode_begin(parent, data);
        if (ret != MIX_RESULT_SUCCESS)
        {
            LOG_V("mix_videofmt_h264_decode_begin failed.\n");
            goto cleanup;
        }
    }
    else
    {
        // parital frame
        LOG_V("partial frame handling...\n");
        ret = mix_videofmt_h264_decode_continue(parent, data);
        if (ret != MIX_RESULT_SUCCESS)
        {
            LOG_V("mix_videofmt_h264_decode_continue failed.\n");
            goto cleanup;
        }
    }

    cleanup:

    LOG_V( "End\n");

    return ret;
}


MIX_RESULT mix_videofmt_h264_initialize(MixVideoFormat *mix,
		MixVideoConfigParamsDec * config_params,
        MixFrameManager * frame_mgr,
		MixBufferPool * input_buf_pool,
		MixSurfacePool ** surface_pool,
		VADisplay va_display ) {

    uint32 pret = 0;
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    enum _vbp_parser_type ptype = VBP_H264;
    vbp_data_h264 *data = NULL;
    MixVideoFormat *parent = NULL;
    MixIOVec *header = NULL;
    guint pic_width_in_codec_data = 0;
    guint pic_height_in_codec_data = 0;

    if (mix == NULL || config_params == NULL || frame_mgr == NULL || input_buf_pool == NULL || va_display == NULL)
    {
        LOG_E( "NUll pointer passed in\n");
        return MIX_RESULT_NULL_PTR;
    }

    LOG_V( "Begin\n");

    /* Chainup parent method. */

    if (parent_class->initialize) {
        ret = parent_class->initialize(mix, config_params,
            frame_mgr, input_buf_pool, surface_pool,
            va_display);
    }

    if (ret != MIX_RESULT_SUCCESS)
    {
        LOG_E( "Error initializing\n");
        return ret;
    }

    if (!MIX_IS_VIDEOFORMAT_H264(mix))
       return MIX_RESULT_INVALID_PARAM;

    parent = MIX_VIDEOFORMAT(mix);
    MixVideoFormat_H264 *self = MIX_VIDEOFORMAT_H264(mix);

    LOG_V( "Locking\n");
    //From now on, we exit this function through cleanup:
    g_mutex_lock(parent->objectlock);

    parent->surfacepool = mix_surfacepool_new();
    *surface_pool = parent->surfacepool;

    if (parent->surfacepool == NULL)
    {
        ret = MIX_RESULT_NO_MEMORY;
        LOG_E( "parent->surfacepool == NULL.\n");
        goto cleanup;
    }

    //Create our table of Decoded Picture Buffer "in use" surfaces
    self->dpb_surface_table = g_hash_table_new_full(
        NULL,
        NULL,
        mix_videofmt_h264_destroy_DPB_key,
        mix_videofmt_h264_destroy_DPB_value);

    if (self->dpb_surface_table == NULL)
    {
        ret = MIX_RESULT_NO_MEMORY;
        LOG_E( "Error allocating dbp surface table\n");
        goto cleanup;  //leave this goto here in case other code is added between here and cleanup label
    }

    ret = mix_videoconfigparamsdec_get_extra_surface_allocation(
        config_params,
        &parent->extra_surfaces);

    if (ret != MIX_RESULT_SUCCESS)
    {
        ret = MIX_RESULT_FAIL;
        LOG_E( "Cannot get extra surface allocation setting\n");
        goto cleanup;
    }

	LOG_V( "Before vbp_open\n");
    //Load the bitstream parser
    pret = vbp_open(ptype, &(parent->parser_handle));
	LOG_V( "After vbp_open\n");

    if (!(pret == VBP_OK))
    {
        ret = MIX_RESULT_FAIL;
        LOG_E( "Error opening parser\n");
        goto cleanup;
    }
    LOG_V( "Opened parser\n");


    ret = mix_videoconfigparamsdec_get_header(config_params, &header);

    if ((ret != MIX_RESULT_SUCCESS) || (header == NULL))
    {
        // Delay initializing VA if codec configuration data is not ready, but don't return an error.
        ret = MIX_RESULT_SUCCESS;
        LOG_W( "Codec data is not available in the configuration parameter.\n");
        goto cleanup;
    }

    LOG_V( "Calling parse on header data, handle %d\n", (int)parent->parser_handle);

    pret = vbp_parse(
        parent->parser_handle,
        header->data,
        header->data_size,
        TRUE);

    if (!((pret == VBP_OK) || (pret == VBP_DONE)))
    {
        ret = MIX_RESULT_FAIL;
        LOG_E( "Error parsing header data\n");
        goto cleanup;
    }

    LOG_V( "Parsed header\n");

    //Get the header data and save
    pret = vbp_query(parent->parser_handle, (void *)&data);

    if ((pret != VBP_OK) || (data == NULL))
    {
        ret = MIX_RESULT_FAIL;
        LOG_E( "Error reading parsed header data\n");
        goto cleanup;
    }

    LOG_V( "Queried parser for header data\n");

    // Update the pic size according to the parsed codec_data
    pic_width_in_codec_data  = (data->pic_data[0].pic_parms->picture_width_in_mbs_minus1 + 1) * 16;
    pic_height_in_codec_data = (data->pic_data[0].pic_parms->picture_height_in_mbs_minus1 + 1) * 16;
    mix_videoconfigparamsdec_set_picture_res (config_params, pic_width_in_codec_data, pic_height_in_codec_data);

    if (parent->picture_width == 0 || parent->picture_height == 0)
    {
        // Update picture resolution only if it is not set. The derived picture res from mbs may not be accurate.
        parent->picture_width  = pic_width_in_codec_data;
        parent->picture_height = pic_height_in_codec_data;
    }

    ret = mix_videofmt_h264_initialize_va(mix, data);
    if (ret != MIX_RESULT_SUCCESS)
    {
        LOG_E( "Error initializing va. \n");
        goto cleanup;
    }

    cleanup:
    if (ret != MIX_RESULT_SUCCESS) {
        pret = vbp_close(parent->parser_handle);
        parent->parser_handle = NULL;
        parent->initialized = FALSE;

    } else {
        parent->initialized = TRUE;
    }

    if (header != NULL)
    {
        if (header->data != NULL)
            g_free(header->data);
        g_free(header);
        header = NULL;
    }


    LOG_V( "Unlocking\n");
    g_mutex_unlock(parent->objectlock);


    return ret;
}

MIX_RESULT mix_videofmt_h264_decode(
    MixVideoFormat *mix,
    MixBuffer * bufin[],
    gint bufincnt,
    MixVideoDecodeParams * decode_params) {

    int i = 0;
    MixVideoFormat *parent = NULL;
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    guint64 ts = 0;
    gboolean discontinuity = FALSE;

    LOG_V( "Begin\n");

    if (mix == NULL || bufin == NULL || decode_params == NULL || bufincnt == 0)
    {
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

    if (!MIX_IS_VIDEOFORMAT_H264(mix))
        return MIX_RESULT_INVALID_PARAM;

    parent = MIX_VIDEOFORMAT(mix);

    ret = mix_videodecodeparams_get_timestamp(decode_params, &ts);
    if (ret != MIX_RESULT_SUCCESS)
    {
        // never happen
        return MIX_RESULT_FAIL;
    }

    ret = mix_videodecodeparams_get_discontinuity(decode_params, &discontinuity);
    if (ret != MIX_RESULT_SUCCESS)
    {
        // never happen
        return MIX_RESULT_FAIL;
    }

    //From now on, we exit this function through cleanup:

    LOG_V( "Locking\n");
    g_mutex_lock(parent->objectlock);

    LOG_V( "Starting current frame %d, timestamp %"G_GINT64_FORMAT"\n", mix_video_h264_counter++, ts);

    for (i = 0; i < bufincnt; i++)
    {
		LOG_V( "Decoding a buf %x, size %d\n", (guint)bufin[i]->data, bufin[i]->size);

        // decode a buffer at a time
        ret = mix_videofmt_h264_decode_a_buffer(
            mix,
            bufin[i],
            ts,
            discontinuity);

        if (ret != MIX_RESULT_SUCCESS)
        {
            LOG_E("mix_videofmt_h264_decode_a_buffer failed.\n");
            goto cleanup;
        }
    }


cleanup:

    LOG_V( "Unlocking\n");
    g_mutex_unlock(parent->objectlock);

    LOG_V( "End\n");

    return ret;
}

MIX_RESULT mix_videofmt_h264_flush(MixVideoFormat *mix) {

    MIX_RESULT ret = MIX_RESULT_SUCCESS;

	LOG_V( "Begin\n");

	if (mix == NULL)
	{
		LOG_E( "Null pointer passed in\n");
		return MIX_RESULT_NULL_PTR;
	}

    uint32 pret = 0;

	/* Chainup parent method.
		We are not chaining up to parent method for now.
	 */

#if 0
	if (parent_class->flush) {
		return parent_class->flush(mix, msg);
	}
#endif

	MixVideoFormat_H264 *self = MIX_VIDEOFORMAT_H264(mix);

    g_mutex_lock(mix->objectlock);

    // drop any decode-pending picture, and ignore return value
    mix_videofmt_h264_decode_end(mix, TRUE);

	//Clear parse_in_progress flag and current timestamp
    mix->parse_in_progress = FALSE;
	mix->discontinuity_frame_in_progress = FALSE;
	mix->current_timestamp = (guint64)-1;

	//Clear the DPB surface table
	g_hash_table_remove_all(self->dpb_surface_table);

	//Call parser flush
	pret = vbp_flush(mix->parser_handle);
	if (pret != VBP_OK)
		ret = MIX_RESULT_FAIL;

    g_mutex_unlock(mix->objectlock);

	LOG_V( "End\n");

	return ret;
}

MIX_RESULT mix_videofmt_h264_eos(MixVideoFormat *mix) {

	MIX_RESULT ret = MIX_RESULT_SUCCESS;

    LOG_V( "Begin\n");

	if (mix == NULL)
	{
		LOG_E( "Null pointer passed in\n");
		return MIX_RESULT_NULL_PTR;
	}

	/* Chainup parent method.
		We are not chaining up to parent method for now.
	 */

#if 0
	if (parent_class->eos) {
		return parent_class->eos(mix, msg);
	}
#endif

    g_mutex_lock(mix->objectlock);

    // finished decoding the pending frame
    mix_videofmt_h264_decode_end(mix, FALSE);

    g_mutex_unlock(mix->objectlock);

	//Call Frame Manager with _eos()
	ret = mix_framemanager_eos(mix->framemgr);

	LOG_V( "End\n");

	return ret;


}

MIX_RESULT mix_videofmt_h264_deinitialize(MixVideoFormat *mix) {

//Note this method is not called; may remove in future

	LOG_V( "Begin\n");

	if (mix == NULL)
	{
		LOG_E( "Null pointer passed in\n");
		return MIX_RESULT_NULL_PTR;
	}

	/* Chainup parent method.
	 */

	if (parent_class->deinitialize) {
		return parent_class->deinitialize(mix);
	}

    //Most stuff is cleaned up in parent_class->finalize() and in _finalize

    LOG_V( "End\n");

	return MIX_RESULT_SUCCESS;
}


#define HACK_DPB
#ifdef HACK_DPB
static inline MIX_RESULT mix_videofmt_h264_hack_dpb(MixVideoFormat *mix,
					vbp_picture_data_h264* pic_data
					)
{

	gboolean found = FALSE;
	guint tflags = 0;
	VAPictureParameterBufferH264 *pic_params = pic_data->pic_parms;
	VAPictureH264 *pRefList = NULL;
	int i = 0, j = 0, k = 0, list = 0;

	MixVideoFormat_H264 *self = MIX_VIDEOFORMAT_H264(mix);

	//Set the surface ID for everything in the parser DPB to INVALID
	for (i = 0; i < 16; i++)
	{
		pic_params->ReferenceFrames[i].picture_id = VA_INVALID_SURFACE;
		pic_params->ReferenceFrames[i].frame_idx = -1;
		pic_params->ReferenceFrames[i].TopFieldOrderCnt = -1;
		pic_params->ReferenceFrames[i].BottomFieldOrderCnt = -1;
		pic_params->ReferenceFrames[i].flags = VA_PICTURE_H264_INVALID;  //assuming we don't need to OR with existing flags
	}

	pic_params->num_ref_frames = 0;

	for (i = 0; i < pic_data->num_slices; i++)
	{

		//Copy from the List0 and List1 surface IDs
		pRefList = pic_data->slc_data[i].slc_parms.RefPicList0;
		for (list = 0; list < 2; list++)
		{
			for (j = 0; j < 32; j++)
			{
				if (pRefList[j].flags & VA_PICTURE_H264_INVALID)
				{
					break;  //no more valid reference frames in this list
				}
				found = FALSE;
				for (k = 0; k < pic_params->num_ref_frames; k++)
				{
					if (pic_params->ReferenceFrames[k].TopFieldOrderCnt == pRefList[j].TopFieldOrderCnt)
					{
						///check for complementary field
						tflags = pic_params->ReferenceFrames[k].flags | pRefList[j].flags;
						//If both TOP and BOTTOM are set, we'll clear those flags
						if ((tflags & VA_PICTURE_H264_TOP_FIELD) &&
							(tflags & VA_PICTURE_H264_TOP_FIELD))
							pic_params->ReferenceFrames[k].flags = VA_PICTURE_H264_SHORT_TERM_REFERENCE;
						found = TRUE;  //already in the DPB; will not add this one
						break;
					}
				}
				if (!found)
				{
					guint poc = mix_videofmt_h264_get_poc(&(pRefList[j]));
					gpointer video_frame = g_hash_table_lookup(self->dpb_surface_table, (gpointer)poc);

#ifdef DECODER_ROBUSTNESS
					if (!video_frame)
					{
						if (!self->last_decoded_frame)
						{
							//No saved reference frame, can't recover this one
							return MIX_RESULT_DROPFRAME;
						}

						pic_params->ReferenceFrames[pic_params->num_ref_frames].picture_id =
							((MixVideoFrame *)self->last_decoded_frame)->frame_id;
						LOG_V( "Reference frame not found, substituting %d\n", pic_params->ReferenceFrames[pic_params->num_ref_frames].picture_id);

					}
					else
					{
						pic_params->ReferenceFrames[pic_params->num_ref_frames].picture_id =
							((MixVideoFrame *)video_frame)->frame_id;
					}
#else
					if (!video_frame) return MIX_RESULT_DROPFRAME; //return non-fatal error

					pic_params->ReferenceFrames[pic_params->num_ref_frames].picture_id =
						((MixVideoFrame *)video_frame)->frame_id;
#endif

        			LOG_V( "Inserting frame id %d into DPB\n", pic_params->ReferenceFrames[pic_params->num_ref_frames].picture_id);

					pic_params->ReferenceFrames[pic_params->num_ref_frames].flags =
						pRefList[j].flags;
					pic_params->ReferenceFrames[pic_params->num_ref_frames].frame_idx =
						pRefList[j].frame_idx;
					pic_params->ReferenceFrames[pic_params->num_ref_frames].TopFieldOrderCnt =
						pRefList[j].TopFieldOrderCnt;
					pic_params->ReferenceFrames[pic_params->num_ref_frames++].BottomFieldOrderCnt =
						pRefList[j].BottomFieldOrderCnt;
				}

			}
		    pRefList = pic_data->slc_data[i].slc_parms.RefPicList1;
		}

	}
	return MIX_RESULT_SUCCESS;
}
#endif



MIX_RESULT mix_videofmt_h264_handle_ref_frames(MixVideoFormat *mix,
					VAPictureParameterBufferH264* pic_params,
					MixVideoFrame * current_frame
					) {

	guint poc = 0;

	LOG_V( "Begin\n");

    if (mix == NULL || current_frame == NULL || pic_params == NULL)
	{
		LOG_E( "Null pointer passed in\n");
		return MIX_RESULT_NULL_PTR;
	}


	LOG_V( "Pic_params has flags %d, topfieldcnt %d, bottomfieldcnt %d.  Surface ID is %d\n", pic_params->CurrPic.flags, pic_params->CurrPic.TopFieldOrderCnt, pic_params->CurrPic.BottomFieldOrderCnt, (gint) current_frame->frame_id);

#ifdef MIX_LOG_ENABLE
	if (pic_params->CurrPic.flags & VA_PICTURE_H264_INVALID)
		LOG_V( "Flags show VA_PICTURE_H264_INVALID\n");

	if (pic_params->CurrPic.flags & VA_PICTURE_H264_TOP_FIELD)
		LOG_V( "Flags show VA_PICTURE_H264_TOP_FIELD\n");

	if (pic_params->CurrPic.flags & VA_PICTURE_H264_BOTTOM_FIELD)
		LOG_V( "Flags show VA_PICTURE_H264_BOTTOM_FIELD\n");

	if (pic_params->CurrPic.flags & VA_PICTURE_H264_SHORT_TERM_REFERENCE)
		LOG_V( "Flags show VA_PICTURE_H264_SHORT_TERM_REFERENCE\n");

	if (pic_params->CurrPic.flags & VA_PICTURE_H264_LONG_TERM_REFERENCE)
		LOG_V( "Flags show VA_PICTURE_H264_LONG_TERM_REFERENCE\n");
#endif

    MixVideoFormat_H264 *self = MIX_VIDEOFORMAT_H264(mix);


	//First we need to check the parser DBP against our DPB table
	//So for each item in our DBP table, we look to see if it is in the parser DPB
	//If it is not, it gets unrefed and removed
#ifdef MIX_LOG_ENABLE
	guint num_removed =
#endif
	g_hash_table_foreach_remove(self->dpb_surface_table, mix_videofmt_h264_check_in_DPB, pic_params);

    LOG_V( "%d entries removed from DPB surface table at this frame\n", num_removed);


	MixVideoFrame *mvf = NULL;
	gboolean found = FALSE;
	//Set the surface ID for everything in the parser DPB
	int i = 0;
	for (; i < 16; i++)
	{
		if (!(pic_params->ReferenceFrames[i].flags & VA_PICTURE_H264_INVALID))
		{

			poc = mix_videofmt_h264_get_poc(&(pic_params->ReferenceFrames[i]));
		    LOG_V( "Looking up poc %d in dpb table\n", poc);
			found = g_hash_table_lookup_extended(self->dpb_surface_table, (gpointer)poc, NULL, (gpointer)&mvf);

			if (found)
			{
				pic_params->ReferenceFrames[i].picture_id = mvf->frame_id;
		        LOG_V( "Looked up poc %d in dpb table found frame ID %d\n", poc, (gint)mvf->frame_id);
			} else {
		        LOG_V( "Looking up poc %d in dpb table did not find value\n", poc);
			}
		    LOG_V( "For poc %d, set surface id for DPB index %d to %d\n", poc, i, (gint)pic_params->ReferenceFrames[i].picture_id);
		}

	}


	//Set picture_id for current picture
	pic_params->CurrPic.picture_id = current_frame->frame_id;

	//Check to see if current frame is a reference frame
	if ((pic_params->CurrPic.flags & VA_PICTURE_H264_SHORT_TERM_REFERENCE) ||
        (pic_params->CurrPic.flags & VA_PICTURE_H264_LONG_TERM_REFERENCE))
	{
		//Get current frame's POC
		poc = mix_videofmt_h264_get_poc(&(pic_params->CurrPic));

		//Increment the reference count for this frame
		mix_videoframe_ref(current_frame);

		LOG_V( "Inserting poc %d, surfaceID %d\n", poc, (gint)current_frame->frame_id);
		//Add this frame to the DPB surface table
		g_hash_table_insert(self->dpb_surface_table, (gpointer)poc, current_frame);
	}

	LOG_V( "End\n");

	return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videofmt_h264_cleanup_ref_frame(MixVideoFormat *mix,
					VAPictureParameterBufferH264* pic_params,
					MixVideoFrame * current_frame
					) {

	guint poc = 0;

	LOG_V( "Begin\n");

        if (mix == NULL || current_frame == NULL || pic_params == NULL)
	{
		LOG_E( "Null pointer passed in\n");
		return MIX_RESULT_NULL_PTR;
	}


	LOG_V( "Pic_params has flags %d, topfieldcnt %d, bottomfieldcnt %d.  Surface ID is %d\n", pic_params->CurrPic.flags, pic_params->CurrPic.TopFieldOrderCnt, pic_params->CurrPic.BottomFieldOrderCnt, (gint) current_frame->frame_id);

        MixVideoFormat_H264 *self = MIX_VIDEOFORMAT_H264(mix);

	//Check to see if current frame is a reference frame
	if ((pic_params->CurrPic.flags & VA_PICTURE_H264_SHORT_TERM_REFERENCE) || (pic_params->CurrPic.flags & VA_PICTURE_H264_LONG_TERM_REFERENCE))
	{
		//Get current frame's POC
		poc = mix_videofmt_h264_get_poc(&(pic_params->CurrPic));

		//We don't need to decrement the ref count for the video frame here; it's done elsewhere

		LOG_V( "Removing poc %d, surfaceID %d\n", poc, (gint)current_frame->frame_id);
		//Remove this frame from the DPB surface table
		g_hash_table_remove(self->dpb_surface_table, (gpointer)poc);
	}


	LOG_V( "End\n");

	return MIX_RESULT_SUCCESS;
}

guint mix_videofmt_h264_get_poc(VAPictureH264 *pic)
{
    if (pic == NULL)
        return 0;

	if (pic->flags & VA_PICTURE_H264_BOTTOM_FIELD)
		return pic->BottomFieldOrderCnt;

	if (pic->flags & VA_PICTURE_H264_TOP_FIELD)
		return pic->TopFieldOrderCnt;

	return pic->TopFieldOrderCnt;

}


gboolean mix_videofmt_h264_check_in_DPB(gpointer key, gpointer value, gpointer user_data)
{
	gboolean ret = TRUE;

    if ((value == NULL) || (user_data == NULL))  //Note that 0 is valid value for key
        return FALSE;

	VAPictureH264* vaPic = NULL;
	int i = 0;
	for (; i < 16; i++)
	{
		vaPic = &(((VAPictureParameterBufferH264*)user_data)->ReferenceFrames[i]);
		if (vaPic->flags & VA_PICTURE_H264_INVALID)
			continue;

		if ((guint)key == vaPic->TopFieldOrderCnt ||
			(guint)key == vaPic->BottomFieldOrderCnt)
		{
			ret = FALSE;
			break;
		}
	}

	return ret;
}


void mix_videofmt_h264_destroy_DPB_key(gpointer data)
{
    //TODO remove this method and don't register it with the hash table foreach call; it is no longer needed
	LOG_V( "Begin, poc of %d\n", (guint)data);
	LOG_V( "End\n");

	return;
}

void mix_videofmt_h264_destroy_DPB_value(gpointer data)
{
	LOG_V( "Begin\n");
    if (data != NULL)
    {
    	mix_videoframe_unref((MixVideoFrame *)data);
    }
	LOG_V( "End\n");
	return;
}


MIX_RESULT mix_videofmt_h264_release_input_buffers(MixVideoFormat *mix,
					guint64 timestamp
					) {

    // no longer used, need to be removed.
	return MIX_RESULT_SUCCESS;
}


