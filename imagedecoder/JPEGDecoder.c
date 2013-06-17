/* INTEL CONFIDENTIAL
* Copyright (c) 2012 Intel Corporation.  All rights reserved.
* Copyright (c) Imagination Technologies Limited, UK
*
* The source code contained or described herein and all documents
* related to the source code ("Material") are owned by Intel
* Corporation or its suppliers or licensors.  Title to the
* Material remains with Intel Corporation or its suppliers and
* licensors.  The Material contains trade secrets and proprietary
* and confidential information of Intel or its suppliers and
* licensors. The Material is protected by worldwide copyright and
* trade secret laws and treaty provisions.  No part of the Material
* may be used, copied, reproduced, modified, published, uploaded,
* posted, transmitted, distributed, or disclosed in any way without
* Intel's prior express written permission.
*
* No license under any patent, copyright, trade secret or other
* intellectual property right is granted to or conferred upon you
* by disclosure or delivery of the Materials, either expressly, by
* implication, inducement, estoppel or otherwise. Any license
* under such intellectual property rights must be express and
* approved by Intel in writing.
*
* Authors:
*    Nana Guo <nana.n.guo@intel.com>
*    Yao Cheng <yao.cheng@intel.com>
*
*/

#include "va/va_tpi.h"
#include "va/va_vpp.h"
#include "va/va_drmcommon.h"
#include "JPEGDecoder.h"
#include "ImageDecoderTrace.h"
#include "JPEGParser.h"
#include <string.h>
#include "jerror.h"
#include <hardware/gralloc.h>
#ifdef JPEGDEC_USES_GEN
#include "ufo/graphics.h"
#define INTEL_VPG_GRALLOC_MODULE_PERFORM_GET_BO_NAME 4
#endif

#define JPEG_MAX_SETS_HUFFMAN_TABLES 2

#define TABLE_CLASS_DC  0
#define TABLE_CLASS_AC  1
#define TABLE_CLASS_NUM 2

// for config
#define HW_DECODE_MIN_WIDTH  100 // for JPEG smaller than this, use SW decode
#define HW_DECODE_MIN_HEIGHT 100 // for JPEG smaller than this, use SW decode

// for debug
#define DECODE_DUMP_FILE    "" // no dump by default
#define YUY2_DUMP_FILE      "" // no dump by default
#define RGBA_DUMP_FILE      "" // no dump by default

#define JD_CHECK(err, label) \
        if (err) { \
            ETRACE("%s::%d: failed: %d", __PRETTY_FUNCTION__, __LINE__, err); \
            goto label; \
        }

#define JD_CHECK_RET(err, label, retcode) \
        if (err) { \
            status = retcode; \
            ETRACE("%s::%d: failed: %d", __PRETTY_FUNCTION__, __LINE__, err); \
            goto label; \
        }

const char * fourcc2str(uint32_t fourcc)
{
    static char str[5];
    memset(str, 0, sizeof str);
    str[0] = fourcc & 0xff;
    str[1] = (fourcc >> 8 )& 0xff;
    str[2] = (fourcc >> 16) & 0xff;
    str[3] = (fourcc >> 24)& 0xff;
    str[4] = '\0';
    return str;
}

// VPG supports only YUY2->RGBA, YUY2->NV12_TILED now
// needs to convert IMC3/YV16/444P to YUY2 before HW CSC
static void write_to_YUY2(uint8_t *pDst,
                          uint32_t dst_stride,
                          VAImage *pImg,
                          uint8_t *pSrc)
{
    uint8_t *pY, *pU, *pV;
    float h_samp_factor, v_samp_factor;
    int row, col;
    switch (pImg->format.fourcc) {
    case VA_FOURCC_IMC3:
        h_samp_factor = 0.5;
        v_samp_factor = 0.5;
        break;
    case VA_FOURCC_422H:
        h_samp_factor = 0.5;
        v_samp_factor = 1;
        break;
    case VA_FOURCC_444P:
        h_samp_factor = 1;
        v_samp_factor = 1;
        break;
    default:
        // non-supported
        ETRACE("%s to YUY2: Not-supported input YUV format", fourcc2str(pImg->format.fourcc));
        return;
    }
    pY = pSrc + pImg->offsets[0];
    pU = pSrc + pImg->offsets[1];
    pV = pSrc + pImg->offsets[2];
    for (row = 0; row < pImg->height; ++row) {
        for (col = 0; col < pImg->width; ++col) {
            // Y
            *(pDst + 2 * col) = *(pY + col);
            uint32_t actual_col = h_samp_factor * col;
            if (col % 2 == 1) {
                // U
                *(pDst + 2 * col + 1) = *(pU + actual_col);
            }
            else {
                // V
                *(pDst + 2 * col + 1) = *(pV + actual_col);
            }
        }
        pDst += dst_stride * 2;
        pY += pImg->pitches[0];
        uint32_t actual_row = row * v_samp_factor;
        pU = pSrc + pImg->offsets[1] + actual_row * pImg->pitches[1];
        pV = pSrc + pImg->offsets[2] + actual_row * pImg->pitches[2];
    }
}

static void write_to_file(char *file, VAImage *pImg, uint8_t *pSrc)
{
    FILE *fp = fopen(file, "wb");
    if (!fp) {
        return;
    }
    uint8_t *pY, *pU, *pV;
    float h_samp_factor, v_samp_factor;
    int row, col;
    ITRACE("Dumping decoded YUV to %s", file);
    switch (pImg->format.fourcc) {
    case VA_FOURCC_IMC3:
        h_samp_factor = 0.5;
        v_samp_factor = 0.5;
        break;
    case VA_FOURCC_422H:
        h_samp_factor = 0.5;
        v_samp_factor = 1;
        break;
    case VA_FOURCC_444P:
        h_samp_factor = 1;
        v_samp_factor = 1;
        break;
    default:
        // non-supported
        ETRACE("%s to YUY2: Not-supported input YUV format", fourcc2str(pImg->format.fourcc));
        return;
    }
    pY = pSrc + pImg->offsets[0];
    pU = pSrc + pImg->offsets[1];
    pV = pSrc + pImg->offsets[2];
    // Y
    for (row = 0; row < pImg->height; ++row) {
        fwrite(pY, 1, pImg->width, fp);
        pY += pImg->pitches[0];
    }
    // U
    for (row = 0; row < pImg->height * v_samp_factor; ++row) {
        fwrite(pU, 1, pImg->width * h_samp_factor, fp);
        pU += pImg->pitches[1];
    }
    // V
    for (row = 0; row < pImg->height * v_samp_factor; ++row) {
        fwrite(pV, 1, pImg->width * h_samp_factor, fp);
        pV += pImg->pitches[2];
    }
    fclose(fp);
}

/*
 * Initialize VA API related stuff
 *
 * We will check the return value of  jva_initialize
 * to determine which path will be use (SW or HW)
 *
 */
