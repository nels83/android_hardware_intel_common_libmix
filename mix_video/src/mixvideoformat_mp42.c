/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */
#include <glib.h>
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


/* The parent class. The pointer will be saved
 * in this class's initialization. The pointer
 * can be used for chaining method call if needed.
 */
static MixVideoFormatClass *parent_class = NULL;

static void mix_videoformat_mp42_finalize(GObject * obj);

/*
 * Please note that the type we pass to G_DEFINE_TYPE is MIX_TYPE_VIDEOFORMAT
 */
G_DEFINE_TYPE( MixVideoFormat_MP42, mix_videoformat_mp42, MIX_TYPE_VIDEOFORMAT);

static void mix_videoformat_mp42_init(MixVideoFormat_MP42 * self) {
	MixVideoFormat *parent = MIX_VIDEOFORMAT(self);

	self->reference_frames[0] = NULL;
	self->reference_frames[1] = NULL;

	self->last_frame = NULL;
	self->last_vop_coding_type = -1;
	self->last_vop_time_increment = 0;
	self->next_nvop_for_PB_frame = FALSE;
	self->iq_matrix_buf_sent = FALSE;

	/* NOTE: we don't need to do this here.
	 * This just demostrates how to access
	 * member varibles beloned to parent
	 */
	parent->initialized = FALSE;
}

static void mix_videoformat_mp42_class_init(MixVideoFormat_MP42Class * klass) {

	/* root class */
	GObjectClass *gobject_class = (GObjectClass *) klass;

	/* direct parent class */
	MixVideoFormatClass *video_format_class = MIX_VIDEOFORMAT_CLASS(klass);

	/* parent class for later use */
	parent_class = g_type_class_peek_parent(klass);

	/* setup finializer */
	gobject_class->finalize = mix_videoformat_mp42_finalize;

	/* setup vmethods with base implementation */
	video_format_class->getcaps = mix_videofmt_mp42_getcaps;
	video_format_class->initialize = mix_videofmt_mp42_initialize;
	video_format_class->decode = mix_videofmt_mp42_decode;
	video_format_class->flush = mix_videofmt_mp42_flush;
	video_format_class->eos = mix_videofmt_mp42_eos;
	video_format_class->deinitialize = mix_videofmt_mp42_deinitialize;
}

MixVideoFormat_MP42 *mix_videoformat_mp42_new(void) {
	MixVideoFormat_MP42 *ret = g_object_new(MIX_TYPE_VIDEOFORMAT_MP42, NULL);

	return ret;
}

void mix_videoformat_mp42_finalize(GObject * obj) {
	/* clean up here. */

	/*	MixVideoFormat_MP42 *mix = MIX_VIDEOFORMAT_MP42(obj); */
	GObjectClass *root_class = (GObjectClass *) parent_class;
	MixVideoFormat *parent = NULL;
	gint32 vbp_ret = VBP_OK;
	MixVideoFormat_MP42 *self = NULL;
    gint idx = 0;

	LOG_V("Begin\n");

	if (obj == NULL) {
		LOG_E("obj is NULL\n");
		return;
	}

	if (!MIX_IS_VIDEOFORMAT_MP42(obj)) {
		LOG_E("obj is not mixvideoformat_mp42\n");
		return;
	}

	self = MIX_VIDEOFORMAT_MP42(obj);
	parent = MIX_VIDEOFORMAT(self);

	//surfacepool is deallocated by parent
	//inputbufqueue is deallocated by parent
	//parent calls vaDestroyConfig, vaDestroyContext and vaDestroySurfaces

	g_mutex_lock(parent->objectlock);

	/* unref reference frames */
	for (idx = 0; idx < 2; idx++) {
		if (self->reference_frames[idx] != NULL) {
			mix_videoframe_unref(self->reference_frames[idx]);
			self->reference_frames[idx] = NULL;
		}
	}
    if (self->last_frame)
    {
        mix_videoframe_unref(self->last_frame);
        self->last_frame = NULL;
    }
    self->next_nvop_for_PB_frame = FALSE;
    self->iq_matrix_buf_sent = FALSE;

	/* Reset state */
	parent->initialized = TRUE;
	parent->end_picture_pending = FALSE;
	parent->discontinuity_frame_in_progress = FALSE;
	parent->current_timestamp = (guint64)-1;

	/* Close the parser */
	if (parent->parser_handle)
	{
    	vbp_ret = vbp_close(parent->parser_handle);
	    parent->parser_handle = NULL;
    }	    

	g_mutex_unlock(parent->objectlock);

	/* Chain up parent */
	if (root_class->finalize) {
		root_class->finalize(obj);
	}

	LOG_V("End\n");
}

