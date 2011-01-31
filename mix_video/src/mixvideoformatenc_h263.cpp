/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */
#include <glib.h>
#include <string.h>
#include <stdlib.h>

#include "mixvideolog.h"

#include "mixvideoformatenc_h263.h"
#include "mixvideoconfigparamsenc_h263.h"
#include <va/va_tpi.h>

#undef SHOW_SRC

#ifdef SHOW_SRC
Window win = 0;
#endif /* SHOW_SRC */


/* The parent class. The pointer will be saved
 * in this class's initialization. The pointer
 * can be used for chaining method call if needed.
 */
static MixVideoFormatEncClass *parent_class = NULL;

static void mix_videoformatenc_h263_finalize(GObject * obj);

/*
 * Please note that the type we pass to G_DEFINE_TYPE is MIX_TYPE_VIDEOFORMATENC
 */
G_DEFINE_TYPE (MixVideoFormatEnc_H263, mix_videoformatenc_h263, MIX_TYPE_VIDEOFORMATENC);

static void mix_videoformatenc_h263_init(MixVideoFormatEnc_H263 * self) {
    MixVideoFormatEnc *parent = MIX_VIDEOFORMATENC(self);

    /* member initialization */
    self->encoded_frames = 0;
    self->pic_skipped = FALSE;
    self->is_intra = TRUE;
    self->cur_frame = NULL;
    self->ref_frame = NULL;
    self->rec_frame = NULL;
#ifdef ANDROID	
    self->last_mix_buffer = NULL;
#endif
    self->ci_shared_surfaces = NULL;
    self->surfaces= NULL;
    self->surface_num = 0;
    self->coded_buf_index = 0;

    parent->initialized = FALSE;
	
}

static void mix_videoformatenc_h263_class_init(
        MixVideoFormatEnc_H263Class * klass) {

    /* root class */
    GObjectClass *gobject_class = (GObjectClass *) klass;

    /* direct parent class */
    MixVideoFormatEncClass *video_formatenc_class = 
        MIX_VIDEOFORMATENC_CLASS(klass);

    /* parent class for later use */
    parent_class = reinterpret_cast<MixVideoFormatEncClass*>(g_type_class_peek_parent(klass));

    /* setup finializer */
    gobject_class->finalize = mix_videoformatenc_h263_finalize;

    /* setup vmethods with base implementation */
    video_formatenc_class->getcaps = mix_videofmtenc_h263_getcaps;
    video_formatenc_class->initialize = mix_videofmtenc_h263_initialize;
    video_formatenc_class->encode = mix_videofmtenc_h263_encode;
    video_formatenc_class->flush = mix_videofmtenc_h263_flush;
    video_formatenc_class->eos = mix_videofmtenc_h263_eos;
    video_formatenc_class->deinitialize = mix_videofmtenc_h263_deinitialize;
    video_formatenc_class->getmaxencodedbufsize = mix_videofmtenc_h263_get_max_encoded_buf_size;
}

MixVideoFormatEnc_H263 *
mix_videoformatenc_h263_new(void) {
    MixVideoFormatEnc_H263 *ret = reinterpret_cast<MixVideoFormatEnc_H263*>(
        g_object_new(MIX_TYPE_VIDEOFORMATENC_H263, NULL));

    return ret;
}

void mix_videoformatenc_h263_finalize(GObject * obj) {
    /* clean up here. */

    /*MixVideoFormatEnc_H263 *mix = MIX_VIDEOFORMATENC_H263(obj); */
    GObjectClass *root_class = (GObjectClass *) parent_class;

    LOG_V( "\n");

    /* Chain up parent */
    if (root_class->finalize) {
        root_class->finalize(obj);
    }
}

MixVideoFormatEnc_H263 *
mix_videoformatenc_h263_ref(MixVideoFormatEnc_H263 * mix) {
    return (MixVideoFormatEnc_H263 *) g_object_ref(G_OBJECT(mix));
}

/*H263 vmethods implementation */
MIX_RESULT mix_videofmtenc_h263_getcaps(MixVideoFormatEnc *mix, GString *msg) {

    LOG_V( "mix_videofmtenc_h263_getcaps\n");

    if (mix == NULL) {
        LOG_E( "mix == NULL\n");				
        return MIX_RESULT_NULL_PTR;	
    }
	

    if (parent_class->getcaps) {
        return parent_class->getcaps(mix, msg);
    }
    return MIX_RESULT_SUCCESS;
}

#define CLEAN_UP {\
        if(ret == MIX_RESULT_SUCCESS) {\
            parent->initialized = TRUE;\
        }\
        /*free profiles and entrypoints*/\
        if(va_profiles)\
            g_free(va_profiles);\
        if(va_entrypoints)\
            g_free(va_entrypoints);\
        if(surfaces)\
            g_free(surfaces);\
        g_mutex_unlock(parent->objectlock);\
    LOG_V( "end\n");		\
    return ret;}

