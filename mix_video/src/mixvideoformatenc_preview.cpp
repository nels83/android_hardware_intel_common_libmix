/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

#include <string.h>
#include <stdlib.h>

#include "mixvideolog.h"

#include "mixvideoformatenc_preview.h"
#include "mixvideoconfigparamsenc_preview.h"
#include <va/va_tpi.h>

#undef SHOW_SRC

#ifdef SHOW_SRC
Window win = 0;
#endif /* SHOW_SRC */

MixVideoFormatEnc_Preview::MixVideoFormatEnc_Preview()
        :shared_surfaces(NULL)
        ,surfaces(NULL)
        ,surface_num(0)
        ,shared_surfaces_cnt(0)
        ,precreated_surfaces_cnt(0)
        ,cur_frame(NULL)
        ,ref_frame(NULL)
        ,rec_frame(NULL)
        ,lookup_frame(NULL)
        ,encoded_frames(0)
        ,pic_skipped(FALSE)
        ,is_intra(TRUE)
        ,usrptr(NULL) {
}

MixVideoFormatEnc_Preview::~MixVideoFormatEnc_Preview() {
}

MixVideoFormatEnc_Preview *
mix_videoformatenc_preview_new(void) {
    return new MixVideoFormatEnc_Preview();
}


MixVideoFormatEnc_Preview *
mix_videoformatenc_preview_ref(MixVideoFormatEnc_Preview * mix) {
    if (NULL != mix) {
        mix->Ref();
        return mix;
    }
    else {
        return NULL;
    }
}

MixVideoFormatEnc_Preview *
mix_videoformatenc_preview_unref(MixVideoFormatEnc_Preview * mix) {
    if (NULL!=mix)
        if (NULL != mix->Unref())
            return mix;
        else
            return NULL;
    else
        return NULL;
}