Decode_Status jdva_initialize (jd_libva_struct * jd_libva_ptr) {
  /*
   * Please note that we won't check the input parameters to follow the
   * convention of libjpeg duo to we need these parameters to do error handling,
   * and if these parameters are invalid, means the whole stack is crashed, so check
   * them here and return false is meaningless, same situation for all internal methods
   * related to VA API
  */
    uint32_t va_major_version = 0;
    uint32_t va_minor_version = 0;
    VAStatus va_status = VA_STATUS_SUCCESS;
    Decode_Status status = DECODE_SUCCESS;
    uint32_t index;

    if (jd_libva_ptr->initialized)
        return DECODE_NOT_STARTED;

    jd_libva_ptr->android_display = (Display*)malloc(sizeof(Display));
    if (jd_libva_ptr->android_display == NULL) {
        return DECODE_MEMORY_FAIL;
    }
    jd_libva_ptr->va_display = vaGetDisplay (jd_libva_ptr->android_display);

    if (jd_libva_ptr->va_display == NULL) {
        ETRACE("vaGetDisplay failed.");
        free (jd_libva_ptr->android_display);
        return DECODE_DRIVER_FAIL;
    }
    va_status = vaInitialize(jd_libva_ptr->va_display, &va_major_version, &va_minor_version);
    if (va_status != VA_STATUS_SUCCESS) {
        ETRACE("vaInitialize failed. va_status = 0x%x", va_status);
        status = DECODE_DRIVER_FAIL;
        goto cleanup;
    }

    VAConfigAttrib attrib;
    attrib.type = VAConfigAttribRTFormat;
    va_status = vaGetConfigAttributes(jd_libva_ptr->va_display, VAProfileJPEGBaseline, VAEntrypointVLD, &attrib, 1);
    if (va_status != VA_STATUS_SUCCESS) {
        ETRACE("vaGetConfigAttributes failed. va_status = 0x%x", va_status);
        status = DECODE_DRIVER_FAIL;
        goto cleanup;
    }
    if ((VA_RT_FORMAT_YUV444 & attrib.value) == 0) {
        WTRACE("Format not surportted\n");
        status = DECODE_FAIL;
        goto cleanup;
    }

    va_status = vaCreateConfig(jd_libva_ptr->va_display, VAProfileJPEGBaseline, VAEntrypointVLD, &attrib, 1, &(jd_libva_ptr->va_config));
    if (va_status != VA_STATUS_SUCCESS) {
        ETRACE("vaCreateConfig failed. va_status = 0x%x", va_status);
        status = DECODE_DRIVER_FAIL;
        goto cleanup;
    }
    jd_libva_ptr->initialized = TRUE;
    status = DECODE_SUCCESS;

cleanup:
#if 0
    /*free profiles and entrypoints*/
    if (va_profiles)
        free(va_profiles);

    if (va_entrypoints)
        free (va_entrypoints);
#endif
    if (status) {
        jd_libva_ptr->initialized = TRUE; // make sure we can call into jva_deinitialize()
        jdva_deinitialize (jd_libva_ptr);
        return status;
    }

  return status;
}

void jdva_deinitialize (jd_libva_struct * jd_libva_ptr) {
    if (!(jd_libva_ptr->initialized)) {
        return;
    }

    if (jd_libva_ptr->JPEGParser) {
        free(jd_libva_ptr->JPEGParser);
        jd_libva_ptr->JPEGParser = NULL;
    }

    if (jd_libva_ptr->va_display) {
        vaTerminate(jd_libva_ptr->va_display);
        jd_libva_ptr->va_display = NULL;
    }

    if (jd_libva_ptr->android_display) {
        free(jd_libva_ptr->android_display);
        jd_libva_ptr->android_display = NULL;
    }

    jd_libva_ptr->initialized = FALSE;
    ITRACE("jdva_deinitialize finished");
    return;
}