MixVideoFormat_MP42 *
mix_videoformat_mp42_ref(MixVideoFormat_MP42 * mix) {
	return (MixVideoFormat_MP42 *) g_object_ref(G_OBJECT(mix));
}

/*  MP42 vmethods implementation */
MIX_RESULT mix_videofmt_mp42_getcaps(MixVideoFormat *mix, GString *msg) {

//This method is reserved for future use

	LOG_V("Begin\n");
	if (parent_class->getcaps) {
		return parent_class->getcaps(mix, msg);
	}

	LOG_V("End\n");
	return MIX_RESULT_NOTIMPL;
}


MIX_RESULT mix_videofmt_mp42_update_config_params(
    MixVideoFormat *mix,
    vbp_data_mp42 *data)
{
    MixVideoFormat *parent = MIX_VIDEOFORMAT(mix);
    //MixVideoFormat_MP42 *self = MIX_VIDEOFORMAT_MP42(mix);  

    if (parent->picture_width == 0 || 
        parent->picture_height == 0 ||
       parent->picture_width < data->codec_data.video_object_layer_width ||
       parent->picture_height < data->codec_data.video_object_layer_height)
    {
        parent->picture_width = data->codec_data.video_object_layer_width;
        parent->picture_height = data->codec_data.video_object_layer_height;

        mix_videoconfigparamsdec_set_picture_res(
            mix->config_params, 
            parent->picture_width, 
            parent->picture_height);        
    }
    

    // video_range has default value of 0. Y ranges from 16 to 235.
    mix_videoconfigparamsdec_set_video_range(mix->config_params, data->codec_data.video_range);
    
    uint8  color_matrix;
    
    switch (data->codec_data.matrix_coefficients)
    {
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
    mix_videoconfigparamsdec_set_color_matrix(mix->config_params, color_matrix); 

    mix_videoconfigparamsdec_set_pixel_aspect_ratio(
        mix->config_params,
        data->codec_data.par_width,
        data->codec_data.par_height);
    
    return MIX_RESULT_SUCCESS;
}


MIX_RESULT mix_videofmt_mp42_initialize_va(
    MixVideoFormat *mix,
    vbp_data_mp42 *data)
{
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    VAStatus vret = VA_STATUS_SUCCESS;
    VAConfigAttrib attrib;
    VAProfile va_profile;      
    MixVideoFormat *parent = MIX_VIDEOFORMAT(mix);
    //MixVideoFormat_MP42 *self = MIX_VIDEOFORMAT_MP42(mix);  

    LOG_V( "Begin\n");

    if (parent->va_initialized)
    {
        LOG_W("va already initialized.\n");
        return MIX_RESULT_SUCCESS;
    }
    
    //We are requesting RT attributes
    attrib.type = VAConfigAttribRTFormat;
    attrib.value = VA_RT_FORMAT_YUV420;

    //Initialize and save the VA config ID
	if ((data->codec_data.profile_and_level_indication & 0xF8) == 0xF0) 
	{
		va_profile = VAProfileMPEG4AdvancedSimple;
	} 
	else 
	{
		va_profile = VAProfileMPEG4Simple;
	}
		
    vret = vaCreateConfig(
        parent->va_display, 
        va_profile, 
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

    // add 1 more surface for packed frame (PB frame), and another one
    // for partial frame handling
	parent->va_num_surfaces = parent->extra_surfaces + 4 + 1 + 1;
	//if (parent->va_num_surfaces > MIX_VIDEO_MP42_SURFACE_NUM) 
	//	parent->va_num_surfaces = MIX_VIDEO_MP42_SURFACE_NUM;
                
    parent->va_surfaces = g_malloc(sizeof(VASurfaceID)*parent->va_num_surfaces);
    if (parent->va_surfaces == NULL)
    {
        ret = MIX_RESULT_FAIL;
        LOG_E( "parent->va_surfaces == NULL. \n");
        goto cleanup;
    }
    
    vret = vaCreateSurfaces(
        parent->va_display, 
        parent->picture_width,
        parent->picture_height,
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
    
    //Initialize and save the VA context ID
    //Note: VA_PROGRESSIVE libva flag is only relevant to MPEG2
    vret = vaCreateContext(
        parent->va_display, 
        parent->va_config,
        parent->picture_width, 
        parent->picture_height,
        0, 
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

MIX_RESULT mix_videofmt_mp42_decode_a_slice(
    MixVideoFormat *mix,
    vbp_data_mp42* data,
    vbp_picture_data_mp42* pic_data)
{  
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    VAStatus vret = VA_STATUS_SUCCESS;
    VADisplay vadisplay = NULL;
    VAContextID vacontext;
    guint buffer_id_cnt = 0;
    gint frame_type = -1;
    // maximum 4 buffers to render a slice: picture parameter, IQMatrix, slice parameter, slice data
    VABufferID buffer_ids[4];
    MixVideoFormat_MP42 *self = MIX_VIDEOFORMAT_MP42(mix);    
    VAPictureParameterBufferMPEG4* pic_params = &(pic_data->picture_param);
    vbp_slice_data_mp42* slice_data = &(pic_data->slice_data);
    VASliceParameterBufferMPEG4* slice_params = &(slice_data->slice_param);
    
    LOG_V( "Begin\n");

    vadisplay = mix->va_display;
    vacontext = mix->va_context;

    if (!mix->end_picture_pending)
    {
        LOG_E("picture decoder is not started!\n");
        ret = MIX_RESULT_FAIL;
        goto cleanup;
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
				= self->reference_frames[0]->frame_id;
		pic_params-> backward_reference_picture = VA_INVALID_SURFACE;
		break;
		
	case MP4_VOP_TYPE_B:
		pic_params->vop_fields.bits.backward_reference_vop_coding_type
				= self->last_vop_coding_type;
		pic_params->forward_reference_picture
				= self->reference_frames[1]->frame_id;				
		pic_params->backward_reference_picture
				= self->reference_frames[0]->frame_id;
		break;
		
	case MP4_VOP_TYPE_S:
		pic_params-> forward_reference_picture
				= self->reference_frames[0]->frame_id;
		pic_params-> backward_reference_picture = VA_INVALID_SURFACE;
		break;

	default:
		LOG_W("default, Will never reach here\n");
		ret = MIX_RESULT_FAIL;
		goto cleanup;
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

    if (vret != VA_STATUS_SUCCESS)
    {
        ret = MIX_RESULT_FAIL;
        LOG_E( "Video driver returned error from vaCreateBuffer\n");
        goto cleanup;
    }

    buffer_id_cnt++;
            
    if (pic_params->vol_fields.bits.quant_type && self->iq_matrix_buf_sent == FALSE) 
    {
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
    
        if (vret != VA_STATUS_SUCCESS)
        {
            ret = MIX_RESULT_FAIL;
            LOG_E( "Video driver returned error from vaCreateBuffer\n");
            goto cleanup;
        }
        self->iq_matrix_buf_sent = TRUE;
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


MIX_RESULT mix_videofmt_mp42_decode_end(
    MixVideoFormat *mix, 
    gboolean drop_picture)
{
    MIX_RESULT ret = MIX_RESULT_SUCCESS;    
    VAStatus vret = VA_STATUS_SUCCESS;
    MixVideoFormat* parent = MIX_VIDEOFORMAT(mix);
    //MixVideoFormat_MP42 *self = MIX_VIDEOFORMAT_MP42(mix);  
    
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
    vret = vaEndPicture(parent->va_display, parent->va_context);
    
    if (vret != VA_STATUS_SUCCESS)
    {
        ret = MIX_RESULT_DROPFRAME;
        LOG_E( "Video driver returned error from vaEndPicture\n");
        goto cleanup;
    }
    
#if 0	/* we don't call vaSyncSurface here, the call is moved to mix_video_render() */
    
    LOG_V( "Calling vaSyncSurface\n");

    //Decode the picture
    vret = vaSyncSurface(vadisplay, surface);

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
    }
    
cleanup:
   if (parent->video_frame)
   {
        /* this always indicates an error */        
        mix_videoframe_unref(parent->video_frame);
        parent->video_frame = NULL;
   }
   parent->end_picture_pending = FALSE;
   return ret;

}


MIX_RESULT mix_videofmt_mp42_decode_continue(
    MixVideoFormat *mix, 
    vbp_data_mp42 *data)
{
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    VAStatus vret = VA_STATUS_SUCCESS;
    int i;   
    gint frame_type = -1;
    vbp_picture_data_mp42* pic_data = NULL;
    VAPictureParameterBufferMPEG4* pic_params = NULL;
    MixVideoFormat_MP42 *self = MIX_VIDEOFORMAT_MP42(mix);

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
	for (i = 0; i < data->number_picture_data; i++, pic_data = pic_data->next_picture_data)
	{
	    pic_params = &(pic_data->picture_param);
	    frame_type = pic_params->vop_fields.bits.vop_coding_type;
    	if (frame_type == MP4_VOP_TYPE_S && pic_params->no_of_sprite_warping_points > 1)
    	{
    	    // hardware only support up to one warping point (stationary or translation)
    	    LOG_E("sprite with %d warping points is not supported by HW.\n",
    	        pic_params->no_of_sprite_warping_points);    	        
    	    return MIX_RESULT_DROPFRAME;
    	}
        
    	if (pic_data->vop_coded == 0)
    	{
    	    // this should never happen
    	    LOG_E("VOP is not coded.\n");
    	    return MIX_RESULT_DROPFRAME;
    	}
    	
        if (pic_data->new_picture_flag == 1 || mix->end_picture_pending == FALSE)
        {
            if (pic_data->new_picture_flag == 0)
            {
                LOG_W("First slice of picture is lost!\n");
            }
            
            gulong surface = 0;                    
            if (mix->end_picture_pending)
            {
                // this indicates the start of a new frame in the packed frame
                LOG_V("packed frame is found.\n");

                // Update timestamp for packed frame as timestamp is for the B frame!
                if (mix->video_frame && pic_params->vop_time_increment_resolution)
                {
                    guint64 ts, ts_inc;
                    mix_videoframe_get_timestamp(mix->video_frame, &ts);
                    ts_inc= self->last_vop_time_increment - pic_data->vop_time_increment + 
                        pic_params->vop_time_increment_resolution;
                    ts_inc = ts_inc % pic_params->vop_time_increment_resolution;
                    LOG_V("timestamp is incremented by %d at %d resolution.\n",
                        ts_inc, pic_params->vop_time_increment_resolution);
                    // convert to nano-second
                    ts_inc = ts_inc * 1e9 / pic_params->vop_time_increment_resolution;
                    LOG_V("timestamp of P frame in packed frame is updated from %"G_GINT64_FORMAT"  to %"G_GUINT64_FORMAT".\n",
                        ts, ts + ts_inc);

                    ts += ts_inc; 
                    mix_videoframe_set_timestamp(mix->video_frame, ts);
                }
                
                mix_videofmt_mp42_decode_end(mix, FALSE);
                self->next_nvop_for_PB_frame = TRUE;
            }     
            if (self->next_nvop_for_PB_frame == TRUE && frame_type != MP4_VOP_TYPE_B)
            {
                LOG_E("The second frame in the packed frame is not B frame.\n");
                self->next_nvop_for_PB_frame = FALSE;
                return MIX_RESULT_DROPFRAME;
            }
            
        	//Get a frame from the surface pool
        	ret = mix_surfacepool_get(mix->surfacepool, &(mix->video_frame));
        	if (ret != MIX_RESULT_SUCCESS)
        	{
        		LOG_E( "Error getting frame from surfacepool\n");
        		return MIX_RESULT_FAIL;
        	}
	
        	/* the following calls will always succeed */

            // set frame type
            if (frame_type == MP4_VOP_TYPE_S)
            {
                // sprite is treated as P frame in the display order
                mix_videoframe_set_frame_type(mix->video_frame, MP4_VOP_TYPE_P);
            }
            else
            {
                mix_videoframe_set_frame_type(mix->video_frame, frame_type);
            }
            

            // set frame structure
            if (pic_data->picture_param.vol_fields.bits.interlaced)
            {
                // only MPEG-4 studio profile can have field coding. All other profiles 
                // use frame coding only, i.e, no field VOP.  (see vop_structure in MP4 spec)
                mix_videoframe_set_frame_structure(
                    mix->video_frame, 
                    VA_BOTTOM_FIELD | VA_TOP_FIELD);

                LOG_W("Interlaced content, set frame structure to 3 (TOP | BOTTOM field) !\n");                                    
            }        
            else
            {   
                mix_videoframe_set_frame_structure(mix->video_frame, VA_FRAME_PICTURE);
            }

            //Set the discontinuity flag
            mix_videoframe_set_discontinuity(
                mix->video_frame, 
                mix->discontinuity_frame_in_progress);

            //Set the timestamp
            mix_videoframe_set_timestamp(mix->video_frame, mix->current_timestamp);	
       
            //Get our surface ID from the frame object
            ret = mix_videoframe_get_frame_id(mix->video_frame, &surface);    
            if (ret != MIX_RESULT_SUCCESS)
            {
                LOG_E( "Error getting surface ID from frame object\n");
                goto cleanup;
            }    

            /* If I or P frame, update the reference array */
        	if ((frame_type == MP4_VOP_TYPE_I) || (frame_type == MP4_VOP_TYPE_P)) 
        	{
        		LOG_V("Updating forward/backward references for libva\n");

        		self->last_vop_coding_type = frame_type;
        		self->last_vop_time_increment = pic_data->vop_time_increment;
        		mix_videofmt_mp42_handle_ref_frames(mix, frame_type, mix->video_frame);
                if (self->last_frame != NULL) 
                {
                    mix_videoframe_unref(self->last_frame);
                }
                self->last_frame = mix->video_frame;
                mix_videoframe_ref(self->last_frame);
        	}        
    	
            //Now we can begin the picture
            vret = vaBeginPicture(mix->va_display, mix->va_context, surface);
            if (vret != VA_STATUS_SUCCESS)
            {
                ret = MIX_RESULT_FAIL;
                LOG_E( "Video driver returned error from vaBeginPicture\n");
                goto cleanup;
            }            

            // vaBeginPicture needs a matching vaEndPicture 
            mix->end_picture_pending = TRUE;
            self->iq_matrix_buf_sent = FALSE;
        }
        
        
        ret = mix_videofmt_mp42_decode_a_slice(mix, data, pic_data);
		if (ret != 	MIX_RESULT_SUCCESS)
    	{
	    	LOG_E( "mix_videofmt_mp42_decode_a_slice failed, error =  %#X.", ret);
		    goto cleanup;
		}		
	}

cleanup:
    // nothing to cleanup;

    return ret;
}


MIX_RESULT mix_videofmt_mp42_decode_begin(
    MixVideoFormat *mix, 
    vbp_data_mp42* data)
{
	MIX_RESULT ret = MIX_RESULT_SUCCESS;	
	gint frame_type = -1;
    VAPictureParameterBufferMPEG4* pic_params = NULL;
    MixVideoFormat_MP42 *self = MIX_VIDEOFORMAT_MP42(mix);
    vbp_picture_data_mp42 *pic_data = NULL;

    pic_data = data->picture_data;
    pic_params = &(pic_data->picture_param);
    frame_type = pic_params->vop_fields.bits.vop_coding_type;
    
    if (self->next_nvop_for_PB_frame)
    {
        // if we are waiting for n-vop for packed frame, and the new frame is coded, the coding type 
        // of this frame must be B. 
        // for example: {PB} B N P B B P...
        if (pic_data->vop_coded == 1 && frame_type != MP4_VOP_TYPE_B)
        {
            LOG_E("Invalid coding type while waiting for n-vop for packed frame.\n");
            // timestamp of P frame in the queue is not correct
            mix_framemanager_flush(mix->framemgr);
            self->next_nvop_for_PB_frame = FALSE;
        }
    }
    
    if (pic_data->vop_coded == 0)
    {
        if (self->last_frame == NULL)
        {
            LOG_E("The forward reference frame is NULL, couldn't reconstruct skipped frame.\n");
            mix_framemanager_flush(mix->framemgr);
            self->next_nvop_for_PB_frame = FALSE;
            return MIX_RESULT_DROPFRAME;
        }
        
        if (self->next_nvop_for_PB_frame)
        {
            // P frame is already in queue, just need to update time stamp.
            mix_videoframe_set_timestamp(self->last_frame, mix->current_timestamp);
            self->next_nvop_for_PB_frame = FALSE;
        }
        else
        {
            // handle skipped frame
            MixVideoFrame *skip_frame = NULL;
            gulong frame_id = VA_INVALID_SURFACE;
	
    		skip_frame = mix_videoframe_new();
    		ret = mix_videoframe_set_is_skipped(skip_frame, TRUE);
    		ret = mix_videoframe_get_frame_id(self->last_frame, &frame_id);
    		ret = mix_videoframe_set_frame_id(skip_frame, frame_id);
    		ret = mix_videoframe_set_frame_type(skip_frame, MP4_VOP_TYPE_P);
    		ret = mix_videoframe_set_real_frame(skip_frame, self->last_frame);
    		// add a reference as skip_frame holds the last_frame.
    		mix_videoframe_ref(self->last_frame);
    		ret = mix_videoframe_set_timestamp(skip_frame, mix->current_timestamp);
    		ret = mix_videoframe_set_discontinuity(skip_frame, FALSE);

    		LOG_V("Processing skipped frame %x, frame_id set to %d, ts %"G_GINT64_FORMAT"\n",
    				(guint)skip_frame, (guint)frame_id, mix->current_timestamp);

    		/* Enqueue the skipped frame using frame manager */
    		ret = mix_framemanager_enqueue(mix->framemgr, skip_frame);         
        }

        if (data->number_picture_data > 1)
        {
            LOG_E("Unexpected to have more picture data following a not-coded VOP.\n");
            //picture data is thrown away. No issue if picture data is for N-VOP. if picture data is for
            // coded picture, a frame is lost.
        }
        return MIX_RESULT_SUCCESS;
    }
    else
    {     
        /*
             * Check for B frames after a seek
             * We need to have both reference frames in hand before we can decode a B frame
             * If we don't have both reference frames, we must return MIX_RESULT_DROPFRAME
             */
        if (frame_type == MP4_VOP_TYPE_B) 
        {        
            if (self->reference_frames[0] == NULL ||
                self->reference_frames[1] == NULL) 
            {
                LOG_W("Insufficient reference frames for B frame\n");
                return MIX_RESULT_DROPFRAME;
            }
        }
        else if (frame_type == MP4_VOP_TYPE_P || frame_type == MP4_VOP_TYPE_S)
        {
            if (self->reference_frames[0] == NULL)
            {
                LOG_W("Reference frames for P/S frame is missing\n");
                return MIX_RESULT_DROPFRAME;
            }
        }
        
        // all sanity check passes, continue decoding through mix_videofmt_mp42_decode_continue
        ret = mix_videofmt_mp42_decode_continue(mix, data);
    }  
   
	return ret;

}


MIX_RESULT mix_videofmt_mp42_decode_a_buffer(
    MixVideoFormat *mix, 
    MixBuffer * bufin,
    guint64 ts,
    gboolean discontinuity) 
{
    uint32 pret = 0;
    MixVideoFormat *parent = NULL;
	MIX_RESULT ret = MIX_RESULT_SUCCESS;
	vbp_data_mp42 *data = NULL;

    LOG_V( "Begin\n");

    parent = MIX_VIDEOFORMAT(mix);

	pret = vbp_parse(parent->parser_handle, 
		bufin->data, 
		bufin->size,
		FALSE);
        
	if (pret != VBP_OK)
    {
        ret = MIX_RESULT_DROPFRAME;
        LOG_E( "vbp_parse failed.\n");
        goto cleanup;
    }
    else
    {
        LOG_V("vbp_parse succeeded.\n");
    }

	//query for data
	pret = vbp_query(parent->parser_handle, (void *) &data);

	if ((pret != VBP_OK) || (data == NULL))
	{
	    // never happen!
	    ret = MIX_RESULT_FAIL;
	    LOG_E( "vbp_query failed.\n");
        goto cleanup;
	}
    else
    {
        LOG_V("vbp_query succeeded.\n");
    }

    if (parent->va_initialized == FALSE)
    {    
        mix_videofmt_mp42_update_config_params(parent, data);
        
        LOG_V("try initializing VA...\n");
        ret = mix_videofmt_mp42_initialize_va(parent, data);
        if (ret != MIX_RESULT_SUCCESS)
        {         
            LOG_V("mix_videofmt_mp42_initialize_va failed.\n");
            goto cleanup; 
        }
    }

    // check if any slice is parsed, we may just receive configuration data
    if (data->number_picture_data == 0)
    {
        ret = MIX_RESULT_SUCCESS;
        LOG_V("slice is not available.\n");
        goto cleanup;      
    }

    guint64 last_ts = parent->current_timestamp;    
    parent->current_timestamp = ts;
    parent->discontinuity_frame_in_progress = discontinuity;

    if (last_ts != ts)
    {
		// finish decoding the last frame
		ret = mix_videofmt_mp42_decode_end(parent, FALSE);
        if (ret != MIX_RESULT_SUCCESS)
        {         
            LOG_V("mix_videofmt_mp42_decode_end failed.\n");
            goto cleanup; 
        }

        // start decoding a new frame
		ret = mix_videofmt_mp42_decode_begin(parent, data); 
        if (ret != MIX_RESULT_SUCCESS)
        {         
            LOG_V("mix_videofmt_mp42_decode_begin failed.\n");
            goto cleanup; 
        }        
    }
    else
    {
        ret = mix_videofmt_mp42_decode_continue(parent, data);
        if (ret != MIX_RESULT_SUCCESS)
        {         
            LOG_V("mix_videofmt_mp42_decode_continue failed.\n");
            goto cleanup; 
        }
    }

cleanup:

    LOG_V( "End\n");

	return ret;
}



MIX_RESULT mix_videofmt_mp42_initialize(
    MixVideoFormat *mix, 
	MixVideoConfigParamsDec * config_params,
    MixFrameManager * frame_mgr,
	MixBufferPool * input_buf_pool,
	MixSurfacePool ** surface_pool,
	VADisplay va_display ) 
{
	uint32 pret = 0;
	MIX_RESULT ret = MIX_RESULT_SUCCESS;
	enum _vbp_parser_type ptype = VBP_MPEG4;
	vbp_data_mp42 *data = NULL;
	MixVideoFormat *parent = NULL;
	MixIOVec *header = NULL;
	
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

	if (!MIX_IS_VIDEOFORMAT_MP42(mix))
		return MIX_RESULT_INVALID_PARAM;

	parent = MIX_VIDEOFORMAT(mix);
	//MixVideoFormat_MP42 *self = MIX_VIDEOFORMAT_MP42(mix);

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

    ret = mix_videoconfigparamsdec_get_extra_surface_allocation(config_params,
            &parent->extra_surfaces);

    if (ret != MIX_RESULT_SUCCESS)
    {
    	ret = MIX_RESULT_FAIL;
    	LOG_E( "Cannot get extra surface allocation setting\n");
    	goto cleanup;
    }    

	//Load the bitstream parser
	pret = vbp_open(ptype, &(parent->parser_handle));

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

	pret = vbp_parse(parent->parser_handle, header->data, 
			header->data_size, TRUE);

    if (pret != VBP_OK)
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
	
    mix_videofmt_mp42_update_config_params(mix, data);

    ret = mix_videofmt_mp42_initialize_va(mix, data);
    if (ret != MIX_RESULT_SUCCESS)
    {
        LOG_E( "Error initializing va. \n");
        goto cleanup;
    }


cleanup:
	if (ret != MIX_RESULT_SUCCESS) {
	    if (parent->parser_handle)
	    {
            pret = vbp_close(parent->parser_handle);
            parent->parser_handle = NULL;
    	}
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


MIX_RESULT mix_videofmt_mp42_decode(MixVideoFormat *mix, MixBuffer * bufin[],
                gint bufincnt, MixVideoDecodeParams * decode_params) {

  	int i = 0;
    MixVideoFormat *parent = NULL;
	MIX_RESULT ret = MIX_RESULT_SUCCESS;
	guint64 ts = 0;
	gboolean discontinuity = FALSE;

    LOG_V( "Begin\n");

    if (mix == NULL || bufin == NULL || decode_params == NULL )
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

	if (!MIX_IS_VIDEOFORMAT_MP42(mix))
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

	LOG_I("ts after mix_videodecodeparams_get_timestamp() = %"G_GINT64_FORMAT"\n", ts);

	for (i = 0; i < bufincnt; i++)
	{
	    // decode a buffer at a time
        ret = mix_videofmt_mp42_decode_a_buffer(
            mix, 
            bufin[i],
            ts,
            discontinuity);
        
		if (ret != MIX_RESULT_SUCCESS)
		{
			LOG_E("mix_videofmt_mp42_decode_a_buffer failed.\n");
   			goto cleanup;
		}        
    }


cleanup:

	LOG_V( "Unlocking\n");
 	g_mutex_unlock(parent->objectlock);

    LOG_V( "End\n");

	return ret;
}

MIX_RESULT mix_videofmt_mp42_flush(MixVideoFormat *mix) {

	MIX_RESULT ret = MIX_RESULT_SUCCESS;
	MixVideoFormat_MP42 *self = MIX_VIDEOFORMAT_MP42(mix);

	LOG_V("Begin\n");

	g_mutex_lock(mix->objectlock);

    // drop any decode-pending picture, and ignore return value
     mix_videofmt_mp42_decode_end(mix, TRUE);

	/*
	 * Clear parse_in_progress flag and current timestamp
	 */
	mix->parse_in_progress = FALSE;
	mix->discontinuity_frame_in_progress = FALSE;
	mix->current_timestamp = (guint64)-1;
	self->next_nvop_for_PB_frame = FALSE;

	gint idx = 0;
	for (idx = 0; idx < 2; idx++) {
		if (self->reference_frames[idx] != NULL) {
			mix_videoframe_unref(self->reference_frames[idx]);
			self->reference_frames[idx] = NULL;
		}
	}
	if (self->last_frame)
	{
	    mix_videoframe_unref(self->last_frame);
	    self->last_frame = NULL;
	}
	

	/* Call parser flush */
	vbp_flush(mix->parser_handle);

	g_mutex_unlock(mix->objectlock);

	LOG_V("End\n");

	return ret;
}

MIX_RESULT mix_videofmt_mp42_eos(MixVideoFormat *mix) {

	MIX_RESULT ret = MIX_RESULT_SUCCESS;

	LOG_V("Begin\n");

	if (mix == NULL) {
		return MIX_RESULT_NULL_PTR;
	}

	if (!MIX_IS_VIDEOFORMAT_MP42(mix)) {
		return MIX_RESULT_INVALID_PARAM;
	}

	g_mutex_lock(mix->objectlock);

    mix_videofmt_mp42_decode_end(mix, FALSE);
	
	ret = mix_framemanager_eos(mix->framemgr);

	g_mutex_unlock(mix->objectlock);

	LOG_V("End\n");

	return ret;
}

MIX_RESULT mix_videofmt_mp42_deinitialize(MixVideoFormat *mix) {

	/*
	 * We do the all the cleanup in _finalize
	 */

	MIX_RESULT ret = MIX_RESULT_FAIL;

	LOG_V("Begin\n");

	if (mix == NULL) {
		LOG_V("mix is NULL\n");
		return MIX_RESULT_NULL_PTR;
	}

	if (!MIX_IS_VIDEOFORMAT_MP42(mix)) {
		LOG_V("mix is not mixvideoformat_mp42\n");
		return MIX_RESULT_INVALID_PARAM;
	}

	if (parent_class->deinitialize) {
		ret = parent_class->deinitialize(mix);
	}

	LOG_V("End\n");
	return ret;
}

MIX_RESULT mix_videofmt_mp42_handle_ref_frames(MixVideoFormat *mix,
		enum _picture_type frame_type, MixVideoFrame * current_frame) {

	MixVideoFormat_MP42 *self = MIX_VIDEOFORMAT_MP42(mix);

	LOG_V("Begin\n");

	if (mix == NULL || current_frame == NULL) {
		return MIX_RESULT_NULL_PTR;
	}

	switch (frame_type) {
	case MP4_VOP_TYPE_I:
	case MP4_VOP_TYPE_P:
		LOG_V("Refing reference frame %x\n", (guint) current_frame);

		mix_videoframe_ref(current_frame);

		/* should only happen on first frame */
		if (self->reference_frames[0] == NULL) {
			self->reference_frames[0] = current_frame;
			/* should only happen on second frame */
		} else if (self->reference_frames[1] == NULL) {
			self->reference_frames[1] = current_frame;
		} else {
			LOG_V("Releasing reference frame %x\n",
					(guint) self->reference_frames[0]);
			mix_videoframe_unref(self->reference_frames[0]);
			self->reference_frames[0] = self->reference_frames[1];
			self->reference_frames[1] = current_frame;
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

MIX_RESULT mix_videofmt_mp42_release_input_buffers(MixVideoFormat *mix,
		guint64 timestamp) {

    // not used, to be removed
	return MIX_RESULT_SUCCESS;
}