MIX_RESULT mix_videofmtenc_h263_initialize(MixVideoFormatEnc *mix, 
        MixVideoConfigParamsEnc * config_params_enc,
        MixFrameManager * frame_mgr,
        MixBufferPool * input_buf_pool,
        MixSurfacePool ** surface_pool,
        VADisplay va_display ) {

    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    MixVideoFormatEnc *parent = NULL;
    MixVideoConfigParamsEncH263 * config_params_enc_h263;
    
    VAStatus va_status = VA_STATUS_SUCCESS;
    VASurfaceID * surfaces = NULL;
    
    gint va_max_num_profiles, va_max_num_entrypoints, va_max_num_attribs;
    gint va_num_profiles,  va_num_entrypoints;

    VAProfile *va_profiles = NULL;
    VAEntrypoint *va_entrypoints = NULL;
    VAConfigAttrib va_attrib[2];	
    guint index;		
	

    /*frame_mgr and input_buf_pool is reservered for future use*/
    
    if (mix == NULL || config_params_enc == NULL || va_display == NULL) {
        LOG_E( 
                "mix == NULL || config_params_enc == NULL || va_display == NULL\n");			
        return MIX_RESULT_NULL_PTR;
    }

    LOG_V( "begin\n");

    /* Chainup parent method. */
    if (parent_class->initialize) {
        ret = parent_class->initialize(mix, config_params_enc,
                frame_mgr, input_buf_pool, surface_pool, 
                va_display);
    }
    
    if (ret != MIX_RESULT_SUCCESS)
    {
        return ret;
    }

    if (!MIX_IS_VIDEOFORMATENC_H263(mix))
        return MIX_RESULT_INVALID_PARAM;

        parent = MIX_VIDEOFORMATENC(&(mix->parent));
        MixVideoFormatEnc_H263 *self = MIX_VIDEOFORMATENC_H263(mix);
        
        if (MIX_IS_VIDEOCONFIGPARAMSENC_H263 (config_params_enc)) {
            config_params_enc_h263 = 
                MIX_VIDEOCONFIGPARAMSENC_H263 (config_params_enc);
        } else {
            LOG_V( 
                    "mix_videofmtenc_h263_initialize:  no h263 config params found\n");
            return MIX_RESULT_FAIL;
        }
        
        g_mutex_lock(parent->objectlock);        

        LOG_V( 
                "Start to get properities from H263 params\n");

        /* get properties from H263 params Object, which is special to H263 format*/

        ret = mix_videoconfigparamsenc_h263_get_slice_num (config_params_enc_h263,
                &self->slice_num);
        
        if (ret != MIX_RESULT_SUCCESS) {
            LOG_E( 
                    "Failed to mix_videoconfigparamsenc_h263_get_slice_num\n");                             
            CLEAN_UP;
        }	
          
        ret = mix_videoconfigparamsenc_h263_get_dlk (config_params_enc_h263,
                &(self->disable_deblocking_filter_idc));
        
        if (ret != MIX_RESULT_SUCCESS) {
            LOG_E( 
                    "Failed to mix_videoconfigparamsenc_h263_get_dlk\n");            
            CLEAN_UP;
        }			


        LOG_V( 
                "======H263 Encode Object properities======:\n");

        LOG_I( "self->slice_num = %d\n", 
                self->slice_num);			
        LOG_I( "self->disabled_deblocking_filter_idc = %d\n\n", 
                self->disable_deblocking_filter_idc);					
        
        LOG_V( 
                "Get properities from params done\n");
        
        parent->va_display = va_display;	
        
        LOG_V( "Get Display\n");
        LOG_I( "Display = 0x%08x\n", 
                (guint)va_display);			

#if 0
        /* query the vender information, can ignore*/
        va_vendor = vaQueryVendorString (va_display);
        LOG_I( "Vendor = %s\n", 
                va_vendor);			
#endif		
        
        /*get the max number for profiles/entrypoints/attribs*/
        va_max_num_profiles = vaMaxNumProfiles(va_display);
        LOG_I( "va_max_num_profiles = %d\n", 
                va_max_num_profiles);		
        
        va_max_num_entrypoints = vaMaxNumEntrypoints(va_display);
        LOG_I( "va_max_num_entrypoints = %d\n", 
                va_max_num_entrypoints);	
        
        va_max_num_attribs = vaMaxNumConfigAttributes(va_display);
        LOG_I( "va_max_num_attribs = %d\n", 
                va_max_num_attribs);		        
          
        va_profiles = reinterpret_cast<VAProfile*>(g_malloc(sizeof(VAProfile)*va_max_num_profiles));
        va_entrypoints = reinterpret_cast<VAEntrypoint*>(g_malloc(sizeof(VAEntrypoint)*va_max_num_entrypoints));	
        
        if (va_profiles == NULL || va_entrypoints ==NULL) 
        {
            LOG_E( 
                    "!va_profiles || !va_entrypoints\n");	
            ret = MIX_RESULT_NO_MEMORY;
            CLEAN_UP;
        }

        LOG_I( 
                "va_profiles = 0x%08x\n", (guint)va_profiles);		
		
        LOG_V( "vaQueryConfigProfiles\n");
		        	 	 
        
        va_status = vaQueryConfigProfiles (va_display, va_profiles, &va_num_profiles);
        
        if (va_status != VA_STATUS_SUCCESS)	 
        {
            LOG_E( 
                    "Failed to call vaQueryConfigProfiles\n");	
            ret = MIX_RESULT_FAIL;
            CLEAN_UP;
        }
        
        LOG_V( "vaQueryConfigProfiles Done\n");

        
        
        /*check whether profile is supported*/
        for(index= 0; index < va_num_profiles; index++) {
            if(parent->va_profile == va_profiles[index])
                break;
        }
        
        if(index == va_num_profiles) 
        {
            LOG_E( "Profile not supported\n");				
            ret = MIX_RESULT_FAIL;
            CLEAN_UP;
        }

        LOG_V( "vaQueryConfigEntrypoints\n");
        
	
        /*Check entry point*/
        va_status = vaQueryConfigEntrypoints(va_display, 
                parent->va_profile, 
                va_entrypoints, &va_num_entrypoints);
        
        if (va_status != VA_STATUS_SUCCESS)	 
        {
            LOG_E( 
                    "Failed to call vaQueryConfigEntrypoints\n");	
            ret = MIX_RESULT_FAIL;
            CLEAN_UP;
        }
        
        for (index = 0; index < va_num_entrypoints; index ++) {
            if (va_entrypoints[index] == VAEntrypointEncSlice) {
                break;
            }
        }
        
        if (index == va_num_entrypoints) {
            LOG_E( "Entrypoint not found\n");			
            ret = MIX_RESULT_FAIL;
            CLEAN_UP;
        }	
        
        va_attrib[0].type = VAConfigAttribRTFormat;
        va_attrib[1].type = VAConfigAttribRateControl;
        
        LOG_V( "vaGetConfigAttributes\n");
        
        va_status = vaGetConfigAttributes(va_display, parent->va_profile, 
                parent->va_entrypoint,
                &va_attrib[0], 2);		
        
        if (va_status != VA_STATUS_SUCCESS)	 
        {
            LOG_E( 
                    "Failed to call vaGetConfigAttributes\n");	
            ret = MIX_RESULT_FAIL;
            CLEAN_UP;
        }
        
        if ((va_attrib[0].value & parent->va_format) == 0) {
            LOG_E( "Matched format not found\n");	
            ret = MIX_RESULT_FAIL;
            CLEAN_UP;
        }	  
        
        
        if ((va_attrib[1].value & parent->va_rcmode) == 0) {
            LOG_E( "RC mode not found\n");	
            ret = MIX_RESULT_FAIL;
            CLEAN_UP;
        }
        
        va_attrib[0].value = parent->va_format; //VA_RT_FORMAT_YUV420;
        va_attrib[1].value = parent->va_rcmode; 

        LOG_V( "======VA Configuration======\n");

        LOG_I( "profile = %d\n", 
                parent->va_profile);	
        LOG_I( "va_entrypoint = %d\n", 
                parent->va_entrypoint);	
        LOG_I( "va_attrib[0].type = %d\n", 
                va_attrib[0].type);			
        LOG_I( "va_attrib[1].type = %d\n", 
                va_attrib[1].type);				
        LOG_I( "va_attrib[0].value (Format) = %d\n", 
                va_attrib[0].value);			
        LOG_I( "va_attrib[1].value (RC mode) = %d\n", 
                va_attrib[1].value);				

        LOG_V( "vaCreateConfig\n");
		
        va_status = vaCreateConfig(va_display, parent->va_profile, 
                parent->va_entrypoint, 
                &va_attrib[0], 2, &(parent->va_config));
        
        if (va_status != VA_STATUS_SUCCESS)	 
        {
            LOG_E( "Failed vaCreateConfig\n");				
            ret = MIX_RESULT_FAIL;
            CLEAN_UP;
        }

        /*TODO: compute the surface number*/
        int numSurfaces;
        
        if (parent->share_buf_mode) {
            numSurfaces = 2;
        }
        else {
            numSurfaces = 8;
            parent->ci_frame_num = 0;			
        }
        
        self->surface_num = numSurfaces + parent->ci_frame_num;
        
        surfaces = reinterpret_cast<VASurfaceID*>(g_malloc(sizeof(VASurfaceID)*numSurfaces));
        
        if (surfaces == NULL)
        {
            LOG_E( 
                    "Failed allocate surface\n");	
            ret = MIX_RESULT_NO_MEMORY;
            CLEAN_UP;
        }
      
        LOG_V( "vaCreateSurfaces\n");
        
        va_status = vaCreateSurfaces(va_display, parent->picture_width, 
                parent->picture_height, parent->va_format,
                numSurfaces, surfaces);
        
        if (va_status != VA_STATUS_SUCCESS)	 
        {
            LOG_E( 
                    "Failed vaCreateSurfaces\n");	
            ret = MIX_RESULT_FAIL;
            CLEAN_UP;
        }

        if (parent->share_buf_mode) {
            
            LOG_V( 
                    "We are in share buffer mode!\n");	
            self->ci_shared_surfaces =  reinterpret_cast<VASurfaceID*>(
                g_malloc(sizeof(VASurfaceID) * parent->ci_frame_num));
    
            if (self->ci_shared_surfaces == NULL)
            {
                LOG_E( 
                        "Failed allocate shared surface\n");	
                ret = MIX_RESULT_NO_MEMORY;
                CLEAN_UP;
            }
            
            guint index;
            for(index = 0; index < parent->ci_frame_num; index++) {
                
                LOG_I( "ci_frame_id = %lu\n", 
                        parent->ci_frame_id[index]);	
                
                LOG_V( 
                        "vaCreateSurfaceFromCIFrame\n");		
                
                va_status = vaCreateSurfaceFromCIFrame(va_display, 
                        (gulong) (parent->ci_frame_id[index]), 
                        &self->ci_shared_surfaces[index]);
                if (va_status != VA_STATUS_SUCCESS)	 
                {
                    LOG_E( 
                            "Failed to vaCreateSurfaceFromCIFrame\n");				   
                    ret = MIX_RESULT_FAIL;
                    CLEAN_UP;
                }		
            }
            
            LOG_V( 
                    "vaCreateSurfaceFromCIFrame Done\n");
            
        }// if (parent->share_buf_mode)
        
        self->surfaces = reinterpret_cast<VASurfaceID*>(g_malloc(sizeof(VASurfaceID) * self->surface_num));
        
        if (self->surfaces == NULL)
        {
            LOG_E( 
                    "Failed allocate private surface\n");	
            ret = MIX_RESULT_NO_MEMORY;
            CLEAN_UP;
        }		

        if (parent->share_buf_mode) {  
            /*shared surfaces should be put in pool first, 
             * because we will get it accoring to CI index*/
            for(index = 0; index < parent->ci_frame_num; index++)
                self->surfaces[index] = self->ci_shared_surfaces[index];
        }
        
        for(index = 0; index < numSurfaces; index++) {
            self->surfaces[index + parent->ci_frame_num] = surfaces[index];	
        }

        LOG_V( "assign surface Done\n");	
        LOG_I( "Created %d libva surfaces\n", 
                numSurfaces + parent->ci_frame_num);		
        
#if 0  //current put this in gst
        images = g_malloc(sizeof(VAImage)*numSurfaces);	
        if (images == NULL)
        {
            g_mutex_unlock(parent->objectlock);            
            return MIX_RESULT_FAIL;
        }		
        
        for (index = 0; index < numSurfaces; index++) {   
            //Derive an VAImage from an existing surface. 
            //The image buffer can then be mapped/unmapped for CPU access
            va_status = vaDeriveImage(va_display, surfaces[index],
                    &images[index]);
        }
#endif		 
        
        LOG_V( "mix_surfacepool_new\n");		

        parent->surfacepool = mix_surfacepool_new();
        if (surface_pool)
            *surface_pool = parent->surfacepool;  
        //which is useful to check before encode

        if (parent->surfacepool == NULL)
        {
            LOG_E( 
                    "Failed to mix_surfacepool_new\n");
            ret = MIX_RESULT_FAIL;
            CLEAN_UP;
        }

        LOG_V( 
                "mix_surfacepool_initialize\n");			
        
        ret = mix_surfacepool_initialize(parent->surfacepool,
                self->surfaces, parent->ci_frame_num + numSurfaces, va_display);
        
        switch (ret)
        {
            case MIX_RESULT_SUCCESS:
                break;
            case MIX_RESULT_ALREADY_INIT:

                LOG_E("Error init failure\n");

                ret = MIX_RESULT_ALREADY_INIT;
                CLEAN_UP;
            default:
                break;
        }

        
        //Initialize and save the VA context ID
        LOG_V( "vaCreateContext\n");		        
          
        va_status = vaCreateContext(va_display, parent->va_config,
                parent->picture_width, parent->picture_height,
                VA_PROGRESSIVE, self->surfaces, parent->ci_frame_num + numSurfaces,
                &(parent->va_context));
        
        LOG_I( 
                "Created libva context width %d, height %d\n", 
                parent->picture_width, parent->picture_height);
        
        if (va_status != VA_STATUS_SUCCESS)	 
        {
            LOG_E( 
                    "Failed to vaCreateContext\n");	
            LOG_I( "va_status = %d\n", 
                    (guint)va_status);			
            ret = MIX_RESULT_FAIL;
            CLEAN_UP;
        }

	    guint max_size = 0;
        ret = mix_videofmtenc_h263_get_max_encoded_buf_size (parent, &max_size);
        if (ret != MIX_RESULT_SUCCESS)
        {
            LOG_E( 
                    "Failed to mix_videofmtenc_h263_get_max_encoded_buf_size\n");	
            ret = MIX_RESULT_FAIL;
            CLEAN_UP;
        }
    
        /*Create coded buffer for output*/
        va_status = vaCreateBuffer (va_display, parent->va_context,
                VAEncCodedBufferType,
                self->coded_buf_size,  //
                1, NULL,
                &self->coded_buf[0]);
        
        if (va_status != VA_STATUS_SUCCESS)	 
        {
            LOG_E( 
                    "Failed to vaCreateBuffer: VAEncCodedBufferType\n");	
            ret = MIX_RESULT_FAIL;
            CLEAN_UP;
        }


        /*Create coded buffer for output*/
        va_status = vaCreateBuffer (va_display, parent->va_context,
                VAEncCodedBufferType,
                self->coded_buf_size,  //
                1, NULL,
                &(self->coded_buf[1]));
        
        if (va_status != VA_STATUS_SUCCESS)	 
        {
            LOG_E( 
                    "Failed to vaCreateBuffer: VAEncCodedBufferType\n");	
            ret = MIX_RESULT_FAIL;
            CLEAN_UP;
        }
		
#ifdef SHOW_SRC
        Display * display = XOpenDisplay (NULL);

        LOG_I( "display = 0x%08x\n", 
                (guint) display);	        
        win = XCreateSimpleWindow(display, RootWindow(display, 0), 0, 0,
                parent->picture_width,  parent->picture_height, 0, 0,
                WhitePixel(display, 0));
        XMapWindow(display, win);
        XSelectInput(display, win, KeyPressMask | StructureNotifyMask);

        XSync(display, False);
        LOG_I( "va_display = 0x%08x\n", 
                (guint) va_display);	            
        
#endif /* SHOW_SRC */		

       CLEAN_UP;
}
#undef CLEAN_UP