static Decode_Status doColorConversion(jd_libva_struct *jd_libva_ptr, VASurfaceID surface, char ** buf, uint32_t rows)
{
#if 0 // dump RGB to file
        uint8_t* rgb_buf;
        int32_t data_len = 0;
        uint32_t surface_width, surface_height;
        surface_width = (( ( jd_libva_ptr->image_width + 7 ) & ( ~7 )) + 15 ) & ( ~15 );
        surface_height = (( ( jd_libva_ptr->image_height + 7 ) & ( ~7 )) + 15 ) & ( ~15 );

        rgb_buf = (uint8_t*) malloc((surface_width * surface_height) << 2);
        if(rgb_buf == NULL){
            return DECODE_MEMORY_FAIL;
        }
        va_status = vaPutSurfaceBuf(jd_libva_ptr->va_display, jd_libva_ptr->va_surfaces[0], rgb_buf, &data_len, 0, 0, surface_width, surface_height, 0, 0, surface_width, surface_height, NULL, 0, 0);

        buf = rgb_buf;
    // dump RGB data
        {
            FILE *pf_tmp = fopen("img_out.rgb", "wb");
            if(pf_tmp == NULL)
                ETRACE("Open file error");
            fwrite(rgb_buf, 1, surface_width * surface_height * 4, pf_tmp);
            fclose(pf_tmp);
        }
#endif

#ifdef JPEGDEC_USES_GEN
    VAImage decoded_img;
    uint8_t *decoded_buf = NULL;
    int row, col;
    VAStatus vpp_status;
    uint8_t *pSrc, *pDst;
    VADisplay display = NULL;
    VAContextID context = VA_INVALID_ID;
    VASurfaceID dst_surface = VA_INVALID_ID, src_surface = VA_INVALID_ID;
    VAConfigID config = VA_INVALID_ID;
    VAConfigAttrib  vpp_attrib;
    VASurfaceAttrib dst_surf_attrib, src_surf_attrib;
    buffer_handle_t dst_handle, src_handle;
    int32_t dst_stride, src_stride;
    uint32_t dst_buf_name, src_buf_name;
    VAProcPipelineParameterBuffer vpp_param;
    VABufferID vpp_pipeline_buf = VA_INVALID_ID;
    int major_version, minor_version;
    uint8_t *src_gralloc_buf, *dst_gralloc_buf;
    hw_module_t const* module = NULL;
    alloc_device_t *alloc_dev = NULL;
    struct gralloc_module_t *gralloc_module = NULL;
    VAProcPipelineCaps vpp_pipeline_cap ;
    VARectangle src_rect, dst_rect;
    int err;
    Display vppdpy;
    FILE *fp;
    Decode_Status status = DECODE_SUCCESS;
    VASurfaceAttribExternalBuffers vaSurfaceExternBufIn, vaSurfaceExternBufOut;

    decoded_img.image_id = VA_INVALID_ID;
    vpp_status = vaDeriveImage(jd_libva_ptr->va_display, surface, &decoded_img);
    JD_CHECK_RET(vpp_status, cleanup, DECODE_DRIVER_FAIL);

    vpp_status = vaMapBuffer(jd_libva_ptr->va_display, decoded_img.buf, (void **)&decoded_buf);
    if (vpp_status) {
        vaDestroyImage(jd_libva_ptr->va_display, decoded_img.image_id);
    }
    JD_CHECK_RET(vpp_status, cleanup, DECODE_DRIVER_FAIL);

    write_to_file(DECODE_DUMP_FILE, &decoded_img, decoded_buf);

    ITRACE("Start HW CSC: color %s=>RGBA8888", fourcc2str(jd_libva_ptr->fourcc));
    err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);
    JD_CHECK_RET(vpp_status, cleanup, DECODE_DRIVER_FAIL);

    gralloc_module = (struct gralloc_module_t *)module;
    err = gralloc_open(module, &alloc_dev);
    JD_CHECK_RET(vpp_status, cleanup, DECODE_DRIVER_FAIL);

    err = alloc_dev->alloc(alloc_dev,
                           jd_libva_ptr->image_width,
                           jd_libva_ptr->image_height,
                           HAL_PIXEL_FORMAT_YCbCr_422_I, // YUY2
                           GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_SW_WRITE_MASK | GRALLOC_USAGE_SW_READ_MASK,
                           (buffer_handle_t *)&src_handle,
                           &src_stride);
    JD_CHECK_RET(vpp_status, cleanup, DECODE_DRIVER_FAIL);

    ITRACE("src_gralloc_buf: handle=%u, stride=%u", src_handle, src_stride);
    err = alloc_dev->alloc(alloc_dev,
                           jd_libva_ptr->image_width,
                           jd_libva_ptr->image_height,
                           HAL_PIXEL_FORMAT_RGBA_8888,
                           GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_SW_WRITE_MASK | GRALLOC_USAGE_SW_READ_MASK,
                           (buffer_handle_t *)&dst_handle,
                           &dst_stride);
    JD_CHECK_RET(vpp_status, cleanup, DECODE_DRIVER_FAIL);
    ITRACE("dst_gralloc_buf: handle=%u, stride=%u", dst_handle, dst_stride);

    err = gralloc_module->perform(gralloc_module, INTEL_VPG_GRALLOC_MODULE_PERFORM_GET_BO_NAME, src_handle, &src_buf_name);
    JD_CHECK_RET(vpp_status, cleanup, DECODE_DRIVER_FAIL);
    err = gralloc_module->perform(gralloc_module, INTEL_VPG_GRALLOC_MODULE_PERFORM_GET_BO_NAME, dst_handle, &dst_buf_name);
    JD_CHECK_RET(vpp_status, cleanup, DECODE_DRIVER_FAIL);

    // copy decoded buf into the gralloc buf in YUY2 format
    err = gralloc_module->lock(gralloc_module, src_handle,
                               GRALLOC_USAGE_SW_WRITE_MASK,
                               0, 0,
                               jd_libva_ptr->image_width,
                               jd_libva_ptr->image_height,
                               &src_gralloc_buf);
    JD_CHECK_RET(vpp_status, cleanup, DECODE_DRIVER_FAIL);
    ITRACE("Convert %s buf into YUY2:", fourcc2str(jd_libva_ptr->fourcc));

    write_to_YUY2(src_gralloc_buf,
                  src_stride,
                  &decoded_img,
                  decoded_buf);
    err = gralloc_module->unlock(gralloc_module, src_handle);
    JD_CHECK_RET(vpp_status, cleanup, DECODE_DRIVER_FAIL);

    fp = fopen(YUY2_DUMP_FILE, "wb");
    if (fp) {
        ITRACE("DUMP YUY2 to " YUY2_DUMP_FILE);
        err = gralloc_module->lock(gralloc_module, src_handle,
                                   GRALLOC_USAGE_SW_READ_MASK,
                                   0, 0,
                                   jd_libva_ptr->image_width,
                                   jd_libva_ptr->image_height,
                                   &src_gralloc_buf);
        unsigned char *pYUV = src_gralloc_buf;
        int loop;
		for(loop=0;loop<jd_libva_ptr->image_height;loop++)
		{
            fwrite(pYUV, 2, jd_libva_ptr->image_width, fp);
            pYUV += 2 * src_stride;
		}
        gralloc_module->unlock(gralloc_module, src_handle);
		fclose(fp);
    }

    vaUnmapBuffer(jd_libva_ptr->va_display, decoded_img.buf);
    vaDestroyImage(jd_libva_ptr->va_display, decoded_img.image_id);
    decoded_buf= NULL;

    display = vaGetDisplay (&vppdpy);
    vpp_status = vaInitialize(display, &major_version, &minor_version);
    JD_CHECK_RET(vpp_status, cleanup, DECODE_DRIVER_FAIL);

    vpp_attrib.type  = VAConfigAttribRTFormat;
    vpp_attrib.value = VA_RT_FORMAT_YUV420;
    vpp_status = vaCreateConfig(display,
                                VAProfileNone,
                                VAEntrypointVideoProc,
                                &vpp_attrib,
                                1,
                                &config);
    JD_CHECK_RET(vpp_status, cleanup, DECODE_DRIVER_FAIL);

    vpp_status = vaCreateContext(display,
                                 config,
                                 jd_libva_ptr->image_width,
                                 jd_libva_ptr->image_height,
                                 0,
                                 NULL,
                                 0,
                                 &context);
    JD_CHECK_RET(vpp_status, cleanup, DECODE_DRIVER_FAIL);

    vaSurfaceExternBufIn.pixel_format  = VA_FOURCC_YUY2;
    vaSurfaceExternBufIn.width         = jd_libva_ptr->image_width;
    vaSurfaceExternBufIn.height        = jd_libva_ptr->image_height;
    vaSurfaceExternBufIn.pitches[0]    = src_stride * 2; // YUY2 is 16bit
    vaSurfaceExternBufIn.buffers       = &src_buf_name;
    vaSurfaceExternBufIn.num_buffers   = 1;
    vaSurfaceExternBufIn.flags         = VA_SURFACE_ATTRIB_MEM_TYPE_KERNEL_DRM;
    src_surf_attrib.type               = VASurfaceAttribExternalBufferDescriptor;
    src_surf_attrib.flags              = VA_SURFACE_ATTRIB_SETTABLE;
    src_surf_attrib.value.type         = VAGenericValueTypePointer;
    src_surf_attrib.value.value.p      = (void *)&vaSurfaceExternBufIn;
    vpp_status = vaCreateSurfaces(display,
                                  VA_RT_FORMAT_YUV422,
                                  jd_libva_ptr->image_width,
                                  jd_libva_ptr->image_height,
                                  &src_surface,
                                  1,
                                  &src_surf_attrib,
                                  1);
    JD_CHECK_RET(vpp_status, cleanup, DECODE_DRIVER_FAIL);

    vaSurfaceExternBufOut.pixel_format = VA_FOURCC_ARGB;
    vaSurfaceExternBufOut.width        = jd_libva_ptr->image_width;
    vaSurfaceExternBufOut.height       = jd_libva_ptr->image_height;
    vaSurfaceExternBufOut.pitches[0]   = dst_stride * 4; // RGBA is 32bit
    vaSurfaceExternBufOut.buffers      = &dst_buf_name;
    vaSurfaceExternBufOut.num_buffers  = 1;
    vaSurfaceExternBufOut.flags        = VA_SURFACE_ATTRIB_MEM_TYPE_KERNEL_DRM;
    dst_surf_attrib.type               = VASurfaceAttribExternalBufferDescriptor;
    dst_surf_attrib.flags              = VA_SURFACE_ATTRIB_SETTABLE;
    dst_surf_attrib.value.type         = VAGenericValueTypePointer;
    dst_surf_attrib.value.value.p      = (void *)&vaSurfaceExternBufOut;
    vpp_status = vaCreateSurfaces(display,
                                  VA_RT_FORMAT_RGB32,
                                  jd_libva_ptr->image_width,
                                  jd_libva_ptr->image_height,
                                  &dst_surface,
                                  1,
                                  &dst_surf_attrib,
                                  1);
    JD_CHECK_RET(vpp_status, cleanup, DECODE_DRIVER_FAIL);
    ITRACE("vaCreateSurfaces got surface %u=>%u", src_surface, dst_surface);
    //query caps for pipeline
    vpp_status = vaQueryVideoProcPipelineCaps(display,
                                              context,
                                              NULL,
                                              0,
                                              &vpp_pipeline_cap);
    JD_CHECK_RET(vpp_status, cleanup, DECODE_DRIVER_FAIL);

    src_rect.x = dst_rect.x           = 0;
    src_rect.y = dst_rect.y           = 0;
    src_rect.width  = dst_rect.width  = jd_libva_ptr->image_width;
    src_rect.height = dst_rect.height = jd_libva_ptr->image_height;
    ITRACE("from (%d, %d, %u, %u) to (%d, %d, %u, %u)",
        src_rect.x, src_rect.y, src_rect.width, src_rect.height,
        dst_rect.x, dst_rect.y, dst_rect.width, dst_rect.height);
    vpp_param.surface                 = src_surface;
    vpp_param.output_region           = &dst_rect;
    vpp_param.surface_region          = &src_rect;
    vpp_param.surface_color_standard  = VAProcColorStandardBT601;   //csc
    vpp_param.output_background_color = 0x8000;                     //colorfill
    vpp_param.output_color_standard   = VAProcColorStandardNone;
    vpp_param.filter_flags            = VA_FRAME_PICTURE;
    vpp_param.filters                 = NULL;
    //vpp_param.pipeline_flags          = 1084929476;
    vpp_param.num_filters             = 0;
    vpp_param.forward_references      = 0;
    vpp_param.num_forward_references  = 0;
    vpp_param.backward_references     = 0;
    vpp_param.num_backward_references = 0;
    vpp_param.blend_state             = NULL;
    vpp_param.rotation_state          = VA_ROTATION_NONE;
    vpp_status = vaCreateBuffer(display,
                                context,
                                VAProcPipelineParameterBufferType,
                                sizeof(VAProcPipelineParameterBuffer),
                                1,
                                &vpp_param,
                                &vpp_pipeline_buf);
    JD_CHECK_RET(vpp_status, cleanup, DECODE_DRIVER_FAIL);

    vpp_status = vaBeginPicture(display,
                                context,
                                dst_surface);
    JD_CHECK_RET(vpp_status, cleanup, DECODE_DRIVER_FAIL);

    //Render the picture
    vpp_status = vaRenderPicture(display,
                                 context,
                                 &vpp_pipeline_buf,
                                 1);
    JD_CHECK_RET(vpp_status, cleanup, DECODE_DRIVER_FAIL);

    vpp_status = vaEndPicture(display, context);
    JD_CHECK_RET(vpp_status, cleanup, DECODE_DRIVER_FAIL);

    vpp_status = vaSyncSurface(display, dst_surface);
    JD_CHECK_RET(vpp_status, cleanup, DECODE_DRIVER_FAIL);
    ITRACE("Finished HW CSC YUY2=>RGBA8888");

    // gralloc lock + copy
    err = gralloc_module->lock(gralloc_module, dst_handle,
                               GRALLOC_USAGE_SW_READ_MASK,
                               0, 0,
                               jd_libva_ptr->image_width,
                               jd_libva_ptr->image_height,
                               &dst_gralloc_buf);
    JD_CHECK_RET(vpp_status, cleanup, DECODE_DRIVER_FAIL);
    ITRACE("Copy RGBA8888 buffer (%ux%u) to skia buffer (%ux%u)",
           jd_libva_ptr->image_width,
           jd_libva_ptr->image_height,
           buf[1] - buf[0],
           rows);
    pSrc = dst_gralloc_buf;
    fp = fopen(RGBA_DUMP_FILE, "wb");
    if (fp)
        ITRACE("dumping RGBA8888 to " RGBA_DUMP_FILE);
    // FIXME: is it RGBA? or BGRA? or ARGB?
    for (row = 0; row < rows; ++ row) {
        memcpy(buf[row], pSrc, 4 * jd_libva_ptr->image_width);
        if (fp)
            fwrite(pSrc, 4, jd_libva_ptr->image_width, fp);
        pSrc += dst_stride * 4;
    }
    if (fp)
        fclose(fp);
    gralloc_module->unlock(gralloc_module, dst_handle);