MIX_RESULT MixVideoFormatEnc_Preview::Initialize(
    MixVideoConfigParamsEnc * config_params_enc,
    MixFrameManager * frame_mgr,
    MixBufferPool * input_buf_pool,
    MixSurfacePool ** surface_pool,
    MixUsrReqSurfacesInfo * requested_surface_info,
    VADisplay va_display ) {

    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    MixVideoConfigParamsEncPreview * config_params_enc_preview;

    VAStatus va_status = VA_STATUS_SUCCESS;
    VASurfaceID * surfaces = NULL;

    int va_max_num_profiles, va_max_num_entrypoints, va_max_num_attribs;
    int va_num_profiles, va_num_entrypoints;

    VAProfile *va_profiles = NULL;
    VAEntrypoint *va_entrypoints = NULL;
    VAConfigAttrib va_attrib[2];
    uint index;

    /*
    * Different MIX buffer mode will have different surface handling approach
    */
    uint normal_surfaces_cnt = 2;

    /*
    * shared_surfaces_cnt is for upstream buffer allocation case
    */
    uint shared_surfaces_cnt = 0;

    /*
    * precreated_surfaces_cnt is for self buffer allocation case
    */
    uint precreated_surfaces_cnt = 0;

    MixCISharedBufferInfo * ci_info = NULL;

    /*frame_mgr and input_buf_pool is reservered for future use*/
    if (config_params_enc == NULL || va_display == NULL) {
        LOG_E("config_params_enc == NULL || va_display == NULL\n");
        return MIX_RESULT_NULL_PTR;
    }

    LOG_V( "begin\n");

    /* Chainup parent method. */
    ret = MixVideoFormatEnc::Initialize(
              config_params_enc, frame_mgr, input_buf_pool,
              surface_pool, requested_surface_info, va_display);
    if (ret != MIX_RESULT_SUCCESS) {
        return ret;
    }

    if (MIX_IS_VIDEOCONFIGPARAMSENC_PREVIEW(config_params_enc)) {
        config_params_enc_preview = MIX_VIDEOCONFIGPARAMSENC_PREVIEW (config_params_enc);
    } else {
        LOG_V("mix_videofmtenc_preview_initialize:  no preview config params found\n");
        return MIX_RESULT_FAIL;
    }

    Lock();

    LOG_V("Get properities from params done\n");

    this->va_display = va_display;

    LOG_V( "Get Display\n");
    LOG_I( "Display = 0x%08x\n", (uint)va_display);

    /*get the max number for profiles/entrypoints/attribs*/
    va_max_num_profiles = vaMaxNumProfiles(va_display);
    LOG_I( "va_max_num_profiles = %d\n", va_max_num_profiles);

    va_max_num_entrypoints = vaMaxNumEntrypoints(va_display);
    LOG_I( "va_max_num_entrypoints = %d\n", va_max_num_entrypoints);

    va_max_num_attribs = vaMaxNumConfigAttributes(va_display);
    LOG_I( "va_max_num_attribs = %d\n", va_max_num_attribs);

    va_profiles = new VAProfile[va_max_num_profiles];
    va_entrypoints = new VAEntrypoint[va_max_num_entrypoints];

    if (va_profiles == NULL || va_entrypoints ==NULL) {
        LOG_E("!va_profiles || !va_entrypoints\n");
        ret = MIX_RESULT_NO_MEMORY;
        goto cleanup;
    }

    LOG_I("va_profiles = 0x%08x\n", (uint)va_profiles);
    LOG_V("vaQueryConfigProfiles\n");

    va_status = vaQueryConfigProfiles (va_display, va_profiles, &va_num_profiles);
    if (va_status != VA_STATUS_SUCCESS) {
        LOG_E("Failed to call vaQueryConfigProfiles\n");
        ret = MIX_RESULT_FAIL;
        goto cleanup;
    }
    LOG_V( "vaQueryConfigProfiles Done\n");

    /*check whether profile is supported*/
    for (index= 0; index < va_num_profiles; index++) {
        if (this->va_profile == va_profiles[index])
            break;
    }

    if (index == va_num_profiles) {
        LOG_E( "Profile not supported\n");
        ret = MIX_RESULT_FAIL;
        goto cleanup;
    }

    LOG_V( "vaQueryConfigEntrypoints\n");

    /*Check entry point*/
    va_status = vaQueryConfigEntrypoints(
                    va_display,
                    this->va_profile,
                    va_entrypoints, &va_num_entrypoints);

    if (va_status != VA_STATUS_SUCCESS) {
        LOG_E("Failed to call vaQueryConfigEntrypoints\n");
        ret = MIX_RESULT_FAIL;
        goto cleanup;
    }

    for (index = 0; index < va_num_entrypoints; index ++) {
        if (va_entrypoints[index] == VAEntrypointEncSlice) {
            break;
        }
    }

    if (index == va_num_entrypoints) {
        LOG_E( "Entrypoint not found\n");
        ret = MIX_RESULT_FAIL;
        goto cleanup;
    }

    va_attrib[0].type = VAConfigAttribRTFormat;
    va_attrib[1].type = VAConfigAttribRateControl;

    LOG_V( "vaGetConfigAttributes\n");

    va_status = vaGetConfigAttributes(
                    va_display, this->va_profile,
                    this->va_entrypoint, &va_attrib[0], 2);

    if (va_status != VA_STATUS_SUCCESS) {
        LOG_E("Failed to call vaGetConfigAttributes\n");
        ret = MIX_RESULT_FAIL;
        goto cleanup;
    }

    if ((va_attrib[0].value & this->va_format) == 0) {
        LOG_E( "Matched format not found\n");
        ret = MIX_RESULT_FAIL;
        goto cleanup;
    }


    if ((va_attrib[1].value & this->va_rcmode) == 0) {
        LOG_E( "RC mode not found\n");
        ret = MIX_RESULT_FAIL;
        goto cleanup;
    }

    va_attrib[0].value = this->va_format; //VA_RT_FORMAT_YUV420;
    va_attrib[1].value = this->va_rcmode;

    LOG_V( "======VA Configuration======\n");
    LOG_I( "profile = %d\n", this->va_profile);
    LOG_I( "va_entrypoint = %d\n", this->va_entrypoint);
    LOG_I( "va_attrib[0].type = %d\n", va_attrib[0].type);
    LOG_I( "va_attrib[1].type = %d\n", va_attrib[1].type);
    LOG_I( "va_attrib[0].value (Format) = %d\n", va_attrib[0].value);
    LOG_I( "va_attrib[1].value (RC mode) = %d\n", va_attrib[1].value);
    LOG_V( "vaCreateConfig\n");

    va_status = vaCreateConfig(
                    va_display, this->va_profile,
                    this->va_entrypoint,
                    &va_attrib[0], 2, &(this->va_config));

    if (va_status != VA_STATUS_SUCCESS) {
        LOG_E( "Failed vaCreateConfig\n");
        ret = MIX_RESULT_FAIL;
        goto cleanup;
    }

    /*
     * For upstream allocates buffer, it is mandatory to set buffer mode
     * and for other stuff, it is optional
     */

    LOG_I ("alloc_surface_cnt = %d\n", requested_surface_info->surface_cnt);

    if (requested_surface_info->surface_cnt == 0) {
        switch (this->buffer_mode) {
        case MIX_BUFFER_UPSTREAM_ALLOC_CI:
            ci_info = (MixCISharedBufferInfo *) (this->buf_info);
            shared_surfaces_cnt = ci_info->ci_frame_cnt;
            normal_surfaces_cnt = 2;
            break;
        case MIX_BUFFER_UPSTREAM_ALLOC_V4L2:
            /*
            * To be develped
            */
            break;
        case MIX_BUFFER_UPSTREAM_ALLOC_SURFACE:
            /*
            * To be develped
            */
            break;
        default:
            this->buffer_mode = MIX_BUFFER_ALLOC_NORMAL;
            normal_surfaces_cnt = 8;
            break;
        }
    }
    else if (requested_surface_info->surface_cnt == 1) {
        /*
        * Un-normal case, TBD
        */
        this->buffer_mode = MIX_BUFFER_ALLOC_NORMAL;
        normal_surfaces_cnt = 8;
    }
    else {
        this->buffer_mode = MIX_BUFFER_SELF_ALLOC_SURFACE;
        precreated_surfaces_cnt = requested_surface_info->surface_cnt;
        this->alloc_surface_cnt = requested_surface_info->surface_cnt;
        this->usrptr = new uint8*[requested_surface_info->surface_cnt];
        if (this->usrptr == NULL) {
            LOG_E("Failed allocate memory\n");
            ret = MIX_RESULT_NO_MEMORY;
            goto cleanup;
        }

        memcpy (this->usrptr, requested_surface_info->usrptr, requested_surface_info->surface_cnt * sizeof (uint8 *));

    }

    LOG_I ("buffer_mode = %d\n", this->buffer_mode);

    this->shared_surfaces_cnt = shared_surfaces_cnt;
    this->precreated_surfaces_cnt = precreated_surfaces_cnt;

#if 0

    int ii = 0;
    for (ii=0; ii < alloc_surface_cnt; ii++) {

        g_print ("self->usrptr[%d] = 0x%08x\n", ii, self->usrptr[ii]);
        g_print ("usrptr[%d] = 0x%08x\n", ii, usrptr[ii]);


    }

    /*TODO: compute the surface number*/
    int numSurfaces;

    if (parent->share_buf_mode) {
        numSurfaces = 2;
    }
    else {
        numSurfaces = 2;
        parent->ci_frame_num = 0;
    }

    //self->surface_num = numSurfaces + parent->ci_frame_num;
#endif

    this->surface_num = normal_surfaces_cnt + shared_surfaces_cnt + precreated_surfaces_cnt;
    surfaces = new VASurfaceID[normal_surfaces_cnt];

    if (surfaces == NULL) {
        LOG_E("Failed allocate surface\n");
        ret = MIX_RESULT_NO_MEMORY;
        goto cleanup;
    }

    this->surfaces = new VASurfaceID[this->surface_num];

    if (this->surfaces == NULL) {
        LOG_E("Failed allocate private surface\n");
        ret = MIX_RESULT_NO_MEMORY;
        goto cleanup;
    }

    LOG_V( "vaCreateSurfaces\n");

    va_status = vaCreateSurfaces(
                    va_display, this->va_format,
                    this->picture_width, this->picture_height,
                    surfaces, normal_surfaces_cnt, NULL, 0 );

    if (va_status != VA_STATUS_SUCCESS) {
        LOG_E("Failed vaCreateSurfaces\n");
        ret = MIX_RESULT_FAIL;
        goto cleanup;
    }

    if (shared_surfaces_cnt != 0) {
        this->shared_surfaces = new VASurfaceID[shared_surfaces_cnt];
        if (this->shared_surfaces == NULL) {
            LOG_E("Failed allocate shared surface\n");
            ret = MIX_RESULT_NO_MEMORY;
            goto cleanup;
        }
    }


    switch (this->buffer_mode) {
    case MIX_BUFFER_UPSTREAM_ALLOC_CI:
    {
		#if 0
        for (index = 0; index < this->shared_surfaces_cnt; index++) {
            va_status = vaCreateSurfaceFromCIFrame(
                            va_display, (ulong) (ci_info->ci_frame_id[index]),
                            &this->shared_surfaces[index]);
            if (va_status != VA_STATUS_SUCCESS) {
                LOG_E("Failed to vaCreateSurfaceFromCIFrame\n");
                ret = MIX_RESULT_FAIL;
                goto cleanup;
            }
            this->surfaces[index] = this->shared_surfaces[index];
        }
		#endif
    }
    break;
    case MIX_BUFFER_UPSTREAM_ALLOC_V4L2:
        /*To be develped*/
        break;
    case MIX_BUFFER_UPSTREAM_ALLOC_SURFACE:
        /*To be develped*/
        break;
    case MIX_BUFFER_ALLOC_NORMAL:
        break;
    case MIX_BUFFER_SELF_ALLOC_SURFACE:
    {
        for (index = 0; index < requested_surface_info->surface_cnt; index ++) {
            this->surfaces[index] = requested_surface_info->surface_allocated[index];
        }
    }
    break;
    default:
        break;
    }


    for (index = 0; index < normal_surfaces_cnt; index++) {
        this->surfaces[precreated_surfaces_cnt + shared_surfaces_cnt + index] = surfaces[index];
    }

    LOG_V( "assign surface Done\n");
    LOG_I( "Created %d libva surfaces\n", this->surface_num);

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

    this->surfacepool = mix_surfacepool_new();
    if (surface_pool)
        *surface_pool = this->surfacepool;
    //which is useful to check before encode

    if (this->surfacepool == NULL) {
        LOG_E("Failed to mix_surfacepool_new\n");
        ret = MIX_RESULT_FAIL;
        goto cleanup;
    }

    LOG_V("mix_surfacepool_initialize\n");

    ret = mix_surfacepool_initialize(
              this->surfacepool, this->surfaces,
              this->surface_num, va_display);

    switch (ret) {
    case MIX_RESULT_SUCCESS:
        break;
    case MIX_RESULT_ALREADY_INIT:
        ret = MIX_RESULT_ALREADY_INIT;
        goto cleanup;
    default:
        break;
    }

    //Initialize and save the VA context ID
    LOG_V( "vaCreateContext\n");

    va_status = vaCreateContext(
                    va_display, this->va_config,
                    this->picture_width, this->picture_height,
                    0, this->surfaces, this->surface_num,
                    &(this->va_context));

    LOG_I("Created libva context width %d, height %d\n",
          this->picture_width, this->picture_height);

    if (va_status != VA_STATUS_SUCCESS) {
        LOG_E("Failed to vaCreateContext\n");
        LOG_I( "va_status = %d\n", (uint)va_status);
        ret = MIX_RESULT_FAIL;
        goto cleanup;
    }

    this->coded_buf_size = 4;

    /*Create coded buffer for output*/
    va_status = vaCreateBuffer (
                    va_display, this->va_context,
                    VAEncCodedBufferType,
                    this->coded_buf_size,  //
                    1, NULL,
                    &this->coded_buf);

    if (va_status != VA_STATUS_SUCCESS) {
        LOG_E("Failed to vaCreateBuffer: VAEncCodedBufferType\n");
        ret = MIX_RESULT_FAIL;
        goto cleanup;
    }

#ifdef SHOW_SRC
    Display * display = XOpenDisplay (NULL);
    LOG_I("display = 0x%08x\n", (uint) display);
    win = XCreateSimpleWindow(
              display, RootWindow(display, 0), 0, 0,
              this->picture_width,  this->picture_height, 0, 0,
              WhitePixel(display, 0));
    XMapWindow(display, win);
    XSelectInput(display, win, KeyPressMask | StructureNotifyMask);
    XSync(display, False);
    LOG_I( "va_display = 0x%08x\n", (uint) va_display);
#endif /* SHOW_SRC */

    LOG_V("end\n");

cleanup:

    if (ret == MIX_RESULT_SUCCESS) {
        this->initialized = TRUE;
    }

    /*free profiles and entrypoints*/
    if (va_profiles)
        delete [] va_profiles;

    if (va_entrypoints)
        delete [] va_entrypoints;

    if (surfaces)
        delete [] surfaces;

    Unlock();
    return ret;
}