#define CLEAN_UP {\
    LOG_V( "UnLocking\n");		\
    g_mutex_unlock(parent->objectlock);\
    LOG_V( "end\n");		\
    return ret;}

MIX_RESULT mix_videofmtenc_h263_encode(MixVideoFormatEnc *mix, MixBuffer * bufin[],
        gint bufincnt, MixIOVec * iovout[], gint iovoutcnt,
        MixVideoEncodeParams * encode_params) {
    
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    MixVideoFormatEnc *parent = NULL;
    
    LOG_V( "Begin\n");		
    
    /*currenly only support one input and output buffer*/

    if (bufincnt != 1 || iovoutcnt != 1) {
        LOG_E( 
                "buffer count not equel to 1\n");			
        LOG_E( 
                "maybe some exception occurs\n");				
    }
   
    if (mix == NULL ||bufin[0] == NULL ||  iovout[0] == NULL) {
        LOG_E( 
                "!mix || !bufin[0] ||!iovout[0]\n");				
        return MIX_RESULT_NULL_PTR;
    }
    
#if 0
    if (parent_class->encode) {
        return parent_class->encode(mix, bufin, bufincnt, iovout,
                iovoutcnt, encode_params);
    }
#endif
    
    if (! MIX_IS_VIDEOFORMATENC_H263(mix))
        return MIX_RESULT_INVALID_PARAM;

    parent = MIX_VIDEOFORMATENC(&(mix->parent));
    MixVideoFormatEnc_H263 *self = MIX_VIDEOFORMATENC_H263 (mix);
        
    LOG_V( "Locking\n");		
    g_mutex_lock(parent->objectlock);
        
        
    //TODO: also we could move some encode Preparation work to here
    
    LOG_V( 
            "mix_videofmtenc_h263_process_encode\n");		        

    ret = mix_videofmtenc_h263_process_encode (self, 
                bufin[0], iovout[0]);
    if (ret != MIX_RESULT_SUCCESS)
    {
        LOG_E( 
                "Failed mix_videofmtenc_h263_process_encode\n");		
        CLEAN_UP;
    }

    CLEAN_UP;
}
#undef CLEAN_UP