cleanup:
    if (vpp_pipeline_buf != VA_INVALID_ID)
        vaDestroyBuffer(display, vpp_pipeline_buf);
    if (dst_surface != VA_INVALID_ID)
        vaDestroySurfaces(display, &dst_surface, 1);
    if (src_surface != VA_INVALID_ID)
        vaDestroySurfaces(display, &src_surface, 1);
    if (context != VA_INVALID_ID)
        vaDestroyContext(display, context);
    if (config != VA_INVALID_ID)
        vaDestroyConfig(display, config);
    if (display)
        vaTerminate(display);
    if (alloc_dev) {
        alloc_dev->free(alloc_dev, dst_handle);
        alloc_dev->free(alloc_dev, src_handle);
        gralloc_close(alloc_dev);
    }
    return status;
#else
    // TODO: CSC with Gralloc On PVR platform
#endif



}

static unsigned int getSurfaceFormat(jd_libva_struct * jd_libva_ptr, VASurfaceAttrib * fourcc) {
    int h1, h2, h3, v1, v2, v3;
    h1 = jd_libva_ptr->picture_param_buf.components[0].h_sampling_factor;
    h2 = jd_libva_ptr->picture_param_buf.components[1].h_sampling_factor;
    h3 = jd_libva_ptr->picture_param_buf.components[2].h_sampling_factor;
    v1 = jd_libva_ptr->picture_param_buf.components[0].v_sampling_factor;
    v2 = jd_libva_ptr->picture_param_buf.components[1].v_sampling_factor;
    v3 = jd_libva_ptr->picture_param_buf.components[2].v_sampling_factor;

    fourcc->type = VASurfaceAttribPixelFormat;
    fourcc->flags = VA_SURFACE_ATTRIB_SETTABLE;
    fourcc->value.type = VAGenericValueTypeInteger;

    if (h1 == 2 && h2 == 1 && h3 == 1 &&
            v1 == 2 && v2 == 1 && v3 == 1) {
        fourcc->value.value.i = VA_FOURCC_IMC3;
        return VA_RT_FORMAT_YUV420;
    }
    else if (h1 == 2 && h2 == 1 && h3 == 1 &&
            v1 == 1 && v2 == 1 && v3 == 1) {
        fourcc->value.value.i = VA_FOURCC_422H;
        return VA_RT_FORMAT_YUV422;
    }
    else if (h1 == 1 && h2 == 1 && h3 == 1 &&
            v1 == 1 && v2 == 1 && v3 == 1) {
        fourcc->value.value.i = VA_FOURCC_444P;
        return VA_RT_FORMAT_YUV444;
    }
    else if (h1 == 4 && h2 == 1 && h3 == 1 &&
            v1 == 1 && v2 == 1 && v3 == 1) {
        fourcc->value.value.i = VA_FOURCC_411P;
        ITRACE("SurfaceFormat: 411P");
        return VA_RT_FORMAT_YUV411;
    }
    else if (h1 == 1 && h2 == 1 && h3 == 1 &&
            v1 == 2 && v2 == 1 && v3 == 1) {
        fourcc->value.value.i = VA_FOURCC_422V;
        return VA_RT_FORMAT_YUV422;
    }
    else if (h1 == 2 && h2 == 1 && h3 == 1 &&
            v1 == 2 && v2 == 2 && v3 == 2) {
        fourcc->value.value.i = VA_FOURCC_422H;
        return VA_RT_FORMAT_YUV422;
    }
    else if (h2 == 2 && h2 == 2 && h3 == 2 &&
            v1 == 2 && v2 == 1 && v3 == 1) {
        fourcc->value.value.i = VA_FOURCC_422V;
        return VA_RT_FORMAT_YUV422;
    }
    else
    {
        fourcc->value.value.i = VA_FOURCC('4','0','0','P');
        return VA_RT_FORMAT_YUV400;
    }

}

