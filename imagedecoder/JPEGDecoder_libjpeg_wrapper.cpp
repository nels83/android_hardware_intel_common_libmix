/* INTEL CONFIDENTIAL
* Copyright (c) 2012, 2013 Intel Corporation.  All rights reserved.
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

/*
 * Initialize VA API related stuff
 *
 * We will check the return value of  jva_initialize
 * to determine which path will be use (SW or HW)
 *
 */
//#define LOG_NDEBUG 0
#define LOG_TAG "ImageDecoder"

#include <utils/Log.h>
#include "JPEGDecoder_libjpeg_wrapper.h"
#include <hardware/gralloc.h>
#include <utils/threads.h>
#include "JPEGDecoder.h"
#include <va/va.h>
#include "va/va_dec_jpeg.h"

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <assert.h>

static Mutex jdlock;

struct jdva_private
{
    JpegInfo jpg_info;
    JpegDecoder decoder;
    RenderTarget dec_buffer;
    RenderTarget yuy2_buffer;
    RenderTarget rgba_buffer;
};

static int internal_buffer_handle = 0;

#define JD_CHECK(err, label) \
        if (err) { \
            ALOGE("%s::%d: failed: %d", __PRETTY_FUNCTION__, __LINE__, err); \
            goto label; \
        }

#define JD_CHECK_RET(err, label, retcode) \
        if (err) { \
            status = retcode; \
            ALOGE("%s::%d: failed: %d", __PRETTY_FUNCTION__, __LINE__, err); \
            goto label; \
        }

Decode_Status jdva_initialize (jd_libva_struct * jd_libva_ptr)
{
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

    if (jd_libva_ptr->initialized) {
        ALOGW("%s HW decode already initialized", __FUNCTION__);
        return DECODE_NOT_STARTED;
    }

    {
        Mutex::Autolock autoLock(jdlock);
        if (!(jd_libva_ptr->initialized)) {
            jdva_private *priv = new jdva_private;
            memset(&priv->jpg_info, 0, sizeof(JpegInfo));
            memset(&priv->dec_buffer, 0, sizeof(RenderTarget));
            memset(&priv->yuy2_buffer, 0, sizeof(RenderTarget));
            memset(&priv->rgba_buffer, 0, sizeof(RenderTarget));
            jd_libva_ptr->initialized = TRUE;
            jd_libva_ptr->priv = (uint32_t)priv;
            status = DECODE_SUCCESS;
        }
    }
cleanup:
    if (status) {
        jd_libva_ptr->initialized = TRUE; // make sure we can call into jva_deinitialize()
        jdva_deinitialize (jd_libva_ptr);
        return status;
    }

  return status;
}
void jdva_deinitialize (jd_libva_struct * jd_libva_ptr)
{
    if (!(jd_libva_ptr->initialized)) {
        return;
    }
    {
        Mutex::Autolock autoLock(jdlock);
        if (jd_libva_ptr->initialized) {
            jdva_private *p = (jdva_private*)jd_libva_ptr->priv;
            delete p;
            jd_libva_ptr->initialized = FALSE;
        }
    }
    ALOGV("jdva_deinitialize finished");
    return;
}

RenderTarget * create_render_target(RenderTarget* target, int width, int height, int pixel_format)
{
    hw_module_t const* module = NULL;
    alloc_device_t *allocdev = NULL;
    struct gralloc_module_t *gralloc_module = NULL;
    buffer_handle_t handle;
    uint32_t fourcc;
    int stride, bpp, err;
    fourcc = pixelFormat2Fourcc(pixel_format);
    bpp = fourcc2LumaBitsPerPixel(fourcc);
    if (target == NULL) {
        ALOGE("%s malloc new RenderTarget failed", __FUNCTION__);
        return NULL;
    }
    ALOGV("%s created %s target %p", __FUNCTION__, fourcc2str(NULL, fourcc), target);
    if ((fourcc == VA_FOURCC_422H) ||
        (fourcc == VA_FOURCC_YUY2) ||
        (fourcc == VA_FOURCC_RGBA)){
        err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);
        if (err || !module) {
            ALOGE("%s failed to get gralloc module", __FUNCTION__);
            return NULL;
        }
        gralloc_module = (struct gralloc_module_t *)module;
        err = gralloc_open(module, &allocdev);
        if (err || !allocdev) {
            ALOGE("%s failed to open alloc device", __FUNCTION__);
            return NULL;
        }
        err = allocdev->alloc(allocdev,
                width, height, pixel_format,
                GRALLOC_USAGE_HW_RENDER,
                &handle, &stride);
        if (err) {
            gralloc_close(allocdev);
            ALOGE("%s failed to allocate surface", __FUNCTION__);
            return NULL;
        }
        target->type = RenderTarget::ANDROID_GRALLOC;
        target->handle = (int)handle;
        target->stride = stride * bpp;
    }
    else {
        *((int*)(&target->type)) = RENDERTARGET_INTERNAL_BUFFER;
        target->handle = internal_buffer_handle++;
    }
    target->width = width;
    target->height = height;
    target->pixel_format = pixel_format;
    target->rect.x = target->rect.y = 0;
    target->rect.width = target->width;
    target->rect.height = target->height;
    return target;
}