MIX_RESULT mix_videofmtenc_h263_flush(MixVideoFormatEnc *mix) {
    
    //MIX_RESULT ret = MIX_RESULT_SUCCESS;
    
    LOG_V( "Begin\n");	

    if (mix == NULL) {
        LOG_E( "mix == NULL\n");				
        return MIX_RESULT_NULL_PTR;	
    }	
 
    
    /*not chain to parent flush func*/
#if 0
    if (parent_class->flush) {
        return parent_class->flush(mix, msg);
    }
#endif
    
    if(!MIX_IS_VIDEOFORMATENC_H263(mix))
        return MIX_RESULT_INVALID_PARAM;

    MixVideoFormatEnc_H263 *self = MIX_VIDEOFORMATENC_H263(mix);
    
    g_mutex_lock(mix->objectlock);
    
    /*unref the current source surface*/ 
    if (self->cur_frame != NULL)
    {
        mix_videoframe_unref (self->cur_frame);
        self->cur_frame = NULL;
    }
    
    /*unref the reconstructed surface*/ 
    if (self->rec_frame != NULL)
    {
        mix_videoframe_unref (self->rec_frame);
        self->rec_frame = NULL;
    }

    /*unref the reference surface*/ 
    if (self->ref_frame != NULL)
    {
        mix_videoframe_unref (self->ref_frame);
        self->ref_frame = NULL;       
    }
#ifdef ANDROID
    if(self->last_mix_buffer) {
       mix_buffer_unref(self->last_mix_buffer);
       self->last_mix_buffer = NULL;
    }
#endif    
    /*reset the properities*/    
    self->encoded_frames = 0;
    self->pic_skipped = FALSE;
    self->is_intra = TRUE;
    
    g_mutex_unlock(mix->objectlock);
    
    LOG_V( "end\n");		
    
    return MIX_RESULT_SUCCESS;
}

MIX_RESULT mix_videofmtenc_h263_eos(MixVideoFormatEnc *mix) {

    LOG_V( "\n");		 

    if (mix == NULL) {
        LOG_E( "mix == NULL\n");				
        return MIX_RESULT_NULL_PTR;	
    }	

    if (parent_class->eos) {
        return parent_class->eos(mix);
    }
    return MIX_RESULT_SUCCESS;
}

#define CLEAN_UP {\
    parent->initialized = TRUE;\
    g_mutex_unlock(parent->objectlock);	\
    LOG_V( "end\n");			\
    return ret;}

MIX_RESULT mix_videofmtenc_h263_deinitialize(MixVideoFormatEnc *mix) {
    
    MixVideoFormatEnc *parent = NULL;
    VAStatus va_status;
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
   
    LOG_V( "Begin\n");		

    if (mix == NULL) {
        LOG_E( "mix == NULL\n");				
        return MIX_RESULT_NULL_PTR;	
    }	

    if(!MIX_IS_VIDEOFORMATENC_H263(mix))
        return MIX_RESULT_INVALID_PARAM;

    if(parent_class->deinitialize) {
        ret = parent_class->deinitialize(mix);
    }

    if(ret != MIX_RESULT_SUCCESS)
    {
        return ret;
    }
    
    parent = MIX_VIDEOFORMATENC(&(mix->parent));
    MixVideoFormatEnc_H263 *self = MIX_VIDEOFORMATENC_H263(mix);	
   
    LOG_V( "Release frames\n");		

    g_mutex_lock(parent->objectlock);

#if 0
    /*unref the current source surface*/ 
    if (self->cur_frame != NULL)
    {
        mix_videoframe_unref (self->cur_frame);
        self->cur_frame = NULL;
    }
#endif	
    
    /*unref the reconstructed surface*/ 
    if (self->rec_frame != NULL)
    {
        mix_videoframe_unref (self->rec_frame);
        self->rec_frame = NULL;
    }

    /*unref the reference surface*/ 
    if (self->ref_frame != NULL)
    {
        mix_videoframe_unref (self->ref_frame);
        self->ref_frame = NULL;       
    }	

    LOG_V( "Release surfaces\n");			

    if (self->ci_shared_surfaces)
    {
        g_free (self->ci_shared_surfaces);
        self->ci_shared_surfaces = NULL;
    }

    if (self->surfaces)
    {
        g_free (self->surfaces);    
        self->surfaces = NULL;
    }		

    LOG_V( "vaDestroyContext\n");	
    
    va_status = vaDestroyContext (parent->va_display, parent->va_context);
    if (va_status != VA_STATUS_SUCCESS)	 
    {
        LOG_E( 
                "Failed vaDestroyContext\n");		
        ret = MIX_RESULT_FAIL;
        CLEAN_UP;
    }		

    LOG_V( "vaDestroyConfig\n");	
    
    va_status = vaDestroyConfig (parent->va_display, parent->va_config);	
    if (va_status != VA_STATUS_SUCCESS)	 
    {
        LOG_E( 
                "Failed vaDestroyConfig\n");	
        ret = MIX_RESULT_FAIL;
        CLEAN_UP;
    }			

    CLEAN_UP;
}
#undef CLEAN_UP

MIX_RESULT mix_videofmtenc_h263_send_seq_params (MixVideoFormatEnc_H263 *mix)
{
    
    VAStatus va_status;
    VAEncSequenceParameterBufferH263 h263_seq_param;
    VABufferID				seq_para_buf_id;
	
    
    MixVideoFormatEnc *parent = NULL;
    
    if (mix == NULL) {
        LOG_E("mix = NULL\n");
        return MIX_RESULT_NULL_PTR;
    }
    
    LOG_V( "Begin\n\n");		
    
    if (!MIX_IS_VIDEOFORMATENC_H263(mix))
        return MIX_RESULT_INVALID_PARAM;

    parent = MIX_VIDEOFORMATENC(&(mix->parent));	
        
    /*set up the sequence params for HW*/
    h263_seq_param.bits_per_second= parent->bitrate;		
    h263_seq_param.frame_rate = 30; //hard-coded, driver need;
         //(unsigned int) (parent->frame_rate_num + parent->frame_rate_denom /2 ) / parent->frame_rate_denom;
    h263_seq_param.initial_qp = parent->initial_qp;
    h263_seq_param.min_qp = parent->min_qp;
    h263_seq_param.intra_period = parent->intra_period;
		
    //h263_seq_param.fixed_vop_rate = 30;

    LOG_V( 
                "===h263 sequence params===\n");		
        
    LOG_I( "bitrate = %d\n", 
                h263_seq_param.bits_per_second);	
    LOG_I( "frame_rate = %d\n", 
                h263_seq_param.frame_rate);		
    LOG_I( "initial_qp = %d\n", 
                h263_seq_param.initial_qp);		
    LOG_I( "min_qp = %d\n", 
                h263_seq_param.min_qp);	
    LOG_I( "intra_period = %d\n\n", 
                h263_seq_param.intra_period);				             
        
    va_status = vaCreateBuffer(parent->va_display, parent->va_context,
                VAEncSequenceParameterBufferType,
                sizeof(h263_seq_param),
                1, &h263_seq_param,
                &seq_para_buf_id);
    if (va_status != VA_STATUS_SUCCESS)	 
    {
        LOG_E( 
                    "Failed to vaCreateBuffer\n");				
        return MIX_RESULT_FAIL;
    }
        
    va_status = vaRenderPicture(parent->va_display, parent->va_context, 
                &seq_para_buf_id, 1);
    if (va_status != VA_STATUS_SUCCESS)	 
    {
        LOG_E( 
                    "Failed to vaRenderPicture\n");		
        LOG_I( "va_status = %d\n", va_status);			
        return MIX_RESULT_FAIL;
    }
    
    LOG_V( "end\n");		
    
    return MIX_RESULT_SUCCESS;
    
}