MIX_RESULT MixVideoFormatEnc_Preview::Encode(
    MixBuffer * bufin[], int bufincnt,
    MixIOVec * iovout[], int iovoutcnt,
    MixVideoEncodeParams * encode_params) {

    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    LOG_V( "Begin\n");
    /*currenly only support one input and output buffer*/
    if (bufincnt != 1 || iovoutcnt != 1) {
        LOG_E("buffer count not equel to 1\n");
        LOG_E("maybe some exception occurs\n");
    }

    if (bufin[0] == NULL ||  iovout[0] == NULL) {
        LOG_E("!bufin[0] ||!iovout[0]\n");
        return MIX_RESULT_NULL_PTR;
    }
#if 0
    if (parent_class->encode) {
        return parent_class->encode(mix, bufin, bufincnt, iovout,
                                    iovoutcnt, encode_params);
    }
#endif
    LOG_V( "Locking\n");
    Lock();

    //TODO:  we also could move some encode Preparation work to here
    LOG_V("mix_videofmtenc_preview_process_encode\n");

    ret = _process_encode (bufin[0], iovout[0]);
    if (ret != MIX_RESULT_SUCCESS) {
        LOG_E("Failed mix_videofmtenc_preview_process_encode\n");
        goto cleanup;
    }

cleanup:

    LOG_V( "UnLocking\n");
    Unlock();
    LOG_V( "end\n");
    return ret;
}

