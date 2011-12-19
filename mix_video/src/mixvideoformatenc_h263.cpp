/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */

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

MixVideoFormatEnc_H263::MixVideoFormatEnc_H263()
        :encoded_frames(0)
        ,pic_skipped(FALSE)
        ,is_intra(TRUE)
        ,cur_frame(NULL)
        ,ref_frame(NULL)
        ,rec_frame(NULL)
        ,lookup_frame(NULL)
        ,last_mix_buffer(NULL)
        ,shared_surfaces(NULL)
        ,surfaces(NULL)
        ,surface_num(0)
        ,shared_surfaces_cnt(0)
        ,precreated_surfaces_cnt(0)
        ,usrptr(NULL)
        ,coded_buf_index(0)
        ,coded_buf_size(0) {
}

MixVideoFormatEnc_H263::~MixVideoFormatEnc_H263() {
}


MixVideoFormatEnc_H263 *
mix_videoformatenc_h263_new(void) {
    return new MixVideoFormatEnc_H263();
}


MixVideoFormatEnc_H263 *
mix_videoformatenc_h263_ref(MixVideoFormatEnc_H263 * mix) {
    if (NULL != mix)
        mix->Ref();
    return mix;
}

MixVideoFormatEnc_H263 *
mix_videoformatenc_h263_unref(MixVideoFormatEnc_H263 * mix) {
    if (NULL != mix)
        return MIX_VIDEOFORMATENC_H263(mix->Unref());
    else
        return mix;
}