Decode_Status jdva_create_resource (jd_libva_struct * jd_libva_ptr) {
    VAStatus va_status = VA_STATUS_SUCCESS;
    Decode_Status status = DECODE_SUCCESS;
    jd_libva_ptr->image_width = jd_libva_ptr->picture_param_buf.picture_width;
    jd_libva_ptr->image_height = jd_libva_ptr->picture_param_buf.picture_height;
    jd_libva_ptr->surface_count = 1;
    jd_libva_ptr->va_surfaces = (VASurfaceID *) malloc(sizeof(VASurfaceID)*jd_libva_ptr->surface_count);
    if (jd_libva_ptr->va_surfaces == NULL) {
        return DECODE_MEMORY_FAIL;
    }

    VASurfaceAttrib fourcc;
    unsigned int surface_format = getSurfaceFormat(jd_libva_ptr, &fourcc);
    jd_libva_ptr->fourcc = fourcc.value.value.i;
#ifdef JPEGDEC_USES_GEN
    va_status = vaCreateSurfaces(jd_libva_ptr->va_display, surface_format,
                                    jd_libva_ptr->image_width,
                                    jd_libva_ptr->image_height,
                                    jd_libva_ptr->va_surfaces,
                                    jd_libva_ptr->surface_count, &fourcc, 1);
#else
    va_status = vaCreateSurfaces(jd_libva_ptr->va_display, VA_RT_FORMAT_YUV444,
                                    jd_libva_ptr->image_width,
                                    jd_libva_ptr->image_height,
                                    jd_libva_ptr->va_surfaces,
                                    jd_libva_ptr->surface_count, NULL, 0);
#endif
    JD_CHECK(va_status, cleanup);
    va_status = vaCreateContext(jd_libva_ptr->va_display, jd_libva_ptr->va_config,
                                   jd_libva_ptr->image_width,
                                   jd_libva_ptr->image_height,
                                   0,  //VA_PROGRESSIVE
                                   jd_libva_ptr->va_surfaces,
                                   jd_libva_ptr->surface_count, &(jd_libva_ptr->va_context));
    if (va_status != VA_STATUS_SUCCESS) {
        ETRACE("vaCreateContext failed. va_status = 0x%x", va_status);
        return DECODE_DRIVER_FAIL;
    }

    jd_libva_ptr->resource_allocated = TRUE;
    return status;
cleanup:
    jd_libva_ptr->resource_allocated = FALSE;

    if (jd_libva_ptr->va_surfaces) {
        free (jd_libva_ptr->va_surfaces);
        jd_libva_ptr->va_surfaces = NULL;
    }
    jdva_deinitialize (jd_libva_ptr);

    return DECODE_DRIVER_FAIL;
}

Decode_Status jdva_release_resource (jd_libva_struct * jd_libva_ptr) {
    Decode_Status status = DECODE_SUCCESS;
    VAStatus va_status = VA_STATUS_SUCCESS;

    if (!(jd_libva_ptr->resource_allocated)) {
        return status;
    }

    if (!(jd_libva_ptr->va_display)) {
        return status; //most likely the resource are already released and HW jpeg is deinitialize, return directly
    }

  /*
   * It is safe to destroy Surface/Config/Context severl times
   * and it is also safe even their value is NULL
   */
    va_status = vaDestroySurfaces(jd_libva_ptr->va_display, jd_libva_ptr->va_surfaces, jd_libva_ptr->surface_count);

    va_status = vaDestroyContext(jd_libva_ptr->va_display, jd_libva_ptr->va_context);
    if (va_status != VA_STATUS_SUCCESS) {
      ETRACE("vaDestroyContext failed. va_status = 0x%x", va_status);
      return DECODE_DRIVER_FAIL;
    }
    jd_libva_ptr->va_context = NULL;

    if (jd_libva_ptr->va_surfaces) {
        free (jd_libva_ptr->va_surfaces);
        jd_libva_ptr->va_surfaces = NULL;
    }

    va_status = vaDestroyConfig(jd_libva_ptr->va_display, jd_libva_ptr->va_config);
    if (va_status != VA_STATUS_SUCCESS) {
        ETRACE("vaDestroyConfig failed. va_status = 0x%x", va_status);
        return DECODE_DRIVER_FAIL;
    }

cleanup:
    jd_libva_ptr->va_config = NULL;

    jd_libva_ptr->resource_allocated = FALSE;

    return va_status;
}