MIX_RESULT MixVideoFormatEnc_Preview::Flush() {

    LOG_V( "Begin\n");
    /*not chain to parent flush func*/
#if 0
    if (parent_class->flush) {
        return parent_class->flush(mix, msg);
    }
#endif
    Lock();
#if 0
    /*unref the current source surface*/
    if (self->cur_frame != NULL) {
        mix_videoframe_unref (self->cur_frame);
        self->cur_frame = NULL;
    }
#endif

    /*unref the reconstructed surface*/
    if (this->rec_frame != NULL) {
        mix_videoframe_unref (this->rec_frame);
        this->rec_frame = NULL;
    }

    /*unref the reference surface*/
    if (this->ref_frame != NULL) {
        mix_videoframe_unref (this->ref_frame);
        this->ref_frame = NULL;
    }

    /*reset the properities*/
    this->encoded_frames = 0;
    this->pic_skipped = FALSE;
    this->is_intra = TRUE;

    Unlock();
    LOG_V( "end\n");
    return MIX_RESULT_SUCCESS;
}



MIX_RESULT MixVideoFormatEnc_Preview::Deinitialize() {
    VAStatus va_status;
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    LOG_V( "Begin\n");
    ret = MixVideoFormatEnc::Deinitialize();
    if (ret != MIX_RESULT_SUCCESS) {
        return ret;
    }

    LOG_V( "Release frames\n");
    Lock();

#if 0
    /*unref the current source surface*/
    if (self->cur_frame != NULL)
    {
        mix_videoframe_unref (self->cur_frame);
        self->cur_frame = NULL;
    }
#endif

    /*unref the reconstructed surface*/
    if (this->rec_frame != NULL) {
        mix_videoframe_unref (this->rec_frame);
        this->rec_frame = NULL;
    }

    /*unref the reference surface*/
    if (this->ref_frame != NULL) {
        mix_videoframe_unref (this->ref_frame);
        this->ref_frame = NULL;
    }

    if (this->lookup_frame != NULL) {
        mix_videoframe_unref (this->lookup_frame);
        this->lookup_frame = NULL;
    }

    LOG_V( "Release surfaces\n");

    if (this->shared_surfaces) {
        delete [] this->shared_surfaces;
        this->shared_surfaces = NULL;
    }

    if (this->surfaces) {
        delete [] this->surfaces;
        this->surfaces = NULL;
    }

    if (this->usrptr) {
        delete [] this->usrptr;
        this->usrptr = NULL;
    }

    LOG_V( "vaDestroyContext\n");

    va_status = vaDestroyContext (this->va_display, this->va_context);
    if (va_status != VA_STATUS_SUCCESS) {
        LOG_E("Failed vaDestroyContext\n");
        ret =  MIX_RESULT_FAIL;
        goto cleanup;
    }

    LOG_V( "vaDestroyConfig\n");

    va_status = vaDestroyConfig (this->va_display, this->va_config);
    if (va_status != VA_STATUS_SUCCESS) {
        LOG_E("Failed vaDestroyConfig\n");
        ret =  MIX_RESULT_FAIL;
        goto cleanup;
    }


cleanup:

    this->initialized = FALSE;
    Unlock();
    LOG_V( "end\n");
    return ret;
}