MIX_RESULT MixVideoFormatEnc_H263::Initialize(
    MixVideoConfigParamsEnc* config_params_enc,
    MixFrameManager * frame_mgr,
    MixBufferPool * input_buf_pool,
    MixSurfacePool ** surface_pool,
    MixUsrReqSurfacesInfo * requested_surface_info,
    VADisplay va_display) {
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    MixVideoConfigParamsEncH263 * config_params_enc_h263;

    VAStatus va_status = VA_STATUS_SUCCESS;
    VASurfaceID * surfaces = NULL;

    int va_max_num_profiles, va_max_num_entrypoints, va_max_num_attribs;
    int va_num_profiles,  va_num_entrypoints;

    VAProfile *va_profiles = NULL;
    VAEntrypoint *va_entrypoints = NULL;
    VAConfigAttrib va_attrib[2];
    uint index;
    uint max_size = 0;

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

    if ( config_params_enc == NULL || va_display == NULL || requested_surface_info == NULL) {
        LOG_E(
            " config_params_enc == NULL || va_display == NULL || requested_surface_info == NULL\n");
        return MIX_RESULT_NULL_PTR;
    }


    /*
    * Check more for requested_surface_info
    */
    if (requested_surface_info->surface_cnt != 0 &&
            (requested_surface_info->surface_allocated == NULL || requested_surface_info->usrptr == NULL)) {
        LOG_E(
            "surface_cnt != 0 && (surface_allocated == NULL || usrptr == NULL)\n");
        return MIX_RESULT_NULL_PTR;
    }

    if (requested_surface_info->surface_cnt > MAX_ENC_SURFACE_COUNT) {
        LOG_E ("Something wrong, we have to quite now!\n");
        return MIX_RESULT_FAIL;
    }

    LOG_V( "begin\n");

    /* Chainup parent method. */
    ret = MixVideoFormatEnc::Initialize(config_params_enc, frame_mgr, input_buf_pool, surface_pool, requested_surface_info, va_display);


    if (ret != MIX_RESULT_SUCCESS)
    {
        return ret;
    }



    if (MIX_IS_VIDEOCONFIGPARAMSENC_H263 (config_params_enc)) {
        config_params_enc_h263 =
            MIX_VIDEOCONFIGPARAMSENC_H263 (config_params_enc);
    } else {
        LOG_V(
            "mix_videofmtenc_h263_initialize:  no h263 config params found\n");
        return MIX_RESULT_FAIL;
    }

//        g_mutex_lock(parent->objectlock);
    Lock();

    LOG_V(
        "Start to get properities from H263 params\n");

    /* get properties from H263 params Object, which is special to H263 format*/

    ret = mix_videoconfigparamsenc_h263_get_slice_num (config_params_enc_h263,
            &this->slice_num);

    if (ret != MIX_RESULT_SUCCESS) {
        LOG_E(
            "Failed to mix_videoconfigparamsenc_h263_get_slice_num\n");
        goto cleanup;
    }

    ret = mix_videoconfigparamsenc_h263_get_dlk (config_params_enc_h263,
            &(this->disable_deblocking_filter_idc));

    if (ret != MIX_RESULT_SUCCESS) {
        LOG_E(
            "Failed to mix_videoconfigparamsenc_h263_get_dlk\n");
        goto cleanup;
    }


    LOG_V(
        "======H263 Encode Object properities======:\n");

    LOG_I( "this->slice_num = %d\n",
           this->slice_num);
    LOG_I( "this->disabled_deblocking_filter_idc = %d\n\n",
           this->disable_deblocking_filter_idc);

    LOG_V(
        "Get properities from params done\n");

    this->va_display = va_display;

    LOG_V( "Get Display\n");
    LOG_I( "Display = 0x%08x\n",
           (uint)va_display);

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

//        va_profiles = g_malloc(sizeof(VAProfile)*va_max_num_profiles);
    va_profiles = new VAProfile[va_max_num_profiles];
//        va_entrypoints = g_malloc(sizeof(VAEntrypoint)*va_max_num_entrypoints);
    va_entrypoints = new VAEntrypoint[va_max_num_entrypoints];

    if (va_profiles == NULL || va_entrypoints ==NULL)
    {
        LOG_E(
            "!va_profiles || !va_entrypoints\n");
        ret = MIX_RESULT_NO_MEMORY;
        goto cleanup;
    }

    LOG_I(
        "va_profiles = 0x%08x\n", (uint)va_profiles);

    LOG_V( "vaQueryConfigProfiles\n");


    va_status = vaQueryConfigProfiles (va_display, va_profiles, &va_num_profiles);

    if (va_status != VA_STATUS_SUCCESS)
    {
        LOG_E(
            "Failed to call vaQueryConfigProfiles\n");
        ret = MIX_RESULT_FAIL;
        goto cleanup;
    }

    LOG_V( "vaQueryConfigProfiles Done\n");



    /*check whether profile is supported*/
    for (index= 0; index < va_num_profiles; index++) {
        if (this->va_profile == va_profiles[index])
            break;
    }

    if (index == va_num_profiles)
    {
        LOG_E( "Profile not supported\n");
        ret = MIX_RESULT_FAIL;
        goto cleanup;
    }

    LOG_V( "vaQueryConfigEntrypoints\n");


    /*Check entry point*/
    va_status = vaQueryConfigEntrypoints(va_display,
                                         this->va_profile,
                                         va_entrypoints, &va_num_entrypoints);

    if (va_status != VA_STATUS_SUCCESS)
    {
        LOG_E(
            "Failed to call vaQueryConfigEntrypoints\n");
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

    va_status = vaGetConfigAttributes(va_display, this->va_profile,
                                      this->va_entrypoint,
                                      &va_attrib[0], 2);

    if (va_status != VA_STATUS_SUCCESS)
    {
        LOG_E(
            "Failed to call vaGetConfigAttributes\n");
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

    LOG_I( "profile = %d\n",
           this->va_profile);
    LOG_I( "va_entrypoint = %d\n",
           this->va_entrypoint);
    LOG_I( "va_attrib[0].type = %d\n",
           va_attrib[0].type);
    LOG_I( "va_attrib[1].type = %d\n",
           va_attrib[1].type);
    LOG_I( "va_attrib[0].value (Format) = %d\n",
           va_attrib[0].value);
    LOG_I( "va_attrib[1].value (RC mode) = %d\n",
           va_attrib[1].value);

    LOG_V( "vaCreateConfig\n");

    va_status = vaCreateConfig(va_display, this->va_profile,
                               this->va_entrypoint,
                               &va_attrib[0], 2, &(this->va_config));

    if (va_status != VA_STATUS_SUCCESS)
    {
        LOG_E( "Failed vaCreateConfig\n");
        ret = MIX_RESULT_FAIL;
        goto cleanup;
    }

    if (this->va_rcmode == VA_RC_VCM) {

        /*
        * Following three features are only enabled in VCM mode
        */
        this->render_mss_required = TRUE;
        this->render_AIR_required = TRUE;
        this->render_bitrate_required = TRUE;
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

//		self->usrptr = g_malloc (requested_surface_info->surface_cnt * sizeof (uint8 *));
        this->usrptr = new uint8 *[requested_surface_info->surface_cnt] ;
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

    //surfaces = g_malloc(sizeof(VASurfaceID)*normal_surfaces_cnt);
    surfaces = new VASurfaceID[normal_surfaces_cnt] ;
    if (surfaces == NULL)
    {
        LOG_E(
            "Failed allocate surface\n");
        ret = MIX_RESULT_NO_MEMORY;
        goto cleanup;
    }

    //self->surfaces = g_malloc(sizeof(VASurfaceID) * self->surface_num);
    this->surfaces = new VASurfaceID[this->surface_num] ;
    if (this->surfaces == NULL)
    {
        LOG_E(
            "Failed allocate private surface\n");
        ret = MIX_RESULT_NO_MEMORY;
        goto cleanup;
    }

    LOG_V( "vaCreateSurfaces\n");

    va_status = vaCreateSurfaces(va_display, this->picture_width,
                                 this->picture_height, this->va_format,
                                 normal_surfaces_cnt, surfaces, NULL, 0);

    if (va_status != VA_STATUS_SUCCESS)
    {
        LOG_E(
            "Failed vaCreateSurfaces\n");
        ret = MIX_RESULT_FAIL;
        goto cleanup;
    }


    if (shared_surfaces_cnt != 0) {
//        self->shared_surfaces =g_malloc(sizeof(VASurfaceID) * shared_surfaces_cnt);
        this->shared_surfaces =new VASurfaceID[shared_surfaces_cnt] ;
        if (this->shared_surfaces == NULL)
        {
            LOG_E(
                "Failed allocate shared surface\n");

            ret = MIX_RESULT_NO_MEMORY;
            goto cleanup;
        }
    }


    switch (this->buffer_mode) {
    case MIX_BUFFER_UPSTREAM_ALLOC_CI:
    {
        for (index = 0; index < this->shared_surfaces_cnt; index++) {

            va_status = vaCreateSurfaceFromCIFrame(va_display,
                                                   (ulong) (ci_info->ci_frame_id[index]),
                                                   &this->shared_surfaces[index]);
            if (va_status != VA_STATUS_SUCCESS)
            {
                LOG_E("Failed to vaCreateSurfaceFromCIFrame\n");
                ret = MIX_RESULT_FAIL;
                goto cleanup;
            }

            this->surfaces[index] = this->shared_surfaces[index];
        }
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

    if (this->surfacepool == NULL)
    {
        LOG_E(
            "Failed to mix_surfacepool_new\n");
        ret = MIX_RESULT_FAIL;
        goto cleanup;
    }

    LOG_V(
        "mix_surfacepool_initialize\n");

    ret = mix_surfacepool_initialize(this->surfacepool,
                                     this->surfaces, this->surface_num, va_display);

    switch (ret)
    {
    case MIX_RESULT_SUCCESS:
        break;
    case MIX_RESULT_ALREADY_INIT:

        LOG_E("Error init failure\n");

        ret = MIX_RESULT_ALREADY_INIT;
        goto cleanup;
    default:
        break;
    }


    //Initialize and save the VA context ID
    LOG_V( "vaCreateContext\n");

    va_status = vaCreateContext(va_display, this->va_config,
                                this->picture_width, this->picture_height,
                                VA_PROGRESSIVE, this->surfaces, this->surface_num,
                                &(this->va_context));

    LOG_I(
        "Created libva context width %d, height %d\n",
        this->picture_width, this->picture_height);

    if (va_status != VA_STATUS_SUCCESS)
    {
        LOG_E(
            "Failed to vaCreateContext\n");
        LOG_I( "va_status = %d\n",
               (uint)va_status);
        ret = MIX_RESULT_FAIL;
        goto cleanup;
    }


    ret = GetMaxEncodedBufSize(&max_size);
    if (ret != MIX_RESULT_SUCCESS)
    {
        LOG_E(
            "Failed to mix_videofmtenc_h263_get_max_encoded_buf_size\n");
        ret = MIX_RESULT_FAIL;
        goto cleanup;
    }

    /*Create coded buffer for output*/
    va_status = vaCreateBuffer (va_display, this->va_context,
                                VAEncCodedBufferType,
                                this->coded_buf_size,  //
                                1, NULL,
                                &this->coded_buf[0]);

    if (va_status != VA_STATUS_SUCCESS)
    {
        LOG_E(
            "Failed to vaCreateBuffer: VAEncCodedBufferType\n");
        ret = MIX_RESULT_FAIL;
        goto cleanup;
    }


    /*Create coded buffer for output*/
    va_status = vaCreateBuffer (va_display, this->va_context,
                                VAEncCodedBufferType,
                                this->coded_buf_size,  //
                                1, NULL,
                                &(this->coded_buf[1]));

    if (va_status != VA_STATUS_SUCCESS)
    {
        LOG_E(
            "Failed to vaCreateBuffer: VAEncCodedBufferType\n");
        ret = MIX_RESULT_FAIL;
        goto cleanup;
    }

#ifdef SHOW_SRC
    Display * display = XOpenDisplay (NULL);

    LOG_I( "display = 0x%08x\n",
           (uint) display);
    win = XCreateSimpleWindow(display, RootWindow(display, 0), 0, 0,
                              parent->picture_width,  parent->picture_height, 0, 0,
                              WhitePixel(display, 0));
    XMapWindow(display, win);
    XSelectInput(display, win, KeyPressMask | StructureNotifyMask);

    XSync(display, False);
    LOG_I( "va_display = 0x%08x\n",
           (uint) va_display);

#endif /* SHOW_SRC */

cleanup:

    if (ret == MIX_RESULT_SUCCESS) {
        this->initialized = TRUE;
    }

    /*free profiles and entrypoints*/
    if (va_profiles)
//           g_free(va_profiles);
        delete[]va_profiles;
    if (va_entrypoints)
//            g_free(va_entrypoints);
        delete[]va_entrypoints;
    if (surfaces)
//            g_free(surfaces);
        delete[]surfaces;

//        g_mutex_unlock(parent->objectlock);
    Unlock();

    LOG_V( "end\n");

    return ret;

}

MIX_RESULT MixVideoFormatEnc_H263::Encode(
    MixBuffer * bufin[], int bufincnt,
    MixIOVec * iovout[], int iovoutcnt,
    MixVideoEncodeParams * encode_params) {
    MIX_RESULT ret = MIX_RESULT_SUCCESS;

    LOG_V( "Begin\n");

    /*currenly only support one input and output buffer*/

    if (bufincnt != 1 || iovoutcnt != 1) {
        LOG_E(
            "buffer count not equel to 1\n");
        LOG_E(
            "maybe some exception occurs\n");
    }

    if (bufin[0] == NULL ||  iovout[0] == NULL) {
        LOG_E(
            "!bufin[0] ||!iovout[0]\n");
        return MIX_RESULT_NULL_PTR;
    }

#if 0
    if (parent_class->encode) {
        return parent_class->encode(mix, bufin, bufincnt, iovout,
                                    iovoutcnt, encode_params);
    }
#endif


    LOG_V( "Locking\n");
//    g_mutex_lock(parent->objectlock);
    Lock();


    //TODO: also we could move some encode Preparation work to here

    LOG_V(
        "mix_videofmtenc_h263_process_encode\n");

    ret = _process_encode(bufin[0], iovout[0]);
    if (ret != MIX_RESULT_SUCCESS)
    {
        LOG_E(
            "Failed mix_videofmtenc_h263_process_encode\n");
        goto cleanup;
    }

cleanup:

    LOG_V( "UnLocking\n");

//    g_mutex_unlock(parent->objectlock);
    Unlock();

    LOG_V( "end\n");

    return ret;

}

MIX_RESULT MixVideoFormatEnc_H263::Flush() {
    //MIX_RESULT ret = MIX_RESULT_SUCCESS;

    LOG_V( "Begin\n");

    /*not chain to parent flush func*/
#if 0
    if (parent_class->flush) {
        return parent_class->flush(mix, msg);
    }
#endif

//   g_mutex_lock(mix->objectlock);
    Lock();

    /*unref the current source surface*/
    if (this->cur_frame != NULL)
    {
        mix_videoframe_unref (this->cur_frame);
        this->cur_frame = NULL;
    }

    /*unref the reconstructed surface*/
    if (this->rec_frame != NULL)
    {
        mix_videoframe_unref (this->rec_frame);
        this->rec_frame = NULL;
    }

    /*unref the reference surface*/
    if (this->ref_frame != NULL)
    {
        mix_videoframe_unref (this->ref_frame);
        this->ref_frame = NULL;
    }

    if (this->last_mix_buffer) {
        mix_buffer_unref(this->last_mix_buffer);
        this->last_mix_buffer = NULL;
    }

    /*reset the properities*/
    this->encoded_frames = 0;
    this->pic_skipped = FALSE;
    this->is_intra = TRUE;

//    g_mutex_unlock(mix->objectlock);
    Unlock();

    LOG_V( "end\n");

    return MIX_RESULT_SUCCESS;
}

MIX_RESULT MixVideoFormatEnc_H263::Deinitialize() {
    VAStatus va_status;
    MIX_RESULT ret = MIX_RESULT_SUCCESS;

    LOG_V( "Begin\n");

    MixVideoFormatEnc::Deinitialize();

    if (ret != MIX_RESULT_SUCCESS)
    {
        return ret;
    }


    LOG_V( "Release frames\n");

//    g_mutex_lock(parent->objectlock);
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
    if (this->rec_frame != NULL)
    {
        mix_videoframe_unref (this->rec_frame);
        this->rec_frame = NULL;
    }

    /*unref the reference surface*/
    if (this->ref_frame != NULL)
    {
        mix_videoframe_unref (this->ref_frame);
        this->ref_frame = NULL;
    }

    if (this->lookup_frame != NULL)
    {
        mix_videoframe_unref (this->lookup_frame);
        this->lookup_frame = NULL;
    }

    if (this->last_mix_buffer) {
        mix_buffer_unref(this->last_mix_buffer);
        this->last_mix_buffer = NULL;
    }

    LOG_V( "Release surfaces\n");

    if (this->shared_surfaces)
    {
//        g_free (self->shared_surfaces);
        delete[]this->shared_surfaces;
        this->shared_surfaces = NULL;
    }

    if (this->surfaces)
    {
//        g_free (self->surfaces);
        delete[]this->surfaces;
        this->surfaces = NULL;
    }

    if (this->usrptr) {
//	g_free (self->usrptr);
        delete[]this->usrptr;
        this->usrptr = NULL;
    }

    LOG_V( "vaDestroyContext\n");

    va_status = vaDestroyContext (this->va_display, this->va_context);
    if (va_status != VA_STATUS_SUCCESS)
    {
        LOG_E(
            "Failed vaDestroyContext\n");
        ret = MIX_RESULT_FAIL;
        goto cleanup;
    }

    LOG_V( "vaDestroyConfig\n");

    va_status = vaDestroyConfig (this->va_display, this->va_config);
    if (va_status != VA_STATUS_SUCCESS)
    {
        LOG_E(
            "Failed vaDestroyConfig\n");
        ret = MIX_RESULT_FAIL;
        goto cleanup;
    }

cleanup:

    this->initialized = TRUE;

//    g_mutex_unlock(parent->objectlock);
    Unlock();

    LOG_V( "end\n");

    return ret;
}

MIX_RESULT MixVideoFormatEnc_H263::GetMaxEncodedBufSize (uint *max_size) {


    LOG_V( "Begin\n");

    if (MIX_IS_VIDEOFORMATENC_H263(this)) {

        if (this->coded_buf_size > 0) {
            *max_size = this->coded_buf_size;
            LOG_V ("Already calculate the max encoded size, get the value directly");
            return MIX_RESULT_SUCCESS;
        }

        /*base on the rate control mode to calculate the defaule encoded buffer size*/
        if (this->va_rcmode_h263 == VA_RC_NONE) {
            this->coded_buf_size =
                (this->picture_width* this->picture_height * 830) / (16 * 16);
            // set to value according to QP
        }
        else {
            this->coded_buf_size = this->bitrate/ 4;
        }

        this->coded_buf_size =
            max (this->coded_buf_size ,
                 (this->picture_width* this->picture_height * 830) / (16 * 16));

        /*in case got a very large user input bit rate value*/
        this->coded_buf_size =
            max(this->coded_buf_size,
                (this->picture_width * this->picture_height * 1.5 * 8));
        this->coded_buf_size =  (this->coded_buf_size + 15) &(~15);
    }
    else
    {
        LOG_E(
            "not H263 video encode Object\n");
        return MIX_RESULT_INVALID_PARAM;
    }

    *max_size = this->coded_buf_size;

    LOG_V( "end\n");

    return MIX_RESULT_SUCCESS;

}

MIX_RESULT MixVideoFormatEnc_H263::_process_encode (MixBuffer * bufin,
        MixIOVec * iovout) {
    MIX_RESULT ret = MIX_RESULT_SUCCESS;
    VAStatus va_status = VA_STATUS_SUCCESS;
    VADisplay va_display = NULL;
    VAContextID va_context;
    ulong surface = 0;
    uint16 width, height;

    MixVideoFrame *  tmp_frame;
    uint8 *buf;
    VACodedBufferSegment *coded_seg = NULL;
    int num_seg = 0;
    uint total_size = 0;
    uint size = 0;

    if ((bufin == NULL) || (iovout == NULL)) {
        LOG_E(
            "bufin == NULL || iovout == NULL\n");
        return MIX_RESULT_NULL_PTR;
    }

    LOG_V( "Begin\n");


    va_display = this->va_display;
    va_context = this->va_context;
    width = this->picture_width;
    height = this->picture_height;


    LOG_I( "encoded_frames = %d\n",
           this->encoded_frames);
    LOG_I( "is_intra = %d\n",
           this->is_intra);
    LOG_I( "ci_frame_id = 0x%08x\n",
           (uint) this->ci_frame_id);

    /* determine the picture type*/
    if ((this->encoded_frames % this->intra_period) == 0) {
        this->is_intra = TRUE;
    } else {
        this->is_intra = FALSE;
    }

    LOG_I( "is_intra_picture = %d\n",
           this->is_intra);

    LOG_V(
        "Get Surface from the pool\n");

    /*current we use one surface for source data,
    * one for reference and one for reconstructed*/
    /*TODO, could be refine here*/



    switch (this->buffer_mode) {
    case MIX_BUFFER_UPSTREAM_ALLOC_CI:
    {


        //MixVideoFrame * frame = mix_videoframe_new();
        if (this->lookup_frame == NULL)
        {
            this->lookup_frame = mix_videoframe_new ();
            if (this->lookup_frame == NULL)
            {
                LOG_E("mix_videoframe_new() failed!\n");
                ret = MIX_RESULT_NO_MEMORY;
                goto cleanup;
            }
        }

        if (this->ref_frame == NULL)
        {
            ret = mix_videoframe_set_ci_frame_idx (this->lookup_frame, this->surface_num - 1);
            if (ret != MIX_RESULT_SUCCESS)
            {
                LOG_E(
                    "mix_videoframe_set_ci_frame_idx failed\n");
                goto cleanup;
            }

            ret = mix_surfacepool_get_frame_with_ci_frameidx
                  (this->surfacepool, &this->ref_frame, this->lookup_frame);
            if (ret != MIX_RESULT_SUCCESS)
            {
                LOG_E(
                    "get reference surface from pool failed\n");
                goto cleanup;
            }
        }

        if (this->rec_frame == NULL)
        {
            ret = mix_videoframe_set_ci_frame_idx (this->lookup_frame, this->surface_num - 2);
            if (ret != MIX_RESULT_SUCCESS)
            {
                LOG_E(
                    "mix_videoframe_set_ci_frame_idx failed\n");
                goto cleanup;
            }

            ret = mix_surfacepool_get_frame_with_ci_frameidx
                  (this->surfacepool, &this->rec_frame, this->lookup_frame);

            if (ret != MIX_RESULT_SUCCESS)
            {
                LOG_E(
                    "get recontructed surface from pool failed\n");
                goto cleanup;
            }
        }

        //mix_videoframe_unref (mix->cur_frame);

        if (this->need_display) {
            this->cur_frame = NULL;
        }

        if (this->cur_frame == NULL)
        {
            uint ci_idx;
#ifndef ANDROID
            memcpy (&ci_idx, bufin->data, bufin->size);
#else
            memcpy (&ci_idx, bufin->data, sizeof(unsigned int));
#endif

            LOG_I(
                "surface_num = %d\n", this->surface_num);
            LOG_I(
                "ci_frame_idx = %d\n", ci_idx);

            if (ci_idx > this->surface_num - 2) {
                LOG_E(
                    "the CI frame idx is too bigger than CI frame number\n");
                ret = MIX_RESULT_FAIL;
                goto cleanup;

            }


            ret = mix_videoframe_set_ci_frame_idx (this->lookup_frame, ci_idx);
            if (ret != MIX_RESULT_SUCCESS)
            {
                LOG_E(
                    "mix_videoframe_set_ci_frame_idx failed\n");
                goto cleanup;
            }


            ret = mix_surfacepool_get_frame_with_ci_frameidx
                  (this->surfacepool, &this->cur_frame, this->lookup_frame);

            if (ret != MIX_RESULT_SUCCESS)
            {
                LOG_E(
                    "get current working surface from pool failed\n");
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
        if (this->lookup_frame == NULL)
        {
            this->lookup_frame = mix_videoframe_new ();
            if (this->lookup_frame == NULL)
            {
                LOG_E("mix_videoframe_new() failed!\n");
                ret = MIX_RESULT_NO_MEMORY;
                goto cleanup;
            }
        }

        uint surface_idx = (uint) -1; //fixme, temp use a big value
        uint idx = 0;

        LOG_I ("bufin->data = 0x%08x\n", bufin->data);

        for (idx = 0; idx < this->alloc_surface_cnt; idx++) {

            LOG_I ("this->usrptr[%d] = 0x%08x\n", idx, this->usrptr[idx]);

            if (bufin->data == this->usrptr[idx])
                surface_idx = idx;
        }

        LOG_I(
            "surface_num = %d\n", this->surface_num);
        LOG_I(
            "surface_idx = %d\n", surface_idx);

        if (surface_idx > this->surface_num - 2) {
            LOG_W(
                "the Surface idx is too big, most likely the buffer passed in is not allocated by us\n");
            ret = MIX_RESULT_FAIL;
            goto no_share_mode;

        }

        if (this->ref_frame == NULL)
        {
            ret = mix_videoframe_set_ci_frame_idx (this->lookup_frame, this->surface_num - 1);
            if (ret != MIX_RESULT_SUCCESS)
            {
                LOG_E(
                    "mix_videoframe_set_ci_frame_idx failed\n");
                goto cleanup;
            }

            ret = mix_surfacepool_get_frame_with_ci_frameidx
                  (this->surfacepool, &this->ref_frame, this->lookup_frame);
            if (ret != MIX_RESULT_SUCCESS)
            {
                LOG_E(
                    "get reference surface from pool failed\n");
                goto cleanup;
            }
        }

        if (this->rec_frame == NULL)
        {
            ret = mix_videoframe_set_ci_frame_idx (this->lookup_frame, this->surface_num - 2);
            if (ret != MIX_RESULT_SUCCESS)
            {
                LOG_E(
                    "mix_videoframe_set_ci_frame_idx failed\n");
                goto cleanup;
            }

            ret = mix_surfacepool_get_frame_with_ci_frameidx
                  (this->surfacepool, &this->rec_frame, this->lookup_frame);

            if (ret != MIX_RESULT_SUCCESS)
            {
                LOG_E(
                    "get recontructed surface from pool failed\n");
                goto cleanup;
            }
        }

        //mix_videoframe_unref (mix->cur_frame);

        if (this->need_display) {
            this->cur_frame = NULL;
        }

        if (this->cur_frame == NULL)
        {

            ret = mix_videoframe_set_ci_frame_idx (this->lookup_frame, surface_idx);
            if (ret != MIX_RESULT_SUCCESS)
            {
                LOG_E(
                    "mix_videoframe_set_ci_frame_idx failed\n");
                goto cleanup;
            }


            ret = mix_surfacepool_get_frame_with_ci_frameidx
                  (this->surfacepool, &this->cur_frame, this->lookup_frame);

            if (ret != MIX_RESULT_SUCCESS)
            {
                LOG_E(
                    "get current working surface from pool failed\n");
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

        LOG_V(
            "We are NOT in share buffer mode\n");

        if (this->ref_frame == NULL)
        {
            ret = mix_surfacepool_get(this->surfacepool, &this->ref_frame);
            if (ret != MIX_RESULT_SUCCESS)  //#ifdef SLEEP_SURFACE not used
            {
                LOG_E(
                    "Failed to mix_surfacepool_get\n");
                goto cleanup;
            }
        }

        if (this->rec_frame == NULL)
        {
            ret = mix_surfacepool_get(this->surfacepool, &this->rec_frame);
            if (ret != MIX_RESULT_SUCCESS)
            {
                LOG_E(
                    "Failed to mix_surfacepool_get\n");
                goto cleanup;
            }
        }

        if (this->need_display) {
            this->cur_frame = NULL;
        }

        if (this->cur_frame == NULL)
        {
            ret = mix_surfacepool_get(this->surfacepool, &this->cur_frame);
            if (ret != MIX_RESULT_SUCCESS)
            {
                LOG_E(
                    "Failed to mix_surfacepool_get\n");
                goto cleanup;
            }
        }

        LOG_V( "Get Surface Done\n");


        VAImage src_image;
        uint8 *pvbuf;
        uint8 *dst_y;
        uint8 *dst_uv;
        int i,j;

        LOG_V(
            "map source data to surface\n");

        ret = mix_videoframe_get_frame_id(this->cur_frame, &surface);
        if (ret != MIX_RESULT_SUCCESS)
        {
            LOG_E(
                "Failed to mix_videoframe_get_frame_id\n");
            goto cleanup;
        }


        LOG_I(
            "surface id = 0x%08x\n", (uint) surface);

        va_status = vaDeriveImage(va_display, surface, &src_image);
        //need to destroy

        if (va_status != VA_STATUS_SUCCESS)
        {
            LOG_E(
                "Failed to vaDeriveImage\n");
            ret = MIX_RESULT_FAIL;
            goto cleanup;
        }

        VAImage *image = &src_image;

        LOG_V( "vaDeriveImage Done\n");


        va_status = vaMapBuffer (va_display, image->buf, (void **)&pvbuf);
        if (va_status != VA_STATUS_SUCCESS)
        {
            LOG_E( "Failed to vaMapBuffer\n");
            ret = MIX_RESULT_FAIL;
            goto cleanup;
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

        uint8 *inbuf = bufin->data;

#ifdef ANDROID
#define USE_SRC_FMT_NV12
#endif
        int offset_uv = width * height;
        uint8 *inbuf_uv = inbuf + offset_uv;
        int height_uv = height / 2;
        int width_uv = width;

#ifdef ANDROID
        //USE_SRC_FMT_NV12 or USE_SRC_FMT_NV21

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

#else

        if (this->raw_format == MIX_RAW_TARGET_FORMAT_YUV420) {
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
        }

        else if (this->raw_format == MIX_RAW_TARGET_FORMAT_NV12) {

            dst_y = pvbuf + image->offsets[0];
            for (i = 0; i < height; i++) {
                memcpy (dst_y, inbuf + i * width, width);
                dst_y += image->pitches[0];
            }

            dst_uv = pvbuf + image->offsets[1];
            for (i = 0; i < height_uv; i++) {
                memcpy(dst_uv, inbuf_uv + i * width_uv, width_uv);
                dst_uv += image->pitches[1];
            }
        }
        else {
            LOG_E("Raw format not supoort\n");
            ret = MIX_RESULT_FAIL;
            goto cleanup;
        }

#endif //USE_SRC_FMT_YUV420

        va_status = vaUnmapBuffer(va_display, image->buf);
        if (va_status != VA_STATUS_SUCCESS)
        {
            LOG_E(
                "Failed to vaUnmapBuffer\n");
            ret = MIX_RESULT_FAIL;
            goto cleanup;
        }

        va_status = vaDestroyImage(va_display, src_image.image_id);
        if (va_status != VA_STATUS_SUCCESS)
        {
            LOG_E(
                "Failed to vaDestroyImage\n");
            ret = MIX_RESULT_FAIL;
            goto cleanup;
        }

        LOG_V(
            "Map source data to surface done\n");
    }
    break;
    default:
        break;

    }

    LOG_V( "vaBeginPicture\n");
    LOG_I( "va_context = 0x%08x\n",(uint)va_context);
    LOG_I( "surface = 0x%08x\n",(uint)surface);
    LOG_I( "va_display = 0x%08x\n",(uint)va_display);


    va_status = vaBeginPicture(va_display, va_context, surface);
    if (va_status != VA_STATUS_SUCCESS)
    {
        LOG_E( "Failed vaBeginPicture\n");
        ret = MIX_RESULT_FAIL;
        goto cleanup;
    }

    ret = _send_encode_command ();
    if (ret != MIX_RESULT_SUCCESS)
    {
        LOG_E (
            "Failed mix_videofmtenc_h264_send_encode_command\n");
        goto cleanup;
    }


    if ((this->va_rcmode == VA_RC_NONE) || this->encoded_frames == 0) {

        va_status = vaEndPicture (va_display, va_context);
        if (va_status != VA_STATUS_SUCCESS)
        {
            LOG_E( "Failed vaEndPicture\n");
            ret = MIX_RESULT_FAIL;
            goto cleanup;
        }
    }

    if (this->encoded_frames == 0) {
        this->encoded_frames ++;
        this->last_coded_buf = this->coded_buf[this->coded_buf_index];
        this->coded_buf_index ++;
        this->coded_buf_index %=2;

        this->last_frame = this->cur_frame;


        /* determine the picture type*/
        if ((this->encoded_frames % this->intra_period) == 0) {
            this->is_intra = TRUE;
        } else {
            this->is_intra = FALSE;
        }

        tmp_frame = this->rec_frame;
        this->rec_frame= this->ref_frame;
        this->ref_frame = tmp_frame;


    }

    LOG_V( "vaSyncSurface\n");

    va_status = vaSyncSurface(va_display, this->last_frame->frame_id);
    if (va_status != VA_STATUS_SUCCESS)
    {
        LOG_E( "Failed vaSyncSurface\n");
        //return MIX_RESULT_FAIL;
    }


    LOG_V(
        "Start to get encoded data\n");

    /*get encoded data from the VA buffer*/
    va_status = vaMapBuffer (va_display, this->last_coded_buf, (void **)&buf);
    if (va_status != VA_STATUS_SUCCESS)
    {
        LOG_E( "Failed vaMapBuffer\n");
        ret = MIX_RESULT_FAIL;
        goto cleanup;
    }



    coded_seg = (VACodedBufferSegment *)buf;
    num_seg = 1;

    while (1) {
        total_size += coded_seg->size;

        if (coded_seg->next == NULL)
            break;

        coded_seg = (VACodedBufferSegment *)coded_seg->next;
        num_seg ++;
    }


#if 0
    // first 4 bytes is the size of the buffer
    memcpy (&(iovout->data_size), (void*)buf, 4);
    //size = (uint*) buf;
#endif

    iovout->data_size = total_size;

    if (iovout->data == NULL) { //means app doesn't allocate the buffer, so _encode will allocate it.

        //iovout->data = g_malloc (iovout->data_size);
        iovout->data = new uchar[iovout->data_size];
        if (iovout->data == NULL) {
            LOG_E( "iovout->data == NULL\n");
            ret = MIX_RESULT_NO_MEMORY;
            goto cleanup;
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

        coded_seg = (VACodedBufferSegment *)coded_seg->next;
    }

    iovout->buffer_size = iovout->data_size;

    LOG_I(
        "out size is = %d\n", iovout->data_size);

    va_status = vaUnmapBuffer (va_display, this->last_coded_buf);
    if (va_status != VA_STATUS_SUCCESS)
    {
        LOG_E( "Failed vaUnmapBuffer\n");
        ret = MIX_RESULT_FAIL;
        goto cleanup;
    }

    LOG_V( "get encoded data done\n");

    if (!((this->va_rcmode == VA_RC_NONE) || this->encoded_frames == 1)) {

        va_status = vaEndPicture (va_display, va_context);
        if (va_status != VA_STATUS_SUCCESS)
        {
            LOG_E( "Failed vaEndPicture\n");
            return MIX_RESULT_FAIL;
        }
    }

    if (this->encoded_frames == 1) {
        va_status = vaBeginPicture(va_display, va_context, surface);
        if (va_status != VA_STATUS_SUCCESS)
        {
            LOG_E( "Failed vaBeginPicture\n");
            ret = MIX_RESULT_FAIL;
            goto cleanup;
        }

        ret = _send_encode_command ();
        if (ret != MIX_RESULT_SUCCESS)
        {
            LOG_E (
                "Failed mix_videofmtenc_h264_send_encode_command\n");
            goto cleanup;
        }

        va_status = vaEndPicture (va_display, va_context);
        if (va_status != VA_STATUS_SUCCESS)
        {
            LOG_E( "Failed vaEndPicture\n");
            ret = MIX_RESULT_FAIL;
            goto cleanup;
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
        goto cleanup;
    }
    this->pic_skipped = status & VASurfaceSkipped;

    //ret = mix_framemanager_enqueue(parent->framemgr, mix->rec_frame);

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

    /*update the reference surface and reconstructed surface */
    if (!this->pic_skipped) {
        tmp_frame = this->rec_frame;
        this->rec_frame= this->ref_frame;
        this->ref_frame = tmp_frame;
    }


#if 0
    if (this->ref_frame != NULL)
        mix_videoframe_unref (this->ref_frame);
    this->ref_frame = this->rec_frame;

    mix_videoframe_unref (this->cur_frame);
#endif

    this->encoded_frames ++;
    this->last_coded_buf = this->coded_buf[this->coded_buf_index];
    this->coded_buf_index ++;
    this->coded_buf_index %=2;
    this->last_frame = this->cur_frame;

    if (this->last_mix_buffer) {
        LOG_V("calls to mix_buffer_unref \n");
        LOG_V("refcount = %d\n", MIX_PARAMS(this->last_mix_buffer)->GetRefCount());
        mix_buffer_unref(this->last_mix_buffer);
    }

    LOG_V("ref the current bufin\n");
    this->last_mix_buffer = mix_buffer_ref(bufin);

    if (!(this->need_display)) {
        mix_videoframe_unref (this->cur_frame);
        this->cur_frame = NULL;
    }

cleanup:

    if (ret != MIX_RESULT_SUCCESS) {
        if (iovout->data) {
            //g_free(iovout->data);
            delete[]iovout->data;
            iovout->data = NULL;
        }
    }

    LOG_V( "end\n");

    return MIX_RESULT_SUCCESS;
}

MIX_RESULT MixVideoFormatEnc_H263::_send_encode_command () {
    MIX_RESULT ret = MIX_RESULT_SUCCESS;

    LOG_V( "Begin\n");

    if (MIX_IS_VIDEOFORMATENC_H263(this))
    {
        if (this->encoded_frames == 0) {
            ret = _send_seq_params ();
            if (ret != MIX_RESULT_SUCCESS)
            {
                LOG_E(
                    "Failed SendSeqParams\n");
                return MIX_RESULT_FAIL;
            }
        }

        ret = _send_picture_parameter ();

        if (ret != MIX_RESULT_SUCCESS)
        {
            LOG_E(
                "Failed SendPictureParameter\n");
            return MIX_RESULT_FAIL;
        }

        ret = _send_slice_parameter ();
        if (ret != MIX_RESULT_SUCCESS)
        {
            LOG_E(
                "Failed SendSliceParameter\n");
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


MIX_RESULT MixVideoFormatEnc_H263::_send_seq_params () {
    VAStatus va_status;
    VAEncSequenceParameterBufferH263 h263_seq_param;
    VABufferID				seq_para_buf_id;


    LOG_V( "Begin\n\n");


    /*set up the sequence params for HW*/
    h263_seq_param.bits_per_second= this->bitrate;
    h263_seq_param.frame_rate = 30; //hard-coded, driver need;
    //(unsigned int) (parent->frame_rate_num + parent->frame_rate_denom /2 ) / parent->frame_rate_denom;
    h263_seq_param.initial_qp = this->initial_qp;
    h263_seq_param.min_qp = this->min_qp;
    h263_seq_param.intra_period = this->intra_period;

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

    va_status = vaCreateBuffer(this->va_display, this->va_context,
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

    va_status = vaRenderPicture(this->va_display, this->va_context,
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

MIX_RESULT MixVideoFormatEnc_H263::_send_picture_parameter () {

    VAStatus va_status;
    VAEncPictureParameterBufferH263 h263_pic_param;

    LOG_V( "Begin\n\n");

#if 0 //not needed currently
    MixVideoConfigParamsEncH263 * params_h263
    = MIX_VIDEOCONFIGPARAMSENC_H263 (config_params_enc);
#endif


    /*set picture params for HW*/
    h263_pic_param.reference_picture = this->ref_frame->frame_id;
    h263_pic_param.reconstructed_picture = this->rec_frame->frame_id;
    h263_pic_param.coded_buf = this->coded_buf[this->coded_buf_index];
    h263_pic_param.picture_width = this->picture_width;
    h263_pic_param.picture_height = this->picture_height;
    h263_pic_param.picture_type = this->is_intra ? VAEncPictureTypeIntra : VAEncPictureTypePredictive;



    LOG_V(
        "======h263 picture params======\n");
    LOG_I( "reference_picture = 0x%08x\n",
           h263_pic_param.reference_picture);
    LOG_I( "reconstructed_picture = 0x%08x\n",
           h263_pic_param.reconstructed_picture);
    LOG_I( "coded_buf = 0x%08x\n",
           h263_pic_param.coded_buf);
    LOG_I( "coded_buf_index = %d\n",
           this->coded_buf_index);
    LOG_I( "picture_width = %d\n",
           h263_pic_param.picture_width);
    LOG_I( "picture_height = %d\n",
           h263_pic_param.picture_height);
    LOG_I( "picture_type = %d\n\n",
           h263_pic_param.picture_type);

    va_status = vaCreateBuffer(this->va_display, this->va_context,
                               VAEncPictureParameterBufferType,
                               sizeof(h263_pic_param),
                               1,&h263_pic_param,
                               &this->pic_param_buf);

    if (va_status != VA_STATUS_SUCCESS)
    {
        LOG_E(
            "Failed to vaCreateBuffer\n");
        return MIX_RESULT_FAIL;
    }


    va_status = vaRenderPicture(this->va_display, this->va_context,
                                &this->pic_param_buf, 1);

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

MIX_RESULT MixVideoFormatEnc_H263::_send_slice_parameter () {
    VAStatus va_status;

    uint slice_num;
    uint slice_height;
    uint slice_index;
    uint slice_height_in_mb;



    LOG_V("Begin\n\n");


    //slice_num = mix->slice_num;
    slice_num = 1; // one slice per picture;
    slice_height = this->picture_height / slice_num;

    slice_height += 15;
    slice_height &= (~15);

    va_status = vaCreateBuffer (this->va_display, this->va_context,
                                VAEncSliceParameterBufferType,
                                sizeof(VAEncSliceParameterBuffer),
                                slice_num, NULL,
                                &this->slice_param_buf);

    if (va_status != VA_STATUS_SUCCESS)
    {
        LOG_E("Failed to vaCreateBuffer\n");
        return MIX_RESULT_FAIL;
    }

    VAEncSliceParameterBuffer *slice_param, *current_slice;

    va_status = vaMapBuffer(this->va_display,
                            this->slice_param_buf,
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
            min (slice_height, this->picture_height
                 - slice_index * slice_height) / 16;

        // starting MB row number for this slice
        current_slice->start_row_number = slice_index * slice_height / 16;
        // slice height measured in MB
        current_slice->slice_height = slice_height_in_mb;
        current_slice->slice_flags.bits.is_intra = this->is_intra;
        current_slice->slice_flags.bits.disable_deblocking_filter_idc
        = this->disable_deblocking_filter_idc;

        LOG_V("======h263 slice params======\n");

        LOG_I("slice_index = %d\n",
              (int) slice_index);
        LOG_I("start_row_number = %d\n",
              (int) current_slice->start_row_number);
        LOG_I("slice_height_in_mb = %d\n",
              (int) current_slice->slice_height);
        LOG_I("slice.is_intra = %d\n",
              (int) current_slice->slice_flags.bits.is_intra);
        LOG_I("disable_deblocking_filter_idc = %d\n\n",
              (int) this->disable_deblocking_filter_idc);

    }

    va_status = vaUnmapBuffer(this->va_display, this->slice_param_buf);

    if (va_status != VA_STATUS_SUCCESS)
    {
        LOG_E("Failed to vaUnmapBuffer\n");
        return MIX_RESULT_FAIL;
    }

    va_status = vaRenderPicture(this->va_display, this->va_context,
                                &this->slice_param_buf, 1);

    if (va_status != VA_STATUS_SUCCESS)
    {
        LOG_E("Failed to vaRenderPicture\n");
        return MIX_RESULT_FAIL;
    }

    LOG_V("end\n");

    return MIX_RESULT_SUCCESS;
}


MIX_RESULT  MixVideoFormatEnc_H263::_send_dynamic_bitrate () {
    VAStatus va_status;


    LOG_V( "Begin\n\n");


    if (this->va_rcmode != MIX_RATE_CONTROL_VCM) {

        LOG_W ("Not in VCM mode, but call SendDynamicBitrate\n");
        return VA_STATUS_SUCCESS;
    }

    VAEncMiscParameterBuffer *  misc_enc_param_buf;
    VAEncMiscParameterRateControl * bitrate_control_param;
    VABufferID misc_param_buffer_id;

    va_status = vaCreateBuffer(this->va_display, this->va_context,
                               VAEncMiscParameterBufferType,
                               sizeof(VAEncMiscParameterBuffer) + sizeof (VAEncMiscParameterRateControl),
                               1, NULL,
                               &misc_param_buffer_id);
    if (va_status != VA_STATUS_SUCCESS)
    {
        LOG_E(
            "Failed to vaCreateBuffer\n");
        return MIX_RESULT_FAIL;
    }

    va_status = vaMapBuffer (this->va_display, misc_param_buffer_id, (void **)&misc_enc_param_buf);
    if (va_status != VA_STATUS_SUCCESS)
    {
        LOG_E(
            "Failed to vaCreateBuffer\n");
        return MIX_RESULT_FAIL;
    }

    misc_enc_param_buf->type = VAEncMiscParameterTypeRateControl;
    bitrate_control_param = (VAEncMiscParameterRateControl *)misc_enc_param_buf->data;

    bitrate_control_param->bits_per_second = this->bitrate;
    bitrate_control_param->initial_qp = this->initial_qp;
    bitrate_control_param->min_qp = this->min_qp;
    bitrate_control_param->target_percentage = this->target_percentage;
    bitrate_control_param->window_size = this->window_size;

    va_status = vaUnmapBuffer(this->va_display, misc_param_buffer_id);
    if (va_status != VA_STATUS_SUCCESS)
    {
        LOG_E(
            "Failed to vaUnmapBuffer\n");
        return MIX_RESULT_FAIL;
    }


    va_status = vaRenderPicture(this->va_display, this->va_context,
                                &misc_param_buffer_id, 1);
    if (va_status != VA_STATUS_SUCCESS)
    {
        LOG_E(
            "Failed to vaRenderPicture\n");
        return MIX_RESULT_FAIL;
    }

    return MIX_RESULT_SUCCESS;

}


MIX_RESULT MixVideoFormatEnc_H263::_send_dynamic_framerate() {
    VAStatus va_status;

    LOG_V( "Begin\n\n");

    if (this->va_rcmode != MIX_RATE_CONTROL_VCM) {

        LOG_W ("Not in VCM mode, but call SendDynamicFramerate\n");
        return VA_STATUS_SUCCESS;
    }

    VAEncMiscParameterBuffer *  misc_enc_param_buf;
    VAEncMiscParameterFrameRate * framerate_param;
    VABufferID misc_param_buffer_id;

    va_status = vaCreateBuffer(this->va_display, this->va_context,
                               VAEncMiscParameterBufferType,
                               sizeof(misc_enc_param_buf) + sizeof(VAEncMiscParameterFrameRate),
                               1, NULL,
                               &misc_param_buffer_id);
    if (va_status != VA_STATUS_SUCCESS)
    {
        LOG_E(
            "Failed to vaCreateBuffer\n");
        return MIX_RESULT_FAIL;
    }

    va_status = vaMapBuffer (this->va_display, misc_param_buffer_id, (void **)&misc_enc_param_buf);
    if (va_status != VA_STATUS_SUCCESS)
    {
        LOG_E(
            "Failed to vaCreateBuffer\n");
        return MIX_RESULT_FAIL;
    }

    misc_enc_param_buf->type = VAEncMiscParameterTypeFrameRate;
    framerate_param = (VAEncMiscParameterFrameRate *)misc_enc_param_buf->data;
    framerate_param->framerate =
        (unsigned int) (this->frame_rate_num + this->frame_rate_denom /2 ) / this->frame_rate_denom;

    va_status = vaUnmapBuffer(this->va_display, misc_param_buffer_id);
    if (va_status != VA_STATUS_SUCCESS)
    {
        LOG_E(
            "Failed to vaUnmapBuffer\n");
        return MIX_RESULT_FAIL;
    }

    va_status = vaRenderPicture(this->va_display, this->va_context,
                                &misc_param_buffer_id, 1);
    if (va_status != VA_STATUS_SUCCESS)
    {
        LOG_E(
            "Failed to vaRenderPicture\n");
        return MIX_RESULT_FAIL;
    }


    LOG_I( "frame rate = %d\n",
           framerate_param->framerate);

    return MIX_RESULT_SUCCESS;

}