Decode_Status jdva_decode (j_decompress_ptr cinfo, jd_libva_struct * jd_libva_ptr) {
    Decode_Status status = DECODE_SUCCESS;
    VAStatus va_status = VA_STATUS_SUCCESS;
    VABufferID desc_buf[5];
    uint32_t bitstream_buffer_size = 0;
    uint32_t scan_idx = 0;
    uint32_t buf_idx = 0;
    char **buf = jd_libva_ptr->output_image;
    uint32_t lines = jd_libva_ptr->output_lines;
    uint32_t chopping = VA_SLICE_DATA_FLAG_ALL;
    uint32_t bytes_remaining;

    if (jd_libva_ptr->eoi_offset)
        bytes_remaining = jd_libva_ptr->eoi_offset - jd_libva_ptr->soi_offset;
    else
        bytes_remaining = jd_libva_ptr->file_size - jd_libva_ptr->soi_offset;
    uint32_t src_offset = jd_libva_ptr->soi_offset;
    uint32_t cpy_row;
    bitstream_buffer_size = cinfo->src->bytes_in_buffer;//1024*1024*5;

    va_status = vaBeginPicture(jd_libva_ptr->va_display, jd_libva_ptr->va_context, jd_libva_ptr->va_surfaces[0]);
    if (va_status != VA_STATUS_SUCCESS) {
        ETRACE("vaBeginPicture failed. va_status = 0x%x", va_status);
        status = DECODE_DRIVER_FAIL;
        return status;
    }
    va_status = vaCreateBuffer(jd_libva_ptr->va_display, jd_libva_ptr->va_context, VAPictureParameterBufferType, sizeof(VAPictureParameterBufferJPEGBaseline), 1, &jd_libva_ptr->picture_param_buf, &desc_buf[buf_idx]);
    if (va_status != VA_STATUS_SUCCESS) {
        ETRACE("vaCreateBuffer VAPictureParameterBufferType failed. va_status = 0x%x", va_status);
        status = DECODE_DRIVER_FAIL;
        return status;
    }
    buf_idx++;
    va_status = vaCreateBuffer(jd_libva_ptr->va_display, jd_libva_ptr->va_context, VAIQMatrixBufferType, sizeof(VAIQMatrixBufferJPEGBaseline), 1, &jd_libva_ptr->qmatrix_buf, &desc_buf[buf_idx]);

    if (va_status != VA_STATUS_SUCCESS) {
        ETRACE("vaCreateBuffer VAIQMatrixBufferType failed. va_status = 0x%x", va_status);
        status = DECODE_DRIVER_FAIL;
        return status;
    }
    buf_idx++;
    va_status = vaCreateBuffer(jd_libva_ptr->va_display, jd_libva_ptr->va_context, VAHuffmanTableBufferType, sizeof(VAHuffmanTableBufferJPEGBaseline), 1, &jd_libva_ptr->hufman_table_buf, &desc_buf[buf_idx]);
    if (va_status != VA_STATUS_SUCCESS) {
        ETRACE("vaCreateBuffer VAHuffmanTableBufferType failed. va_status = 0x%x", va_status);
        status = DECODE_DRIVER_FAIL;
        return status;
    }
    buf_idx++;
    do {
        /* Get Bitstream Buffer */
        uint32_t bytes = ( bytes_remaining < bitstream_buffer_size ) ? bytes_remaining : bitstream_buffer_size;
        bytes_remaining -= bytes;
        /* Get Slice Control Buffer */
        VASliceParameterBufferJPEGBaseline dest_scan_ctrl[JPEG_MAX_COMPONENTS];
        uint32_t src_idx = 0;
        uint32_t dest_idx = 0;
        memset(dest_scan_ctrl, 0, sizeof(dest_scan_ctrl));
        for (src_idx = scan_idx; src_idx < jd_libva_ptr->scan_ctrl_count ; src_idx++) {
            if (jd_libva_ptr->slice_param_buf[ src_idx ].slice_data_offset) {
                /* new scan, reset state machine */
                chopping = VA_SLICE_DATA_FLAG_ALL;
                fprintf(stderr,"Scan:%i FileOffset:%x Bytes:%x \n", src_idx,
                    jd_libva_ptr->slice_param_buf[ src_idx ].slice_data_offset,
                    jd_libva_ptr->slice_param_buf[ src_idx ].slice_data_size );
                /* does the slice end in the buffer */
                if (jd_libva_ptr->slice_param_buf[ src_idx ].slice_data_offset + jd_libva_ptr->slice_param_buf[ src_idx ].slice_data_size > bytes + src_offset) {
                    chopping = VA_SLICE_DATA_FLAG_BEGIN;
                }
            } else {
                if (jd_libva_ptr->slice_param_buf[ src_idx ].slice_data_size > bytes) {
                    chopping = VA_SLICE_DATA_FLAG_MIDDLE;
                } else {
                    if ((chopping == VA_SLICE_DATA_FLAG_BEGIN) || (chopping == VA_SLICE_DATA_FLAG_MIDDLE)) {
                        chopping = VA_SLICE_DATA_FLAG_END;
                    }
                }
            }
            dest_scan_ctrl[dest_idx].slice_data_flag = chopping;
            dest_scan_ctrl[dest_idx].slice_data_offset = ((chopping == VA_SLICE_DATA_FLAG_ALL) ||      (chopping == VA_SLICE_DATA_FLAG_BEGIN) )?
jd_libva_ptr->slice_param_buf[ src_idx ].slice_data_offset : 0;

            const int32_t bytes_in_seg = bytes - dest_scan_ctrl[dest_idx].slice_data_offset;
            const uint32_t scan_data = (bytes_in_seg < jd_libva_ptr->slice_param_buf[src_idx].slice_data_size) ? bytes_in_seg : jd_libva_ptr->slice_param_buf[src_idx].slice_data_size ;
            jd_libva_ptr->slice_param_buf[src_idx].slice_data_offset = 0;
            jd_libva_ptr->slice_param_buf[src_idx].slice_data_size -= scan_data;
            dest_scan_ctrl[dest_idx].slice_data_size = scan_data;
            dest_scan_ctrl[dest_idx].num_components = jd_libva_ptr->slice_param_buf[src_idx].num_components;
            dest_scan_ctrl[dest_idx].restart_interval = jd_libva_ptr->slice_param_buf[src_idx].restart_interval;
            memcpy(&dest_scan_ctrl[dest_idx].components, & jd_libva_ptr->slice_param_buf[ src_idx ].components,
                sizeof(jd_libva_ptr->slice_param_buf[ src_idx ].components) );
            dest_idx++;
            if ((chopping == VA_SLICE_DATA_FLAG_ALL) || (chopping == VA_SLICE_DATA_FLAG_END)) { /* all good good */
            } else {
                break;
            }
        }
        scan_idx = src_idx;
        /* Get Slice Control Buffer */
        va_status = vaCreateBuffer(jd_libva_ptr->va_display, jd_libva_ptr->va_context, VASliceParameterBufferType, sizeof(VASliceParameterBufferJPEGBaseline) * dest_idx, 1, dest_scan_ctrl, &desc_buf[buf_idx]);
        if (va_status != VA_STATUS_SUCCESS) {
            ETRACE("vaCreateBuffer VASliceParameterBufferType failed. va_status = 0x%x", va_status);
            status = DECODE_DRIVER_FAIL;
            return status;
        }
        buf_idx++;
        va_status = vaCreateBuffer(jd_libva_ptr->va_display, jd_libva_ptr->va_context, VASliceDataBufferType, bytes, 1, &jd_libva_ptr->bitstream_buf[ src_offset ], &desc_buf[buf_idx]);
        buf_idx++;
        if (va_status != VA_STATUS_SUCCESS) {
            status = DECODE_DRIVER_FAIL;
            return status;
        }
        va_status = vaRenderPicture( jd_libva_ptr->va_display, jd_libva_ptr->va_context, desc_buf, buf_idx);
        if (va_status != VA_STATUS_SUCCESS) {
            ETRACE("vaRenderPicture failed. va_status = 0x%x", va_status);
            status = DECODE_DRIVER_FAIL;
            return status;
        }
        buf_idx = 0;

        src_offset += bytes;
    } while (bytes_remaining);

    va_status = vaEndPicture(jd_libva_ptr->va_display, jd_libva_ptr->va_context);
    if (va_status != VA_STATUS_SUCCESS) {
        ETRACE("vaRenderPicture failed. va_status = 0x%x", va_status);
        status = DECODE_DRIVER_FAIL;
        return status;
    }

    va_status = vaSyncSurface(jd_libva_ptr->va_display, jd_libva_ptr->va_surfaces[0]);
    if (va_status != VA_STATUS_SUCCESS) {
        WTRACE("vaSyncSurface failed. va_status = 0x%x", va_status);
    }

    status = doColorConversion(jd_libva_ptr,
                               jd_libva_ptr->va_surfaces[0],
                               buf, lines);

    ITRACE("Successfully decoded picture");
    return status;
cleanup:
    return DECODE_DRIVER_FAIL;
}