MIX_RESULT mix_videofmtenc_h263_send_picture_parameter (MixVideoFormatEnc_H263 *mix)
{
    VAStatus va_status;
    VAEncPictureParameterBufferH263 h263_pic_param;
    MixVideoFormatEnc *parent = NULL;
    
    if (mix == NULL) {
        LOG_E("mix = NULL\n");
        return MIX_RESULT_NULL_PTR;
    }

    LOG_V( "Begin\n\n");		
    
#if 0 //not needed currently
    MixVideoConfigParamsEncH263 * params_h263
        = MIX_VIDEOCONFIGPARAMSENC_H263 (config_params_enc);
#endif	
    
    if (! MIX_IS_VIDEOFORMATENC_H263(mix)) 
        return MIX_RESULT_INVALID_PARAM;

        
        parent = MIX_VIDEOFORMATENC(&(mix->parent));
        
        /*set picture params for HW*/
        h263_pic_param.reference_picture = mix->ref_frame->frame_id;  
        h263_pic_param.reconstructed_picture = mix->rec_frame->frame_id;
        h263_pic_param.coded_buf = mix->coded_buf[mix->coded_buf_index];
        h263_pic_param.picture_width = parent->picture_width;
        h263_pic_param.picture_height = parent->picture_height;
        h263_pic_param.picture_type = mix->is_intra ? VAEncPictureTypeIntra : VAEncPictureTypePredictive;	
		
        

        LOG_V( 
                "======h263 picture params======\n");		
        LOG_I( "reference_picture = 0x%08x\n", 
                h263_pic_param.reference_picture);	
        LOG_I( "reconstructed_picture = 0x%08x\n", 
                h263_pic_param.reconstructed_picture);	
        LOG_I( "coded_buf = 0x%08x\n", 
                h263_pic_param.coded_buf);	
        LOG_I( "coded_buf_index = %d\n", 
                mix->coded_buf_index);			
        LOG_I( "picture_width = %d\n", 
                h263_pic_param.picture_width);	
        LOG_I( "picture_height = %d\n", 
                h263_pic_param.picture_height);		
        LOG_I( "picture_type = %d\n\n", 
                h263_pic_param.picture_type);			
       
        va_status = vaCreateBuffer(parent->va_display, parent->va_context,
                VAEncPictureParameterBufferType,
                sizeof(h263_pic_param),
                1,&h263_pic_param,
                &mix->pic_param_buf);	
        
        if (va_status != VA_STATUS_SUCCESS)	 
        {
            LOG_E( 
                    "Failed to vaCreateBuffer\n");												
            return MIX_RESULT_FAIL;
        }
        
        
        va_status = vaRenderPicture(parent->va_display, parent->va_context,
                &mix->pic_param_buf, 1);	
        
        if (va_status != VA_STATUS_SUCCESS)	 
        {
            LOG_E( 
                    "Failed to vaRenderPicture\n");		
            LOG_I( "va_status = %d\n", va_status);			
            return MIX_RESULT_FAIL;
        }			
    
    LOG_V( "end\n");		
    return MIX_RESULT_SUCCESS;  
    
}


MIX_RESULT mix_videofmtenc_h263_send_slice_parameter (MixVideoFormatEnc_H263 *mix)
{
    VAStatus va_status;
    
    guint slice_num;
    guint slice_height;
    guint slice_index;
    guint slice_height_in_mb;
    
    if (mix == NULL) {
        LOG_E("mix = NULL\n");
        return MIX_RESULT_NULL_PTR;
    }

    LOG_V("Begin\n\n");			
    
    
    MixVideoFormatEnc *parent = NULL;	
    
    if (! MIX_IS_VIDEOFORMATENC_H263(mix))
        return MIX_RESULT_INVALID_PARAM;

        parent = MIX_VIDEOFORMATENC(&(mix->parent));		
        
        //slice_num = mix->slice_num;
        slice_num = 1; // one slice per picture;
        slice_height = parent->picture_height / slice_num;	
        
        slice_height += 15;
        slice_height &= (~15);

        va_status = vaCreateBuffer (parent->va_display, parent->va_context, 
                VAEncSliceParameterBufferType,
                sizeof(VAEncSliceParameterBuffer),
                slice_num, NULL,
                &mix->slice_param_buf);		
        
        if (va_status != VA_STATUS_SUCCESS)	 
        {
            LOG_E("Failed to vaCreateBuffer\n");				
            return MIX_RESULT_FAIL;
        }
	        
        VAEncSliceParameterBuffer *slice_param, *current_slice;
        
        va_status = vaMapBuffer(parent->va_display,
                mix->slice_param_buf,
                (void **)&slice_param);	
        
        if (va_status != VA_STATUS_SUCCESS)	 
        {
            LOG_E("Failed to vaMapBuffer\n");				
            return MIX_RESULT_FAIL;
        }			
        
        current_slice = slice_param;        
        
        for (slice_index = 0; slice_index < slice_num; slice_index++) {
            current_slice = slice_param + slice_index;
            slice_height_in_mb = 
                min (slice_height, parent->picture_height  
                        - slice_index * slice_height) / 16;
            
            // starting MB row number for this slice
            current_slice->start_row_number = slice_index * slice_height / 16;  
            // slice height measured in MB
            current_slice->slice_height = slice_height_in_mb;   
            current_slice->slice_flags.bits.is_intra = mix->is_intra;	
            current_slice->slice_flags.bits.disable_deblocking_filter_idc 
                = mix->disable_deblocking_filter_idc;
            
            LOG_V("======h263 slice params======\n");		

            LOG_I("slice_index = %d\n",  		      
                    (gint) slice_index);	 
            LOG_I("start_row_number = %d\n", 
                    (gint) current_slice->start_row_number);	
            LOG_I("slice_height_in_mb = %d\n", 
                    (gint) current_slice->slice_height);		
            LOG_I("slice.is_intra = %d\n", 
                    (gint) current_slice->slice_flags.bits.is_intra);		
            LOG_I("disable_deblocking_filter_idc = %d\n\n", 
                    (gint) mix->disable_deblocking_filter_idc);			
            
        }
                    
        va_status = vaUnmapBuffer(parent->va_display, mix->slice_param_buf);
        
        if (va_status != VA_STATUS_SUCCESS)	 
        {
            LOG_E("Failed to vaUnmapBuffer\n");					
            return MIX_RESULT_FAIL;
        }	

        va_status = vaRenderPicture(parent->va_display, parent->va_context,
                &mix->slice_param_buf, 1);	
        
        if (va_status != VA_STATUS_SUCCESS)	 
        {
            LOG_E("Failed to vaRenderPicture\n");				
            return MIX_RESULT_FAIL;
        }	
                    
    LOG_V("end\n");		
                    
    return MIX_RESULT_SUCCESS;
}
            