void free_render_target(RenderTarget *target)
{
    if (target == NULL)
        return;
    uint32_t fourcc = pixelFormat2Fourcc(target->pixel_format);
    if (target->type == RenderTarget::ANDROID_GRALLOC) {
        buffer_handle_t handle = (buffer_handle_t)target->handle;
        hw_module_t const* module = NULL;
        alloc_device_t *allocdev = NULL;
        struct gralloc_module_t *gralloc_module = NULL;
        int err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);
        if (err || !module) {
            ALOGE("%s failed to get gralloc module", __FUNCTION__);
            return;
        }
        gralloc_module = (struct gralloc_module_t *)module;
        err = gralloc_open(module, &allocdev);
        if (err || !allocdev) {
            ALOGE("%s failed to get gralloc module", __FUNCTION__);
            return;
        }
        allocdev->free(allocdev, handle);
        gralloc_close(allocdev);
    }
    ALOGV("%s deleting %s target %p", __FUNCTION__, fourcc2str(NULL, fourcc), target);
}

void dump_yuy2_target(RenderTarget *target, JpegDecoder *decoder, const char *filename)
{
    uint32_t fourcc = pixelFormat2Fourcc(target->pixel_format);
    assert(fourcc == VA_FOURCC_YUY2);
    uint8_t *data;
    uint32_t offsets[3];
    uint32_t pitches[3];
    JpegDecoder::MapHandle maphandle = decoder->mapData(*target, (void**) &data, offsets, pitches);
    assert (maphandle.valid);
    FILE* fpdump = fopen(filename, "wb");
    if (fpdump) {
        // YUYV
        for (int i = 0; i < target->height; ++i) {
            fwrite(data + offsets[0] + i * pitches[0], 1, target->width * 2, fpdump);
        }
        fclose(fpdump);
    }
    else {
        ALOGW("%s failed to create %s", __FUNCTION__, filename);
    }
    decoder->unmapData(*target, maphandle);
}

void dump_dec_target(RenderTarget *target, JpegDecoder *decoder, const char *filename)
{
    uint32_t fourcc = pixelFormat2Fourcc(target->pixel_format);
    assert((fourcc == VA_FOURCC_IMC3) ||
        (fourcc == VA_FOURCC_411P) ||
        (fourcc == VA_FOURCC('4','0','0','P')) ||
        (fourcc == VA_FOURCC_422H) ||
        (fourcc == VA_FOURCC_422V) ||
        (fourcc == VA_FOURCC_444P));
    uint8_t *data;
    uint32_t offsets[3];
    uint32_t pitches[3];
    JpegDecoder::MapHandle maphandle = decoder->mapData(*target, (void**) &data, offsets, pitches);
    assert (maphandle.valid);
    FILE* fpdump = fopen(filename, "wb");
    if(fpdump) {
        float hfactor, vfactor;
        switch (fourcc) {
            case VA_FOURCC_IMC3:
                hfactor = 1;
                vfactor = 0.5;
                break;
            case VA_FOURCC_444P:
                hfactor = vfactor = 1;
                break;
            case VA_FOURCC_422H:
                hfactor = 0.5;
                vfactor = 1;
                break;
            case VA_FOURCC('4','0','0','P'):
                hfactor = vfactor = 0;
                break;
            case VA_FOURCC_411P:
                hfactor = 0.25;
                vfactor = 1;
                break;
            case VA_FOURCC_422V:
                hfactor = 0.5;
                vfactor = 1;
                break;
            default:
                hfactor = vfactor = 1;
                break;
        }
        // Y
        for (int i = 0; i < target->height; ++i) {
            fwrite(data + offsets[0] + i * pitches[0], 1, target->width, fpdump);
        }
        // U
        for (int i = 0; i < target->height * vfactor; ++i) {
            fwrite(data + offsets[1] + i * pitches[1], 1, target->width * hfactor, fpdump);
        }
        // V
        for (int i = 0; i < target->height * vfactor; ++i) {
            fwrite(data + offsets[2] + i * pitches[2], 1, target->width * hfactor, fpdump);
        }
        fclose(fpdump);
    }
    else {
        ALOGW("%s failed to create %s", __FUNCTION__, filename);
    }
    decoder->unmapData(*target, maphandle);
}