Decode_Status parseBitstream(j_decompress_ptr cinfo, jd_libva_struct * jd_libva_ptr) {
    uint32_t component_order = 0 ;
    uint32_t dqt_ind = 0;
    uint32_t dht_ind = 0;
    uint32_t scan_ind = 0;
    boolean frame_marker_found = FALSE;
    int i;

    uint8_t marker = jd_libva_ptr->JPEGParser->getNextMarker(jd_libva_ptr->JPEGParser);

    while (marker != CODE_EOI &&( !jd_libva_ptr->JPEGParser->endOfBuffer(jd_libva_ptr->JPEGParser))) {
        switch (marker) {
            case CODE_SOI: {
                 jd_libva_ptr->soi_offset = jd_libva_ptr->JPEGParser->getByteOffset(jd_libva_ptr->JPEGParser) - 2;
                break;
            }
            // If the marker is an APP marker skip over the data
            case CODE_APP0:
            case CODE_APP1:
            case CODE_APP2:
            case CODE_APP3:
            case CODE_APP4:
            case CODE_APP5:
            case CODE_APP6:
            case CODE_APP7:
            case CODE_APP8:
            case CODE_APP9:
            case CODE_APP10:
            case CODE_APP11:
            case CODE_APP12:
            case CODE_APP13:
            case CODE_APP14:
            case CODE_APP15: {

                uint32_t bytes_to_burn = jd_libva_ptr->JPEGParser->readBytes(jd_libva_ptr->JPEGParser, 2) - 2;
                jd_libva_ptr->JPEGParser->burnBytes(jd_libva_ptr->JPEGParser, bytes_to_burn);
                    break;
            }
            // Store offset to DQT data to avoid parsing bitstream in user mode
            case CODE_DQT: {
                if (dqt_ind < 4) {
                    jd_libva_ptr->dqt_byte_offset[dqt_ind] = jd_libva_ptr->JPEGParser->getByteOffset(jd_libva_ptr->JPEGParser) - jd_libva_ptr->soi_offset;
                    dqt_ind++;
                    uint32_t bytes_to_burn = jd_libva_ptr->JPEGParser->readBytes( jd_libva_ptr->JPEGParser, 2 ) - 2;
                    jd_libva_ptr->JPEGParser->burnBytes( jd_libva_ptr->JPEGParser, bytes_to_burn );
                } else {
                    ETRACE("ERROR: Decoder does not support more than 4 Quant Tables\n");
                    return DECODE_PARSER_FAIL;
                }
                break;
            }
            // Throw exception for all SOF marker other than SOF0
            case CODE_SOF1:
            case CODE_SOF2:
            case CODE_SOF3:
            case CODE_SOF5:
            case CODE_SOF6:
            case CODE_SOF7:
            case CODE_SOF8:
            case CODE_SOF9:
            case CODE_SOF10:
            case CODE_SOF11:
            case CODE_SOF13:
            case CODE_SOF14:
            case CODE_SOF15: {
                ETRACE("ERROR: unsupport SOF\n");
                break;
            }
            // Parse component information in SOF marker
            case CODE_SOF_BASELINE: {
                frame_marker_found = TRUE;

                jd_libva_ptr->JPEGParser->burnBytes(jd_libva_ptr->JPEGParser, 2); // Throw away frame header length
                uint8_t sample_precision = jd_libva_ptr->JPEGParser->readNextByte(jd_libva_ptr->JPEGParser);
                if (sample_precision != 8) {
                    ETRACE("sample_precision is not supported\n");
                    return DECODE_PARSER_FAIL;
                }
                // Extract pic width and height
                jd_libva_ptr->picture_param_buf.picture_height = jd_libva_ptr->JPEGParser->readBytes(jd_libva_ptr->JPEGParser, 2);
                jd_libva_ptr->picture_param_buf.picture_width = jd_libva_ptr->JPEGParser->readBytes(jd_libva_ptr->JPEGParser, 2);
                jd_libva_ptr->picture_param_buf.num_components = jd_libva_ptr->JPEGParser->readNextByte(jd_libva_ptr->JPEGParser);

                if (jd_libva_ptr->picture_param_buf.num_components > JPEG_MAX_COMPONENTS) {
                    ETRACE("ERROR: reached max components\n");
                    return DECODE_PARSER_FAIL;
                }
                if (jd_libva_ptr->picture_param_buf.picture_height < HW_DECODE_MIN_HEIGHT
                    || jd_libva_ptr->picture_param_buf.picture_width < HW_DECODE_MIN_WIDTH) {
                    ITRACE("PERFORMANCE: %ux%u JPEG will decode faster with SW\n",
                        jd_libva_ptr->picture_param_buf.picture_width,
                        jd_libva_ptr->picture_param_buf.picture_height);
                    return DECODE_PARSER_FAIL;
                }
                uint8_t comp_ind = 0;
                for (comp_ind = 0; comp_ind < jd_libva_ptr->picture_param_buf.num_components; comp_ind++) {
                    jd_libva_ptr->picture_param_buf.components[comp_ind].component_id = jd_libva_ptr->JPEGParser->readNextByte(jd_libva_ptr->JPEGParser);

                    uint8_t hv_sampling = jd_libva_ptr->JPEGParser->readNextByte(jd_libva_ptr->JPEGParser);
                    jd_libva_ptr->picture_param_buf.components[comp_ind].h_sampling_factor = hv_sampling >> 4;
                    jd_libva_ptr->picture_param_buf.components[comp_ind].v_sampling_factor = hv_sampling & 0xf;
                    jd_libva_ptr->picture_param_buf.components[comp_ind].quantiser_table_selector = jd_libva_ptr->JPEGParser->readNextByte(jd_libva_ptr->JPEGParser);
                }


                break;
            }
            // Store offset to DHT data to avoid parsing bitstream in user mode
            case CODE_DHT: {
                if (dht_ind < 4) {
                    jd_libva_ptr->dht_byte_offset[dht_ind] = jd_libva_ptr->JPEGParser->getByteOffset(jd_libva_ptr->JPEGParser) - jd_libva_ptr->soi_offset;
                    dht_ind++;
                    uint32_t bytes_to_burn = jd_libva_ptr->JPEGParser->readBytes(jd_libva_ptr->JPEGParser, 2) - 2;
                    jd_libva_ptr->JPEGParser->burnBytes(jd_libva_ptr->JPEGParser,  bytes_to_burn );
                } else {
                    ETRACE("ERROR: Decoder does not support more than 4 Huff Tables\n");
                    return DECODE_PARSER_FAIL;
                }
                break;
            }
            // Parse component information in SOS marker
            case CODE_SOS: {
                jd_libva_ptr->JPEGParser->burnBytes(jd_libva_ptr->JPEGParser, 2);
                uint32_t component_in_scan = jd_libva_ptr->JPEGParser->readNextByte(jd_libva_ptr->JPEGParser);
                uint8_t comp_ind = 0;

                for (comp_ind = 0; comp_ind < component_in_scan; comp_ind++) {
                    uint8_t comp_id = jd_libva_ptr->JPEGParser->readNextByte(jd_libva_ptr->JPEGParser);
                    uint8_t comp_data_ind;
                    for (comp_data_ind = 0; comp_data_ind < jd_libva_ptr->picture_param_buf.num_components; comp_data_ind++) {
                        if (comp_id == jd_libva_ptr->picture_param_buf.components[comp_data_ind].component_id) {
                            jd_libva_ptr->slice_param_buf[scan_ind].components[comp_ind].component_selector = comp_data_ind + 1;
                            break;
                        }
                    }
                    uint8_t huffman_tables = jd_libva_ptr->JPEGParser->readNextByte(jd_libva_ptr->JPEGParser);
                    jd_libva_ptr->slice_param_buf[scan_ind].components[comp_ind].dc_table_selector = huffman_tables >> 4;
                    jd_libva_ptr->slice_param_buf[scan_ind].components[comp_ind].ac_table_selector = huffman_tables & 0xf;
                }
                uint32_t curr_byte = jd_libva_ptr->JPEGParser->readNextByte(jd_libva_ptr->JPEGParser); // Ss
                if (curr_byte != 0) {
                    ETRACE("ERROR: curr_byte 0x%08x != 0\n", curr_byte);
                    return DECODE_PARSER_FAIL;
                }
                curr_byte = jd_libva_ptr->JPEGParser->readNextByte(jd_libva_ptr->JPEGParser);  // Se
                if (curr_byte != 0x3f) {
                    ETRACE("ERROR: curr_byte 0x%08x != 0x3f\n", curr_byte);
                    return DECODE_PARSER_FAIL;
                }
                curr_byte = jd_libva_ptr->JPEGParser->readNextByte(jd_libva_ptr->JPEGParser);  // Ah, Al
                if (curr_byte != 0) {
                    ETRACE("ERROR: curr_byte 0x%08x != 0\n", curr_byte);
                    return DECODE_PARSER_FAIL;
                }
                // Set slice control variables needed
                jd_libva_ptr->slice_param_buf[scan_ind].slice_data_offset = jd_libva_ptr->JPEGParser->getByteOffset(jd_libva_ptr->JPEGParser) - jd_libva_ptr->soi_offset;
                jd_libva_ptr->slice_param_buf[scan_ind].num_components = component_in_scan;
                if (scan_ind) {
                    /* If there is more than one scan, the slice for all but the final scan should only run up to the beginning of the next scan */
                    jd_libva_ptr->slice_param_buf[scan_ind - 1].slice_data_size =
                        (jd_libva_ptr->slice_param_buf[scan_ind].slice_data_offset - jd_libva_ptr->slice_param_buf[scan_ind - 1].slice_data_offset );;
                    }
                    scan_ind++;
                    jd_libva_ptr->scan_ctrl_count++;   // gsDXVA2Globals.uiScanCtrlCount
                    break;
                }
            case CODE_DRI: {
                uint32_t size =  jd_libva_ptr->JPEGParser->readBytes(jd_libva_ptr->JPEGParser, 2);
                jd_libva_ptr->slice_param_buf[scan_ind].restart_interval =  jd_libva_ptr->JPEGParser->readBytes(jd_libva_ptr->JPEGParser, 2);
                jd_libva_ptr->JPEGParser->burnBytes(jd_libva_ptr->JPEGParser, (size - 4));
                break;
            }
            default:
                break;
        }

        marker = jd_libva_ptr->JPEGParser->getNextMarker(jd_libva_ptr->JPEGParser);
        // If the EOI code is found, store the byte offset before the parsing finishes
        if( marker == CODE_EOI ) {
            jd_libva_ptr->eoi_offset = jd_libva_ptr->JPEGParser->getByteOffset(jd_libva_ptr->JPEGParser);
        }

    }

    jd_libva_ptr->quant_tables_num = dqt_ind;
    jd_libva_ptr->huffman_tables_num = dht_ind;

    /* The slice for the last scan should run up to the end of the picture */
    if (jd_libva_ptr->eoi_offset) {
        jd_libva_ptr->slice_param_buf[scan_ind - 1].slice_data_size = (jd_libva_ptr->eoi_offset - jd_libva_ptr->slice_param_buf[scan_ind - 1].slice_data_offset);
    }
    else {
        jd_libva_ptr->slice_param_buf[scan_ind - 1].slice_data_size = (jd_libva_ptr->file_size - jd_libva_ptr->slice_param_buf[scan_ind - 1].slice_data_offset);
    }
    // throw AppException if SOF0 isn't found
    if (!frame_marker_found) {
        ETRACE("EEORR: Reached end of bitstream while trying to parse headers\n");
        return DECODE_PARSER_FAIL;
    }

    Decode_Status status = parseTableData(cinfo, jd_libva_ptr);
    if (status != DECODE_SUCCESS) {
        ETRACE("ERROR: Parsing table data returns %d", status);
    }
    cinfo->original_image_width = jd_libva_ptr->picture_param_buf.picture_width;  /* nominal image width (from SOF marker) */
    cinfo->image_width = jd_libva_ptr->picture_param_buf.picture_width;   /* nominal image width (from SOF marker) */
    cinfo->image_height = jd_libva_ptr->picture_param_buf.picture_height;  /* nominal image height */
    cinfo->num_components = jd_libva_ptr->picture_param_buf.num_components;       /* # of color components in JPEG image */
    cinfo->jpeg_color_space = JCS_YCbCr; /* colorspace of JPEG image */
    cinfo->out_color_space = JCS_RGB; /* colorspace for output */
    cinfo->src->bytes_in_buffer = jd_libva_ptr->file_size;

    ITRACE("Successfully parsed table");
    return status;

}