#define CLEAN_UP {\
    if(ret != MIX_RESULT_SUCCESS) {\
        if(iovout->data) {\
            g_free(iovout->data);\
            iovout->data = NULL;\
        }\
    }\
    LOG_V( "end\n");		\
    return MIX_RESULT_SUCCESS;}

MIX_RESULT mix_videofmtenc_h263_process_encode (MixVideoFormatEnc_H263 *mix,
        MixBuffer * bufin, MixIOVec * iovout)
{
    
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    VAStatus va_status = VA_STATUS_SUCCESS;
    VADisplay va_display = NULL;	
    VAContextID va_context;
    gulong surface = 0;
    guint16 width, height;
    
    MixVideoFrame *  tmp_frame;
    guint8 *buf;
    
    if ((mix == NULL) || (bufin == NULL) || (iovout == NULL)) {
        LOG_E( 
                "mix == NUL) || bufin == NULL || iovout == NULL\n");
        return MIX_RESULT_NULL_PTR;
    }    

    LOG_V( "Begin\n");		
    
    if (! MIX_IS_VIDEOFORMATENC_H263(mix))
        return MIX_RESULT_INVALID_PARAM;
        
        MixVideoFormatEnc *parent = MIX_VIDEOFORMATENC(&(mix->parent));
        
        va_display = parent->va_display;
        va_context = parent->va_context;
        width = parent->picture_width;
        height = parent->picture_height;		
        

        LOG_I( "encoded_frames = %d\n", 
                mix->encoded_frames);	
        LOG_I( "is_intra = %d\n", 
                mix->is_intra);	
        LOG_I( "ci_frame_id = 0x%08x\n", 
                (guint) parent->ci_frame_id);
		
        /* determine the picture type*/
        if ((mix->encoded_frames % parent->intra_period) == 0) {
            mix->is_intra = TRUE;
        } else {
            mix->is_intra = FALSE;
        }		

        LOG_I( "is_intra_picture = %d\n", 
                mix->is_intra);			
        
        LOG_V( 
                "Get Surface from the pool\n");		
        
        /*current we use one surface for source data, 
         * one for reference and one for reconstructed*/
        /*TODO, could be refine here*/

        if (!parent->share_buf_mode) {
            LOG_V( 
                    "We are NOT in share buffer mode\n");		
            
            if (mix->ref_frame == NULL)
            {
                ret = mix_surfacepool_get(parent->surfacepool, &mix->ref_frame);
                if (ret != MIX_RESULT_SUCCESS)  //#ifdef SLEEP_SURFACE not used
                {
                    LOG_E( 
                            "Failed to mix_surfacepool_get\n");	
                    CLEAN_UP;
                }
            }
            
            if (mix->rec_frame == NULL)	
            {
                ret = mix_surfacepool_get(parent->surfacepool, &mix->rec_frame);
                if (ret != MIX_RESULT_SUCCESS)
                {
                    LOG_E( 
                            "Failed to mix_surfacepool_get\n");					
                    CLEAN_UP;
                }
            }

            if (parent->need_display) {
                mix->cur_frame = NULL;				
            }
			
            if (mix->cur_frame == NULL)
            {
                ret = mix_surfacepool_get(parent->surfacepool, &mix->cur_frame);
                if (ret != MIX_RESULT_SUCCESS)
                {
                    LOG_E( 
                            "Failed to mix_surfacepool_get\n");					
                    CLEAN_UP;
                }			
            }
            
            LOG_V( "Get Surface Done\n");		

            
            VAImage src_image;
            guint8 *pvbuf;
            guint8 *dst_y;
            guint8 *dst_uv;	
            int i,j;
            
            LOG_V( 
                    "map source data to surface\n");	
            
            ret = mix_videoframe_get_frame_id(mix->cur_frame, &surface);
            if (ret != MIX_RESULT_SUCCESS)
            {
                LOG_E( 
                        "Failed to mix_videoframe_get_frame_id\n");				
                CLEAN_UP;
            }
            
            
            LOG_I( 
                    "surface id = 0x%08x\n", (guint) surface);
            
            va_status = vaDeriveImage(va_display, surface, &src_image);	 
            //need to destroy
            
            if (va_status != VA_STATUS_SUCCESS)	 
            {
                LOG_E( 
                        "Failed to vaDeriveImage\n");			
                ret = MIX_RESULT_FAIL;
                CLEAN_UP;
            }
            
            VAImage *image = &src_image;
            
            LOG_V( "vaDeriveImage Done\n");			
    
            
            va_status = vaMapBuffer (va_display, image->buf, (void **)&pvbuf);
            if (va_status != VA_STATUS_SUCCESS)	 
            {
                LOG_E( "Failed to vaMapBuffer\n");
                ret = MIX_RESULT_FAIL;
                CLEAN_UP;
            }		
            
            LOG_V( 
                    "vaImage information\n");	
            LOG_I( 
                    "image->pitches[0] = %d\n", image->pitches[0]);
            LOG_I( 
                    "image->pitches[1] = %d\n", image->pitches[1]);		
            LOG_I( 
                    "image->offsets[0] = %d\n", image->offsets[0]);
            LOG_I( 
                    "image->offsets[1] = %d\n", image->offsets[1]);	
            LOG_I( 
                    "image->num_planes = %d\n", image->num_planes);			
            LOG_I( 
                    "image->width = %d\n", image->width);			
            LOG_I( 
                    "image->height = %d\n", image->height);			
            
            LOG_I( 
                    "input buf size = %d\n", bufin->size);			
            
            guint8 *inbuf = bufin->data;      
           
#ifndef ANDROID
#define USE_SRC_FMT_YUV420
#else
#define USE_SRC_FMT_NV21
#endif

#ifdef USE_SRC_FMT_YUV420
            /*need to convert YUV420 to NV12*/
            dst_y = pvbuf +image->offsets[0];
            
            for (i = 0; i < height; i ++) {
                memcpy (dst_y, inbuf + i * width, width);
                dst_y += image->pitches[0];
            }
            
            dst_uv = pvbuf + image->offsets[1];
            
            for (i = 0; i < height / 2; i ++) {
                for (j = 0; j < width; j+=2) {
                    dst_uv [j] = inbuf [width * height + i * width / 2 + j / 2];
                    dst_uv [j + 1] = 
                        inbuf [width * height * 5 / 4 + i * width / 2 + j / 2];
                }
                dst_uv += image->pitches[1];
            }
           
#else //USE_SRC_FMT_NV12 or USE_SRC_FMT_NV21
            int offset_uv = width * height;
            guint8 *inbuf_uv = inbuf + offset_uv;
            int height_uv = height / 2;
            int width_uv = width;

            dst_y = pvbuf + image->offsets[0];
            for (i = 0; i < height; i++) {
                memcpy (dst_y, inbuf + i * width, width);
                dst_y += image->pitches[0];
            }

#ifdef USE_SRC_FMT_NV12
            dst_uv = pvbuf + image->offsets[1];
            for (i = 0; i < height_uv; i++) {
                memcpy(dst_uv, inbuf_uv + i * width_uv, width_uv);
                dst_uv += image->pitches[1];
            }
#else //USE_SRC_FMT_NV21
            dst_uv = pvbuf + image->offsets[1];
            for (i = 0; i < height_uv; i ++) {
                for (j = 0; j < width_uv; j += 2) {
                    dst_uv[j] = inbuf_uv[j+1];  //u
                    dst_uv[j+1] = inbuf_uv[j];  //v
                }
                dst_uv += image->pitches[1];
                inbuf_uv += width_uv;
            }
#endif
#endif //USE_SRC_FMT_YUV420
 
            va_status = vaUnmapBuffer(va_display, image->buf);	
            if (va_status != VA_STATUS_SUCCESS)	 
            {
                LOG_E( 
                        "Failed to vaUnmapBuffer\n");	
                ret = MIX_RESULT_FAIL;
                CLEAN_UP;
            }	
            
            va_status = vaDestroyImage(va_display, src_image.image_id);
            if (va_status != VA_STATUS_SUCCESS)	 
            {
                LOG_E( 
                        "Failed to vaDestroyImage\n");					
                ret =  MIX_RESULT_FAIL;
                CLEAN_UP;
            }	
            
            LOG_V( 
                    "Map source data to surface done\n");	
            
        }
        
        else {//if (!parent->share_buf_mode)
                   
            MixVideoFrame * frame = mix_videoframe_new();
            
            if (mix->ref_frame == NULL)
            {
                ret = mix_videoframe_set_ci_frame_idx (frame, mix->surface_num - 1);
                if (ret != MIX_RESULT_SUCCESS) 
                {
                    LOG_E( 
                            "mix_videoframe_set_ci_frame_idx failed\n");				
                    CLEAN_UP;
                }

                ret = mix_surfacepool_get_frame_with_ci_frameidx 
                    (parent->surfacepool, &mix->ref_frame, frame);
                if (ret != MIX_RESULT_SUCCESS)  //#ifdef SLEEP_SURFACE not used
                {
                    LOG_E( 
                            "get reference surface from pool failed\n");				
                    CLEAN_UP;
                }
            }
            
            if (mix->rec_frame == NULL)	
            {
                ret = mix_videoframe_set_ci_frame_idx (frame, mix->surface_num - 2);        
                if (ret != MIX_RESULT_SUCCESS)
                {
                    LOG_E( 
                            "mix_videoframe_set_ci_frame_idx failed\n");				
                    CLEAN_UP;
                }

                ret = mix_surfacepool_get_frame_with_ci_frameidx
                    (parent->surfacepool, &mix->rec_frame, frame);

                if (ret != MIX_RESULT_SUCCESS)
                {
                    LOG_E( 
                            "get recontructed surface from pool failed\n");				
                    CLEAN_UP;
                }
            }

            if (parent->need_display) {
                mix->cur_frame = NULL;		
            }			
            
            if (mix->cur_frame == NULL)
            {
                guint ci_idx;
#ifndef ANDROID
                memcpy (&ci_idx, bufin->data, bufin->size);
#else
                memcpy (&ci_idx, bufin->data, sizeof(unsigned int));
#endif
 
                LOG_I( 
                        "surface_num = %d\n", mix->surface_num);			 
                LOG_I( 
                        "ci_frame_idx = %d\n", ci_idx);					
                
                if (ci_idx > mix->surface_num - 2) {
                    LOG_E( 
                            "the CI frame idx is too bigger than CI frame number\n");				
                    ret = MIX_RESULT_FAIL;
                    CLEAN_UP;			
                }
                
                
                ret = mix_videoframe_set_ci_frame_idx (frame, ci_idx);        
                if (ret != MIX_RESULT_SUCCESS)
                {
                    LOG_E( 
                            "mix_videoframe_set_ci_frame_idx failed\n");				
                    CLEAN_UP;
                }

                ret = mix_surfacepool_get_frame_with_ci_frameidx
                    (parent->surfacepool, &mix->cur_frame, frame);

                if (ret != MIX_RESULT_SUCCESS)
                {
                    LOG_E( 
                            "get current working surface from pool failed\n");
                    CLEAN_UP;
                }			
            }
            
            ret = mix_videoframe_get_frame_id(mix->cur_frame, &surface);
            
        }
        
        LOG_V( "vaBeginPicture\n");	
        LOG_I( "va_context = 0x%08x\n",(guint)va_context);
        LOG_I( "surface = 0x%08x\n",(guint)surface);	        
        LOG_I( "va_display = 0x%08x\n",(guint)va_display);
		

        va_status = vaBeginPicture(va_display, va_context, surface);
        if (va_status != VA_STATUS_SUCCESS)	 
        {
            LOG_E( "Failed vaBeginPicture\n");
            ret = MIX_RESULT_FAIL;
            CLEAN_UP;
        }	
        
        ret = mix_videofmtenc_h263_send_encode_command (mix);
        if (ret != MIX_RESULT_SUCCESS)
        {
            LOG_E ( 
                    "Failed mix_videofmtenc_h264_send_encode_command\n");	
            CLEAN_UP;
        }			
        
        
        if ((parent->va_rcmode == VA_RC_NONE) || mix->encoded_frames == 0) {
            
            va_status = vaEndPicture (va_display, va_context);
            if (va_status != VA_STATUS_SUCCESS)	 
            {
                LOG_E( "Failed vaEndPicture\n");		
                ret = MIX_RESULT_FAIL;
                CLEAN_UP;
            }
        }
        
        if (mix->encoded_frames == 0) {
            mix->encoded_frames ++;
            mix->last_coded_buf = mix->coded_buf[mix->coded_buf_index];
            mix->coded_buf_index ++; 
            mix->coded_buf_index %=2;
            
            mix->last_frame = mix->cur_frame;
            
            
            /* determine the picture type*/
            if ((mix->encoded_frames % parent->intra_period) == 0) {
                mix->is_intra = TRUE;
            } else {
                mix->is_intra = FALSE;
            }						
            
            tmp_frame = mix->rec_frame;
            mix->rec_frame= mix->ref_frame;
            mix->ref_frame = tmp_frame;			
            
            
        }
        
        LOG_V( "vaSyncSurface\n");	
        
        va_status = vaSyncSurface(va_display, mix->last_frame->frame_id);
        if (va_status != VA_STATUS_SUCCESS)	 
        {
            LOG_E( "Failed vaSyncSurface\n");		
            //return MIX_RESULT_FAIL;
        }				
        

        LOG_V( 
                "Start to get encoded data\n");		
        
        /*get encoded data from the VA buffer*/
        va_status = vaMapBuffer (va_display, mix->last_coded_buf, (void **)&buf);
        if (va_status != VA_STATUS_SUCCESS)	 
        {
            LOG_E( "Failed vaMapBuffer\n");	
            ret = MIX_RESULT_FAIL;
            CLEAN_UP;
        }			

        VACodedBufferSegment *coded_seg = NULL;
        int num_seg = 0;
        guint total_size = 0;
        guint size = 0;	

        coded_seg = (VACodedBufferSegment *)buf;
        num_seg = 1;

        while (1) {
            total_size += coded_seg->size;
		
            if (coded_seg->next == NULL)	
                break;		

            coded_seg = reinterpret_cast<VACodedBufferSegment*>(coded_seg->next);
            num_seg ++;
        }


#if 0	
        // first 4 bytes is the size of the buffer
		memcpy (&(iovout->data_size), (void*)buf, 4); 
        //size = (guint*) buf;
#endif

        iovout->data_size = total_size;
    
        if (iovout->data == NULL) { //means app doesn't allocate the buffer, so _encode will allocate it.
        
            iovout->data = (guchar*)g_malloc (iovout->data_size);
            if (iovout->data == NULL) {
                LOG_E( "iovout->data == NULL\n");				
                ret = MIX_RESULT_NO_MEMORY;	
                CLEAN_UP;				
            }		
        }
        
        //memcpy (iovout->data, buf + 16, iovout->data_size);

        coded_seg = (VACodedBufferSegment *)buf;
        total_size = 0;		
		
        while (1) {

            memcpy (iovout->data + total_size, coded_seg->buf, coded_seg->size);
            total_size += coded_seg->size;	
		
            if (coded_seg->next == NULL)	
                break;		

            coded_seg = reinterpret_cast<VACodedBufferSegment*>(coded_seg->next);
        }        

        iovout->buffer_size = iovout->data_size;
        
        LOG_I( 
                "out size is = %d\n", iovout->data_size);	
        
        va_status = vaUnmapBuffer (va_display, mix->last_coded_buf);
        if (va_status != VA_STATUS_SUCCESS)	 
        {
            LOG_E( "Failed vaUnmapBuffer\n");				
            ret = MIX_RESULT_FAIL;
            CLEAN_UP;
        }		
	
        LOG_V( "get encoded data done\n");		

       if (!((parent->va_rcmode == VA_RC_NONE) || mix->encoded_frames == 1)) {
            
            va_status = vaEndPicture (va_display, va_context);
            if (va_status != VA_STATUS_SUCCESS)	 
            {
                LOG_E( "Failed vaEndPicture\n");		
                return MIX_RESULT_FAIL;
            }
        }		
                
        if (mix->encoded_frames == 1) {
            va_status = vaBeginPicture(va_display, va_context, surface);
            if (va_status != VA_STATUS_SUCCESS)	 
            {
                LOG_E( "Failed vaBeginPicture\n");
                ret = MIX_RESULT_FAIL;
                CLEAN_UP;
            }				
            
            ret = mix_videofmtenc_h263_send_encode_command (mix);
            if (ret != MIX_RESULT_SUCCESS)
            {
                LOG_E ( 
                        "Failed mix_videofmtenc_h264_send_encode_command\n");	
                CLEAN_UP;
            }				
            
            va_status = vaEndPicture (va_display, va_context);
            if (va_status != VA_STATUS_SUCCESS)	 
            {
                LOG_E( "Failed vaEndPicture\n");		
                ret = MIX_RESULT_FAIL;
                CLEAN_UP;
            }					
            
        }	

        VASurfaceStatus status;
        
        /*query the status of current surface*/
        va_status = vaQuerySurfaceStatus(va_display, surface,  &status);
        if (va_status != VA_STATUS_SUCCESS)	 
        {
            LOG_E( 
                    "Failed vaQuerySurfaceStatus\n");				
            ret = MIX_RESULT_FAIL;
            CLEAN_UP;
        }				
        mix->pic_skipped = status & VASurfaceSkipped;		

	//ret = mix_framemanager_enqueue(parent->framemgr, mix->rec_frame);	

       if (parent->need_display) {
			ret = mix_videoframe_set_sync_flag(mix->cur_frame, TRUE);
			if (ret != MIX_RESULT_SUCCESS) {
				LOG_E("Failed to set sync_flag\n");
				CLEAN_UP;
			}

			ret = mix_framemanager_enqueue(parent->framemgr, mix->cur_frame);
			if (ret != MIX_RESULT_SUCCESS) {
				LOG_E("Failed mix_framemanager_enqueue\n");
				CLEAN_UP;
			}
		}

        /*update the reference surface and reconstructed surface */
        if (!mix->pic_skipped) {
            tmp_frame = mix->rec_frame;
            mix->rec_frame= mix->ref_frame;
            mix->ref_frame = tmp_frame;
        } 			
        
        
#if 0
        if (mix->ref_frame != NULL)
            mix_videoframe_unref (mix->ref_frame);
        mix->ref_frame = mix->rec_frame;
        
        mix_videoframe_unref (mix->cur_frame);
#endif		
        
        mix->encoded_frames ++;
        mix->last_coded_buf = mix->coded_buf[mix->coded_buf_index];
        mix->coded_buf_index ++; 
        mix->coded_buf_index %=2;
        mix->last_frame = mix->cur_frame;

#ifdef ANDROID        
        if(mix->last_mix_buffer) {
           LOG_V("calls to mix_buffer_unref \n");
           LOG_V("refcount = %d\n", MIX_PARAMS(mix->last_mix_buffer)->refcount);
           mix_buffer_unref(mix->last_mix_buffer);
        }

        LOG_V("ref the current bufin\n");
        mix->last_mix_buffer = mix_buffer_ref(bufin);
#endif

        if (!(parent->need_display)) {
             mix_videoframe_unref (mix->cur_frame);
             mix->cur_frame = NULL;
        }
        CLEAN_UP;
}
#undef CLEAN_UP