MIX_RESULT MixVideoFormatEnc_Preview::_process_encode (
    MixBuffer * bufin, MixIOVec * iovout) {

    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    VAStatus va_status = VA_STATUS_SUCCESS;
    VADisplay va_display = NULL;
    VAContextID va_context;
    ulong surface = 0;
    uint16 width, height;

    uint surface_idx = (uint) -1; //fixme, temp use a big value
    uint idx = 0;

    VAImage src_image;
    uint8 *pvbuf;
    uint8 *dst_y;
    uint8 *dst_uv;
    int i,j;

    //MixVideoFrame *  tmp_frame;
    //uint8 *buf;
    if ((bufin == NULL) || (iovout == NULL)) {
        LOG_E("bufin == NULL || iovout == NULL\n");
        return MIX_RESULT_NULL_PTR;
    }

    LOG_V( "Begin\n");
    va_display = this->va_display;
    va_context = this->va_context;
    width = this->picture_width;
    height = this->picture_height;

    LOG_I( "encoded_frames = %d\n", this->encoded_frames);
    LOG_I( "is_intra = %d\n", this->is_intra);
    LOG_I( "ci_frame_id = 0x%08x\n", (uint) this->ci_frame_id);
    LOG_V("Get Surface from the pool\n");

    switch (this->buffer_mode) {
    case MIX_BUFFER_UPSTREAM_ALLOC_CI:
    {
        //MixVideoFrame * frame = mix_videoframe_new();
        if (this->lookup_frame == NULL) {
            this->lookup_frame = mix_videoframe_new ();
            if (this->lookup_frame == NULL) {
                LOG_E("mix_videoframe_new() failed!\n");
                ret = MIX_RESULT_NO_MEMORY;
                goto cleanup;
            }
        }

        if (this->ref_frame == NULL) {
            ret = mix_videoframe_set_ci_frame_idx (this->lookup_frame, this->surface_num - 1);
            if (ret != MIX_RESULT_SUCCESS) {
                LOG_E("mix_videoframe_set_ci_frame_idx failed\n");
                goto cleanup;
            }

            ret = mix_surfacepool_get_frame_with_ci_frameidx
                  (this->surfacepool, &this->ref_frame, this->lookup_frame);
            if (ret != MIX_RESULT_SUCCESS) {
                LOG_E("get reference surface from pool failed\n");
                goto cleanup;
            }
        }

        if (this->rec_frame == NULL) {
            ret = mix_videoframe_set_ci_frame_idx (this->lookup_frame, this->surface_num - 2);
            if (ret != MIX_RESULT_SUCCESS) {
                LOG_E("mix_videoframe_set_ci_frame_idx failed\n");
                goto cleanup;
            }

            ret = mix_surfacepool_get_frame_with_ci_frameidx
                  (this->surfacepool, &this->rec_frame, this->lookup_frame);

            if (ret != MIX_RESULT_SUCCESS) {
                LOG_E("get recontructed surface from pool failed\n");
                goto cleanup;
            }
        }

        //mix_videoframe_unref (mix->cur_frame);
        if (this->need_display) {
            this->cur_frame = NULL;
        }

        if (this->cur_frame == NULL) {
            uint ci_idx;
#ifndef ANDROID
            memcpy (&ci_idx, bufin->data, bufin->size);
#else
            memcpy (&ci_idx, bufin->data, sizeof(unsigned int));
#endif

            LOG_I("surface_num = %d\n", this->surface_num);
            LOG_I("ci_frame_idx = %d\n", ci_idx);

            if (ci_idx > this->surface_num - 2) {
                LOG_E("the CI frame idx is too bigger than CI frame number\n");
                ret = MIX_RESULT_FAIL;
                goto cleanup;
            }

            ret = mix_videoframe_set_ci_frame_idx (this->lookup_frame, ci_idx);
            if (ret != MIX_RESULT_SUCCESS) {
                LOG_E("mix_videoframe_set_ci_frame_idx failed\n");
                goto cleanup;
            }


            ret = mix_surfacepool_get_frame_with_ci_frameidx
                  (this->surfacepool, &this->cur_frame, this->lookup_frame);

            if (ret != MIX_RESULT_SUCCESS) {
                LOG_E("get current working surface from pool failed\n");
                goto cleanup;
            }
        }

        ret = mix_videoframe_get_frame_id(this->cur_frame, &surface);
    }

    /*
    * end of CI buffer allocation mode
    */
    break;
    case MIX_BUFFER_UPSTREAM_ALLOC_V4L2:
        break;
    case MIX_BUFFER_UPSTREAM_ALLOC_SURFACE:
        break;
    case MIX_BUFFER_SELF_ALLOC_SURFACE:
    {
        if (this->lookup_frame == NULL) {
            this->lookup_frame = mix_videoframe_new ();
            if (this->lookup_frame == NULL) {
                LOG_E("mix_videoframe_new() failed!\n");
                ret = MIX_RESULT_NO_MEMORY;
                goto cleanup;
            }
        }

        LOG_I("bufin->data = 0x%08x\n", bufin->data);
        for (idx = 0; idx < this->alloc_surface_cnt; idx++) {
            LOG_I ("mix->usrptr[%d] = 0x%08x\n", idx, this->usrptr[idx]);
            if (bufin->data == this->usrptr[idx])
                surface_idx = idx;
        }

        LOG_I("surface_num = %d\n", this->surface_num);
        LOG_I("surface_idx = %d\n", surface_idx);

        if (surface_idx > this->surface_num - 2) {
            LOG_W("the Surface idx is too big, most likely the buffer passed in is not allocated by us\n");
            ret = MIX_RESULT_FAIL;
            goto no_share_mode;
        }

        if (this->ref_frame == NULL) {
            ret = mix_videoframe_set_ci_frame_idx (this->lookup_frame, this->surface_num - 1);
            if (ret != MIX_RESULT_SUCCESS) {
                LOG_E("mix_videoframe_set_ci_frame_idx failed\n");
                goto cleanup;
            }

            ret = mix_surfacepool_get_frame_with_ci_frameidx
                  (this->surfacepool, &this->ref_frame, this->lookup_frame);
            if (ret != MIX_RESULT_SUCCESS) {
                LOG_E("get reference surface from pool failed\n");
                goto cleanup;
            }
        }

        if (this->rec_frame == NULL) {
            ret = mix_videoframe_set_ci_frame_idx (this->lookup_frame, this->surface_num - 2);
            if (ret != MIX_RESULT_SUCCESS) {
                LOG_E("mix_videoframe_set_ci_frame_idx failed\n");
                goto cleanup;
            }

            ret = mix_surfacepool_get_frame_with_ci_frameidx
                  (this->surfacepool, &this->rec_frame, this->lookup_frame);

            if (ret != MIX_RESULT_SUCCESS) {
                LOG_E("get recontructed surface from pool failed\n");
                goto cleanup;
            }
        }

        //mix_videoframe_unref (this->cur_frame);

        if (this->need_display) {
            this->cur_frame = NULL;
        }

        if (this->cur_frame == NULL) {
            ret = mix_videoframe_set_ci_frame_idx (this->lookup_frame, surface_idx);
            if (ret != MIX_RESULT_SUCCESS) {
                LOG_E("mix_videoframe_set_ci_frame_idx failed\n");
                goto cleanup;
            }

            ret = mix_surfacepool_get_frame_with_ci_frameidx
                  (this->surfacepool, &this->cur_frame, this->lookup_frame);

            if (ret != MIX_RESULT_SUCCESS) {
                LOG_E("get current working surface from pool failed\n");
                goto cleanup;
            }
        }

        ret = mix_videoframe_get_frame_id(this->cur_frame, &surface);
    }

    break;
    /*
    * end of Self buffer allocation mode
    */
    case MIX_BUFFER_ALLOC_NORMAL:
    {

no_share_mode:

        LOG_V("We are NOT in share buffer mode\n");
        if (this->ref_frame == NULL) {
            ret = mix_surfacepool_get(this->surfacepool, &this->ref_frame);
            if (ret != MIX_RESULT_SUCCESS) {//#ifdef SLEEP_SURFACE not used
                LOG_E("Failed to mix_surfacepool_get\n");
                goto cleanup;
            }
        }

        if (this->rec_frame == NULL) {
            ret = mix_surfacepool_get(this->surfacepool, &this->rec_frame);
            if (ret != MIX_RESULT_SUCCESS) {
                LOG_E("Failed to mix_surfacepool_get\n");
                goto cleanup;
            }
        }

        if (this->need_display) {
            this->cur_frame = NULL;
        }

        if (this->cur_frame == NULL) {
            ret = mix_surfacepool_get(this->surfacepool, &this->cur_frame);
            if (ret != MIX_RESULT_SUCCESS) {
                LOG_E("Failed to mix_surfacepool_get\n");
                goto cleanup;
            }
        }

        LOG_V( "Get Surface Done\n");
        LOG_V("map source data to surface\n");
        ret = mix_videoframe_get_frame_id(this->cur_frame, &surface);
        if (ret != MIX_RESULT_SUCCESS) {
            LOG_E("Failed to mix_videoframe_get_frame_id\n");
            goto cleanup;
        }

        LOG_I("surface id = 0x%08x\n", (uint) surface);
        va_status = vaDeriveImage(va_display, surface, &src_image);
        //need to destroy
        if (va_status != VA_STATUS_SUCCESS) {
            LOG_E("Failed to vaDeriveImage\n");
            ret = MIX_RESULT_FAIL;
            goto cleanup;
        }

        VAImage *image = &src_image;
        LOG_V( "vaDeriveImage Done\n");
        va_status = vaMapBuffer (va_display, image->buf, (void **)&pvbuf);
        if (va_status != VA_STATUS_SUCCESS) {
            LOG_E( "Failed to vaMapBuffer\n");
            ret = MIX_RESULT_FAIL;
            goto cleanup;
        }
        LOG_V("vaImage information\n");
        LOG_I("image->pitches[0] = %d\n", image->pitches[0]);
        LOG_I("image->pitches[1] = %d\n", image->pitches[1]);
        LOG_I("image->offsets[0] = %d\n", image->offsets[0]);
        LOG_I("image->offsets[1] = %d\n", image->offsets[1]);
        LOG_I("image->num_planes = %d\n", image->num_planes);
        LOG_I("image->width = %d\n", image->width);
        LOG_I("image->height = %d\n", image->height);
        LOG_I("input buf size = %d\n", bufin->size);
        uint8 *inbuf = bufin->data;

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
                dst_uv [j + 1] = inbuf [width * height * 5 / 4 + i * width / 2 + j / 2];
            }
            dst_uv += image->pitches[1];
        }
#else //USE_SRC_FMT_NV12 or USE_SRC_FMT_NV21
        int offset_uv = width * height;
        uint8 *inbuf_uv = inbuf + offset_uv;
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
        if (va_status != VA_STATUS_SUCCESS) {
            LOG_E("Failed to vaUnmapBuffer\n");
            ret = MIX_RESULT_FAIL;
            goto cleanup;
        }

        va_status = vaDestroyImage(va_display, src_image.image_id);
        if (va_status != VA_STATUS_SUCCESS) {
            LOG_E("Failed to vaDestroyImage\n");
            ret = MIX_RESULT_FAIL;
            goto cleanup;
        }

        LOG_V("Map source data to surface done\n");
    }
    break;
    default:
        break;

    }

    LOG_V( "vaBeginPicture\n");
    LOG_I( "va_context = 0x%08x\n",(uint)va_context);
    LOG_I( "surface = 0x%08x\n",(uint)surface);
    LOG_I( "va_display = 0x%08x\n",(uint)va_display);

    iovout->data_size = 4;
    iovout->data = new uchar[iovout->data_size];
    if (iovout->data == NULL) {
        ret = MIX_RESULT_NO_MEMORY;
        goto cleanup;
    }

    memset (iovout->data, 0, iovout->data_size);
    iovout->buffer_size = iovout->data_size;

    if (this->need_display) {
        ret = mix_videoframe_set_sync_flag(this->cur_frame, TRUE);
        if (ret != MIX_RESULT_SUCCESS) {
            LOG_E("Failed to set sync_flag\n");
            goto cleanup;
        }

        ret = mix_framemanager_enqueue(this->framemgr, this->cur_frame);
        if (ret != MIX_RESULT_SUCCESS) {
            LOG_E("Failed mix_framemanager_enqueue\n");
            goto cleanup;
        }
    }

    if (!(this->need_display)) {
        mix_videoframe_unref (this->cur_frame);
        this->cur_frame = NULL;
    }
    this->encoded_frames ++;

cleanup:

    if (ret != MIX_RESULT_SUCCESS) {
        if (iovout->data) {
            delete [] iovout->data;
            iovout->data = NULL;
        }
    }
    LOG_V( "end\n");
    return ret;
}