Decode_Status parseTableData(j_decompress_ptr cinfo, jd_libva_struct * jd_libva_ptr) {
    CJPEGParse* parser = (CJPEGParse*)malloc(sizeof(CJPEGParse));
    if (parser == NULL) {
        ETRACE("%s ERROR: Parsing table data returns %d", __FUNCTION__, DECODE_MEMORY_FAIL);
        return DECODE_MEMORY_FAIL;
    }

    parserInitialize(parser, jd_libva_ptr->bitstream_buf, jd_libva_ptr->file_size);

    // Parse Quant tables
    memset(&jd_libva_ptr->qmatrix_buf, 0, sizeof(jd_libva_ptr->qmatrix_buf));
    uint32_t dqt_ind = 0;
    for (dqt_ind = 0; dqt_ind < jd_libva_ptr->quant_tables_num; dqt_ind++) {
        if (parser->setByteOffset(parser, jd_libva_ptr->dqt_byte_offset[dqt_ind])) {
            // uint32_t uiTableBytes = parser->readBytes( 2 ) - 2;
            uint32_t table_bytes = parser->readBytes( parser, 2 ) - 2;
            do {
                uint32_t table_info = parser->readNextByte(parser);
                table_bytes--;
                uint32_t table_length = table_bytes > 64 ? 64 : table_bytes;
                uint32_t table_precision = table_info >> 4;
                if (table_precision != 0) {
                    ETRACE("%s ERROR: Parsing table data returns %d", __FUNCTION__, DECODE_PARSER_FAIL);
                    return DECODE_PARSER_FAIL;
                }
                uint32_t table_id = table_info & 0xf;

                jd_libva_ptr->qmatrix_buf.load_quantiser_table[table_id] = 1;

                if (table_id < JPEG_MAX_QUANT_TABLES) {
                    // Pull Quant table data from bitstream
                    uint32_t byte_ind;
                    for (byte_ind = 0; byte_ind < table_length; byte_ind++) {
                        jd_libva_ptr->qmatrix_buf.quantiser_table[table_id][byte_ind] = parser->readNextByte(parser);
                    }
                } else {
                    ETRACE("%s DQT table ID is not supported", __FUNCTION__);
                    parser->burnBytes(parser, table_length);
                }
                table_bytes -= table_length;
            } while (table_bytes);
        }
    }

    // Parse Huffman tables
    memset(&jd_libva_ptr->hufman_table_buf, 0, sizeof(jd_libva_ptr->hufman_table_buf));
    uint32_t dht_ind = 0;
    for (dht_ind = 0; dht_ind < jd_libva_ptr->huffman_tables_num; dht_ind++) {
        if (parser->setByteOffset(parser, jd_libva_ptr->dht_byte_offset[dht_ind])) {
            uint32_t table_bytes = parser->readBytes( parser, 2 ) - 2;
            do {
                uint32_t table_info = parser->readNextByte(parser);
                table_bytes--;
                uint32_t table_class = table_info >> 4; // Identifies whether the table is for AC or DC
                uint32_t table_id = table_info & 0xf;
                jd_libva_ptr->hufman_table_buf.load_huffman_table[table_id] = 1;

                if ((table_class < TABLE_CLASS_NUM) && (table_id < JPEG_MAX_SETS_HUFFMAN_TABLES)) {
                    if (table_class == 0) {
                        uint8_t* bits = parser->getCurrentIndex(parser);
                        // Find out the number of entries in the table
                        uint32_t table_entries = 0;
                        uint32_t bit_ind;
                        for (bit_ind = 0; bit_ind < 16; bit_ind++) {
                            jd_libva_ptr->hufman_table_buf.huffman_table[table_id].num_dc_codes[bit_ind] = bits[bit_ind];
                            table_entries += jd_libva_ptr->hufman_table_buf.huffman_table[table_id].num_dc_codes[bit_ind];
                        }

                        // Create table of code values
                        parser->burnBytes(parser, 16);
                        table_bytes -= 16;
                        uint32_t tbl_ind;
                        for (tbl_ind = 0; tbl_ind < table_entries; tbl_ind++) {
                            jd_libva_ptr->hufman_table_buf.huffman_table[table_id].dc_values[tbl_ind] = parser->readNextByte(parser);
                            table_bytes--;
                        }

                    } else { // for AC class
                        uint8_t* bits = parser->getCurrentIndex(parser);
                        // Find out the number of entries in the table
                        uint32_t table_entries = 0;
                        uint32_t bit_ind = 0;
                        for (bit_ind = 0; bit_ind < 16; bit_ind++) {
                            jd_libva_ptr->hufman_table_buf.huffman_table[table_id].num_ac_codes[bit_ind] = bits[bit_ind];
                            table_entries += jd_libva_ptr->hufman_table_buf.huffman_table[table_id].num_ac_codes[bit_ind];
                        }

                        // Create table of code values
                        parser->burnBytes(parser, 16);
                        table_bytes -= 16;
                        uint32_t tbl_ind = 0;
                        for (tbl_ind = 0; tbl_ind < table_entries; tbl_ind++) {
                            jd_libva_ptr->hufman_table_buf.huffman_table[table_id].ac_values[tbl_ind] = parser->readNextByte(parser);
                            table_bytes--;
                        }
                    }//end of else
                } else {
                    // Find out the number of entries in the table
                    ETRACE("%s DHT table ID is not supported", __FUNCTION__);
                    uint32_t table_entries = 0;
                    uint32_t bit_ind = 0;
                    for(bit_ind = 0; bit_ind < 16; bit_ind++) {
                        table_entries += parser->readNextByte(parser);
                        table_bytes--;
                    }
                    parser->burnBytes(parser, table_entries);
                    table_bytes -= table_entries;
		}

            } while (table_bytes);
        }
    }

    if (parser) {
        free(parser);
        parser = NULL;
    }
    return DECODE_SUCCESS;
}