MIX_RESULT mix_videofmtenc_h263_get_max_encoded_buf_size (
        MixVideoFormatEnc *mix, guint * max_size)
{

    MixVideoFormatEnc *parent = NULL;
	
    if (mix == NULL)
    {
        LOG_E( 
                "mix == NULL\n");
            return MIX_RESULT_NULL_PTR;
    }
    
    LOG_V( "Begin\n");		
	
    parent = MIX_VIDEOFORMATENC(mix);
    MixVideoFormatEnc_H263 *self = MIX_VIDEOFORMATENC_H263 (mix);			

    if (MIX_IS_VIDEOFORMATENC_H263(self)) {

        if (self->coded_buf_size > 0) {
            *max_size = self->coded_buf_size;
            LOG_V ("Already calculate the max encoded size, get the value directly");
            return MIX_RESULT_SUCCESS;  			
        }
                
        /*base on the rate control mode to calculate the defaule encoded buffer size*/
        if (self->va_rcmode == VA_RC_NONE) {
            self->coded_buf_size = 
                (parent->picture_width* parent->picture_height * 830) / (16 * 16);  
            // set to value according to QP
        }
        else {	
            self->coded_buf_size = parent->bitrate/ 4; 
        }
        
        self->coded_buf_size = 
            max (self->coded_buf_size , 
                    (parent->picture_width* parent->picture_height * 830) / (16 * 16));
        
        /*in case got a very large user input bit rate value*/
        self->coded_buf_size = 
            max(self->coded_buf_size, 
                    (parent->picture_width * parent->picture_height * 1.5 * 8));
        self->coded_buf_size =  (self->coded_buf_size + 15) &(~15);
    }
    else
    {
        LOG_E( 
                "not H263 video encode Object\n");				
        return MIX_RESULT_INVALID_PARAM;		
    }

    *max_size = self->coded_buf_size;
	
    LOG_V( "end\n");		
	
    return MIX_RESULT_SUCCESS;    
}