Decode_Status jdva_decode (j_decompress_ptr cinfo, jd_libva_struct * jd_libva_ptr)
{
    JpegDecodeStatus st;
    char **outbuf = jd_libva_ptr->output_image;
    uint32_t lines = jd_libva_ptr->output_lines;
    jdva_private * priv = (jdva_private*)jd_libva_ptr->priv;
    if (!priv)
        return DECODE_DRIVER_FAIL;

    JpegInfo& jpginfo = priv->jpg_info;

    st = priv->decoder.decode(jpginfo, priv->dec_buffer);
    if (st != JD_SUCCESS) {
        ALOGE("%s: error decoding %s image", __FUNCTION__, fourcc2str(NULL, jpginfo.image_color_fourcc));
        return DECODE_DRIVER_FAIL;
    }
    ALOGI("%s successfully decoded JPEG with VAAPI", __FUNCTION__);
    RenderTarget *src_target = &priv->dec_buffer;
    //dump_dec_target(src_target, decoder,"/sdcard/dec_dump.yuv");

    bool yuy2_csc = false;
    hw_module_t const* module = NULL;
    alloc_device_t *allocdev = NULL;
    struct gralloc_module_t *gralloc_module = NULL;
    buffer_handle_t handle;
    int err;
    uint8_t *data = NULL;
    uint32_t offsets[3];
    uint32_t pitches[3];
    JpegDecoder::MapHandle maphandle;
    FILE *rgbafile = NULL;
    if (jpginfo.image_color_fourcc != VA_FOURCC_422H)
        yuy2_csc = true;

    // CSC to YUY2 if needed
    if (yuy2_csc) {
        st = priv->decoder.blit(*src_target, priv->yuy2_buffer);
        if (st != JD_SUCCESS) {
            ALOGE("%s: error blitting to YUY2 buffer", __FUNCTION__);
            goto cleanup;
        }
        //dump_yuy2_target(src_target, decoder,"/sdcard/yuy2_dump.yuv");
        src_target = &priv->yuy2_buffer;
    }

    st = priv->decoder.blit(*src_target, priv->rgba_buffer);
    if (st != JD_SUCCESS) {
        ALOGE("%s: error blitting to RGBA buffer", __FUNCTION__);
        goto cleanup;
    }
    maphandle = priv->decoder.mapData(priv->rgba_buffer, (void**) &data, offsets, pitches);

    //rgbafile = fopen("/sdcard/rgba_dump", "wb");

    for (uint32_t i = 0; i < lines; ++i) {
        if (outbuf[i] != NULL) {
            //memcpy(outbuf[i], data + offsets[0] + i * pitches[0], 4 * jpginfo.image_width);
            for (int j = 0; j < priv->rgba_buffer.width; ++j) {
                // BGRA -> RGBA
                // R
                memcpy(outbuf[i] + 4 * j, data + offsets[0] + i * pitches[0] + 4 * j + 2, 1);
                // G
                memcpy(outbuf[i] + 4 * j + 1, data + offsets[0] + i * pitches[0] + 4 * j + 1, 1);
                // B
                memcpy(outbuf[i] + 4 * j + 2, data + offsets[0] + i * pitches[0] + 4 * j, 1);
                // A
                memcpy(outbuf[i] + 4 * j + 3, data + offsets[0] + i * pitches[0] + 4 * j + 3, 1);
            }
        }
        else {
            ALOGE("%s outbuf line %u is NULL", __FUNCTION__, i);
        }
        //if (rgbafile) {
        //    fwrite(data + offsets[0] + i * pitches[0], 1, 4 * rgba_target->width, rgbafile);
        //}
    }
    //if (rgbafile)
    //    fclose(rgbafile);
    ALOGI("%s successfully blitted RGBA from JPEG %s data", __FUNCTION__, fourcc2str(NULL, priv->jpg_info.image_color_fourcc));
    priv->decoder.unmapData(priv->rgba_buffer, maphandle);
    return DECODE_SUCCESS;

cleanup:
    return DECODE_DRIVER_FAIL;
}