MIX_RESULT mix_videofmtenc_h263_send_encode_command (MixVideoFormatEnc_H263 *mix)
{
    MIX_RESULT ret = MIX_RESULT_SUCCESS;

    LOG_V( "Begin\n");		

    if (MIX_IS_VIDEOFORMATENC_H263(mix))
    {
        if (mix->encoded_frames == 0) {
            ret = mix_videofmtenc_h263_send_seq_params (mix);
            if (ret != MIX_RESULT_SUCCESS)
            {
                LOG_E( 
                        "Failed mix_videofmtenc_h263_send_seq_params\n");
                return MIX_RESULT_FAIL;
            }
        }
        
        ret = mix_videofmtenc_h263_send_picture_parameter (mix);	
        
        if (ret != MIX_RESULT_SUCCESS)
        {
            LOG_E( 
                    "Failed mix_videofmtenc_h263_send_picture_parameter\n");	
            return MIX_RESULT_FAIL;
        }
        
        ret = mix_videofmtenc_h263_send_slice_parameter (mix);
        if (ret != MIX_RESULT_SUCCESS)
        {            
            LOG_E( 
                    "Failed mix_videofmtenc_h263_send_slice_parameter\n");	
            return MIX_RESULT_FAIL;
        }		
        
    }
    else
    {
        LOG_E(
                "not H263 video encode Object\n");
        return MIX_RESULT_INVALID_PARAM;
    }

    LOG_V( "End\n");			

    return MIX_RESULT_SUCCESS;
}