Decode_Status jdva_create_resource (jd_libva_struct * jd_libva_ptr)
{
    VAStatus va_status = VA_STATUS_SUCCESS;
    Decode_Status status = DECODE_SUCCESS;
    RenderTarget *dec_target, *yuy2_target, *rgba_target;
    dec_target = yuy2_target = rgba_target = NULL;
    JpegDecodeStatus st;
    Mutex::Autolock autoLock(jdlock);
    jdva_private *priv = (jdva_private*)jd_libva_ptr->priv;
    jd_libva_ptr->image_width = priv->jpg_info.picture_param_buf.picture_width;
    jd_libva_ptr->image_height = priv->jpg_info.picture_param_buf.picture_height;
    dec_target = create_render_target(&priv->dec_buffer, jd_libva_ptr->image_width,jd_libva_ptr->image_height,fourcc2PixelFormat(priv->jpg_info.image_color_fourcc));
    if (dec_target == NULL) {
        ALOGE("%s failed to create decode render target", __FUNCTION__);
        return DECODE_MEMORY_FAIL;
    }
    rgba_target = create_render_target(&priv->rgba_buffer, jd_libva_ptr->image_width,jd_libva_ptr->image_height, HAL_PIXEL_FORMAT_RGBA_8888);
    if (rgba_target == NULL) {
        ALOGE("%s failed to create YUY2 csc buffer", __FUNCTION__);
        free_render_target(dec_target);
        return DECODE_MEMORY_FAIL;
    }
    yuy2_target = create_render_target(&priv->yuy2_buffer, jd_libva_ptr->image_width,jd_libva_ptr->image_height, HAL_PIXEL_FORMAT_YCbCr_422_I);
    if (yuy2_target == NULL) {
        ALOGE("%s failed to create RGBA csc buffer", __FUNCTION__);
        free_render_target(dec_target);
        free_render_target(rgba_target);
        return DECODE_MEMORY_FAIL;
    }
    RenderTarget *targetlist[3] = { dec_target, yuy2_target, rgba_target };
    st = priv->decoder.init(jd_libva_ptr->image_width, jd_libva_ptr->image_height, targetlist, 3);
    if (st != JD_SUCCESS) {
        free_render_target(dec_target);
        free_render_target(rgba_target);
        free_render_target(yuy2_target);
        ALOGE("%s failed to initialize resources for decoder: %d", __FUNCTION__, st);
        return DECODE_DRIVER_FAIL;
    }

    jd_libva_ptr->resource_allocated = TRUE;
    ALOGV("%s successfully set up HW decode resource", __FUNCTION__);
    return status;
cleanup:
    jd_libva_ptr->resource_allocated = FALSE;

    jdva_deinitialize (jd_libva_ptr);

    return DECODE_DRIVER_FAIL;
}
Decode_Status jdva_release_resource (jd_libva_struct * jd_libva_ptr)
{
    Decode_Status status = DECODE_SUCCESS;
    VAStatus va_status = VA_STATUS_SUCCESS;
    int i;

    if (!(jd_libva_ptr->resource_allocated)) {
        ALOGW("%s decoder resource not yet allocated", __FUNCTION__);
        return status;
    }
    Mutex::Autolock autoLock(jdlock);

    ALOGV("%s deiniting priv 0x%x", __FUNCTION__, jd_libva_ptr->priv);
    jdva_private *priv = (jdva_private*)jd_libva_ptr->priv;
    if (priv) {
        priv->decoder.deinit();
        free_render_target(&priv->dec_buffer);
        free_render_target(&priv->yuy2_buffer);
        free_render_target(&priv->rgba_buffer);
    }
  /*
   * It is safe to destroy Surface/Config/Context severl times
   * and it is also safe even their value is NULL
   */

cleanup:

    jd_libva_ptr->resource_allocated = FALSE;

    return va_status;
}
Decode_Status jdva_parse_bitstream(j_decompress_ptr cinfo, jd_libva_struct * jd_libva_ptr)
{
    jdva_private * priv = (jdva_private*)jd_libva_ptr->priv;
    if (!priv)
        return DECODE_DRIVER_FAIL;
    JpegInfo& jpginfo = priv->jpg_info;
    jpginfo.buf = jd_libva_ptr->bitstream_buf;
    jpginfo.bufsize = jd_libva_ptr->file_size;
    JpegDecodeStatus st = priv->decoder.parse(jpginfo);
    if (st != JD_SUCCESS) {
        ALOGE("%s parser for HW decode failed: %d", __FUNCTION__, st);
        return DECODE_PARSER_FAIL;
    }

    jd_libva_ptr->image_width = jpginfo.image_width;
    jd_libva_ptr->image_height = jpginfo.image_height;
    cinfo->original_image_width = jpginfo.picture_param_buf.picture_width;  /* nominal image width (from SOF marker) */
    cinfo->image_width = jpginfo.picture_param_buf.picture_width;   /* nominal image width (from SOF marker) */
    cinfo->image_height = jpginfo.picture_param_buf.picture_height;  /* nominal image height */
    cinfo->num_components = jpginfo.picture_param_buf.num_components;       /* # of color components in JPEG image */
    cinfo->jpeg_color_space = JCS_YCbCr; /* colorspace of JPEG image */
    cinfo->out_color_space = JCS_RGB; /* colorspace for output */
    cinfo->src->bytes_in_buffer = jd_libva_ptr->file_size;
    return DECODE_SUCCESS;
}

