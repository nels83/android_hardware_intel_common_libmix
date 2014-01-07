/* INTEL CONFIDENTIAL
* Copyright (c) 2012, 2013 Intel Corporation.  All rights reserved.
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

#include <utils/Log.h>
#include "JPEGDecoder_libjpeg_wrapper.h"
#include <hardware/gralloc.h>
#include <utils/threads.h>
#include "JPEGDecoder.h"
#include <va/va.h>
#include <va/va_android.h>
#include "va/va_dec_jpeg.h"
#include <utils/Timers.h>
#ifdef NDEBUG
#undef NDEBUG
#endif

#include <assert.h>
#include <utils/Vector.h>
static Mutex jdlock;
static VADisplay display = NULL;
static VAConfigID vpCfgId = VA_INVALID_ID;
static VAContextID vpCtxId = VA_INVALID_ID;

#define DUMP_DECODE 0
#define DUMP_RGBA 0
#define RGBA_DUMP_FILE_PATTERN "/sdcard/jpeg_%dx%d_from_%s.rgba"
#define DECODE_DUMP_FILE_PATTERN "/sdcard/jpeg_%dx%d.%s"

using namespace android;

struct jdva_private
{
    JpegInfo jpg_info;
    android::Vector<uint8_t> inputs;
    JpegDecoder *decoder;
    RenderTarget dec_buffer;
    uint8_t* rgba_out;
    BlitEvent blit_event;
    int tile_read_x;
    int tile_read_y;
    int tile_read_width;
    int tile_read_height;
    int scale_factor;
};

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

static void libva_vp_pre_init_locked()
{
    if (display == NULL && vpCfgId == VA_INVALID_ID && vpCtxId == VA_INVALID_ID) {
        Display dpy;
        int va_major_version, va_minor_version;
        VAConfigAttrib  vpp_attrib;
        VAStatus st;
        display = vaGetDisplay(&dpy);
        st = vaInitialize(display, &va_major_version, &va_minor_version);
        assert(st == VA_STATUS_SUCCESS);
        vpp_attrib.type  = VAConfigAttribRTFormat;
        vpp_attrib.value = VA_RT_FORMAT_YUV420;
        st = vaCreateConfig(display, VAProfileNone,
                                    VAEntrypointVideoProc,
                                    &vpp_attrib,
                                    1, &vpCfgId);
        assert(st == VA_STATUS_SUCCESS);
        st = vaCreateContext(display, vpCfgId, 1920, 1080, 0, NULL, 0, &vpCtxId);
        assert(st == VA_STATUS_SUCCESS);
    }
}

/* clear the global VA context
 * actually it's not needed
 * when the process terminates, the drm fd will be closed by kernel and the VA
 * context will be automatically released
 */
static void libva_vp_post_deinit_locked()
{
    // DO NOTHING
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

    Mutex::Autolock autoLock(jdlock);

    if (display == NULL || vpCfgId == VA_INVALID_ID || vpCtxId == VA_INVALID_ID) {
        libva_vp_pre_init_locked();
    }

    if (jd_libva_ptr->initialized) {
        ALOGW("%s HW decode already initialized", __FUNCTION__);
        return DECODE_NOT_STARTED;
    }

    {
        if (!(jd_libva_ptr->initialized)) {
            jdva_private *priv = new jdva_private;
            memset(&priv->jpg_info, 0, sizeof(JpegInfo));
            priv->jpg_info.use_vector_input = true;
            memset(&priv->dec_buffer, 0, sizeof(RenderTarget));
            priv->rgba_out = NULL;
            priv->inputs.clear();
            priv->jpg_info.inputs = &priv->inputs;
            jd_libva_ptr->initialized = TRUE;
            jd_libva_ptr->priv = (uint32_t)priv;
            jd_libva_ptr->cap_available= 0x0;
            jd_libva_ptr->cap_available |= JPEG_CAPABILITY_DECODE;
#ifdef GFXGEN
            jd_libva_ptr->cap_available |= JPEG_CAPABILITY_UPSAMPLE | JPEG_CAPABILITY_DOWNSCALE;
#endif
            jd_libva_ptr->cap_enabled = jd_libva_ptr->cap_available;
            if (jd_libva_ptr->cap_available & JPEG_CAPABILITY_UPSAMPLE)
                priv->decoder = new JpegDecoder(display, vpCfgId, vpCtxId, true);
            else
                priv->decoder = new JpegDecoder();
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
            delete p->decoder;
            jd_libva_ptr->bitstream_buf = NULL;
            p->inputs.clear();
            delete p;
            jd_libva_ptr->initialized = FALSE;
        }
    }
    return;
}

Decode_Status jdva_fill_input(j_decompress_ptr cinfo, jd_libva_struct * jd_libva_ptr)
{
    jdva_private *p = (jdva_private*)jd_libva_ptr->priv;
    if ((*cinfo->src->fill_input_buffer)(cinfo)) {
        assert(cinfo->src->next_input_byte);
        assert(cinfo->src->bytes_in_buffer);
        p->inputs.appendArray(cinfo->src->next_input_byte, cinfo->src->bytes_in_buffer);
        jd_libva_ptr->file_size += cinfo->src->bytes_in_buffer;
        ALOGV("%s read %d bytes, file_size %u bytes, vector %u bytes", __FUNCTION__, cinfo->src->bytes_in_buffer, jd_libva_ptr->file_size, p->inputs.size());
        cinfo->src->bytes_in_buffer = 0;
    }
    else {
        return DECODE_DRIVER_FAIL;
    }
    return DECODE_SUCCESS;
}

void jdva_drain_input(j_decompress_ptr cinfo, jd_libva_struct * jd_libva_ptr)
{
    nsecs_t now = systemTime();
    jdva_private *p = (jdva_private*)jd_libva_ptr->priv;
    do {
      if ((*cinfo->src->fill_input_buffer)(cinfo)) {
          p->inputs.appendArray(cinfo->src->next_input_byte, cinfo->src->bytes_in_buffer);
          jd_libva_ptr->file_size += cinfo->src->bytes_in_buffer;
      }
      else {
          break;
      }
    } while (cinfo->src->bytes_in_buffer > 0);
    jd_libva_ptr->bitstream_buf = p->inputs.array();
    ALOGV("%s drained input %u bytes took %.2f ms", __FUNCTION__, jd_libva_ptr->file_size,
        (systemTime() - now)/1000000.0);
}

RenderTarget * create_render_target(RenderTarget* target, int width, int height, uint32_t fourcc)
{
    hw_module_t const* module = NULL;
    alloc_device_t *allocdev = NULL;
    struct gralloc_module_t *gralloc_module = NULL;
    buffer_handle_t handle;
    int stride, bpp, err;
    bpp = fourcc2LumaBitsPerPixel(fourcc);
    if (target == NULL) {
        ALOGE("%s malloc new RenderTarget failed", __FUNCTION__);
        return NULL;
    }
    ALOGV("%s created %s target %p", __FUNCTION__, fourcc2str(fourcc), target);
    target->type = RenderTarget::INTERNAL_BUF;
    target->handle = generateHandle();
    target->width = width;
    target->height = height;
    target->pixel_format = fourcc;
    target->rect.x = target->rect.y = 0;
    target->rect.width = target->width;
    target->rect.height = target->height;
    return target;
}

Decode_Status jdva_init_read_tile_scanline(j_decompress_ptr cinfo, jd_libva_struct * jd_libva_ptr, int *x, int *y, int *w, int *h)
{
    if (jd_libva_ptr->cap_enabled & JPEG_CAPABILITY_UPSAMPLE) {
        JpegDecodeStatus st;
        jdva_private * priv = (jdva_private*)jd_libva_ptr->priv;
        if (priv->scale_factor != cinfo->scale_denom) {
            ALOGV("%s scale_denom changed from %d to %d!!!!", __FUNCTION__, priv->scale_factor, cinfo->scale_denom);
        }
        priv->tile_read_x = (*x < cinfo->image_width)? *x: (cinfo->image_width - 1);
        priv->tile_read_y = (*y < cinfo->image_height)? *y: (cinfo->image_height - 1);
        priv->tile_read_width = (priv->tile_read_x + *w < cinfo->image_width)? *w: (cinfo->image_width - priv->tile_read_x);
        priv->tile_read_width /= priv->scale_factor;
        priv->tile_read_height = (priv->tile_read_y + *h < cinfo->image_height)? *h: (cinfo->image_height - priv->tile_read_y);
        priv->tile_read_height /= priv->scale_factor;
        ALOGV("%s, x=%d->%d, y=%d>%d, w=%d->%d, h=%d->%d", __FUNCTION__,
            *x, priv->tile_read_x,
            *y, priv->tile_read_y,
            *w, priv->tile_read_width,
            *h, priv->tile_read_height);
        *x = priv->tile_read_x;
        *y = priv->tile_read_y;
        *w = priv->tile_read_width;
        *h = priv->tile_read_height;
        return DECODE_SUCCESS;
    }
    else {
        // should not be here
        assert(false);
        return DECODE_DRIVER_FAIL;
    }
}
Decode_Status jdva_read_tile_scanline (j_decompress_ptr cinfo, jd_libva_struct * jd_libva_ptr, char ** scanlines, unsigned int* row_ctr)
{
    if (jd_libva_ptr->cap_enabled & JPEG_CAPABILITY_UPSAMPLE) {
        jdva_private *priv = (jdva_private*)jd_libva_ptr->priv;
        *row_ctr = 1;
        memcpy(scanlines[0], priv->rgba_out + priv->tile_read_y * cinfo->image_width * 4 + priv->tile_read_x * 4, priv->tile_read_width * 4);
        priv->tile_read_y++;
        return DECODE_SUCCESS;
    }
    else {
        // should not be here
        assert(false);
        return DECODE_DRIVER_FAIL;
    }
}

void free_render_target(RenderTarget *target)
{
    if (target == NULL)
        return;
    uint32_t fourcc = target->pixel_format;
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
    ALOGV("%s deleting %s target %p", __FUNCTION__, fourcc2str(fourcc), target);
}

Decode_Status jdva_blit(struct jdva_private * priv);

Decode_Status jdva_decode (j_decompress_ptr cinfo, jd_libva_struct * jd_libva_ptr)
{
    JpegDecodeStatus st;
    jdva_private * priv = (jdva_private*)jd_libva_ptr->priv;
    hw_module_t const* module = NULL;
    alloc_device_t *allocdev = NULL;
    struct gralloc_module_t *gralloc_module = NULL;
    buffer_handle_t handle;
    int err;
    char fname[256];
    FILE *fdec;
    uint8_t *data = NULL;
    uint32_t offsets[3];
    uint32_t pitches[3];
    nsecs_t t1, t2, t3;
    JpegDecoder::MapHandle maphandle;
    if (!priv)
        return DECODE_DRIVER_FAIL;

    t1 = systemTime();
    JpegInfo& jpginfo = priv->jpg_info;

    if (jd_libva_ptr->cap_enabled & JPEG_CAPABILITY_DOWNSCALE) {
        priv->scale_factor = cinfo->scale_denom;
        cinfo->min_DCT_scaled_size = DCTSIZE/priv->scale_factor;
        cinfo->output_width = cinfo->image_width/priv->scale_factor;
        cinfo->output_height = cinfo->image_height/priv->scale_factor;
    }
    else {
        priv->scale_factor = 1;
        cinfo->min_DCT_scaled_size = DCTSIZE;
        cinfo->output_width = cinfo->image_width;
        cinfo->output_height = cinfo->image_height;
    }

    jdva_drain_input(cinfo, jd_libva_ptr);
    jpginfo.need_header_only = false;
    st = priv->decoder->parse(jpginfo);
    switch (st) {
    case JD_ERROR_BITSTREAM:
        ALOGE("%s: error parsing bitstream", __FUNCTION__);
        return DECODE_PARSER_FAIL;
    case JD_SUCCESS:
        break;
    default:
        ALOGE("%s: error in driver: parse failed", __FUNCTION__);
        return DECODE_DRIVER_FAIL;
    }

    st = priv->decoder->decode(jpginfo, priv->dec_buffer);
    if (st != JD_SUCCESS) {
        ALOGE("%s: error decoding %s image", __FUNCTION__, fourcc2str(jpginfo.image_color_fourcc));
        return DECODE_DRIVER_FAIL;
    }
#if DUMP_DECODE
    sprintf(fname, DECODE_DUMP_FILE_PATTERN, jpginfo.image_width, jpginfo.image_height, fourcc2str(jpginfo.image_color_fourcc));
    fdec = fopen(fname, "wb");
    if (fdec) {
        maphandle = priv->decoder->mapData(priv->dec_buffer, (void**)&data, offsets, pitches);
        int ss_x, ss_y;
        ss_x = ss_y = -1;
        switch(jpginfo.image_color_fourcc) {
        case VA_FOURCC_411P:
            ss_x = 2;
            ss_y = 0;
            break;
        case VA_FOURCC_IMC3:
            ss_x = 1;
            ss_y = 1;
            break;
        case VA_FOURCC_422V:
            ss_x = 0;
            ss_y = 1;
            break;
        case VA_FOURCC_422H:
            ss_x = 1;
            ss_y = 0;
            break;
        case VA_FOURCC_444P:
            ss_x = 0;
            ss_y = 0;
            break;
        default:
            break;
        }
        for (int r = 0; r < jpginfo.image_height; ++r)
            fwrite(data + offsets[0] + pitches[0] * r, 1, jpginfo.image_width, fdec);
        if (ss_x >=0 && ss_y >=0) {
            for (int r = 0; r < jpginfo.image_height >> ss_y; ++r)
                fwrite(data + offsets[1] + pitches[1] * r, 1, jpginfo.image_width >> ss_x, fdec);
            for (int r = 0; r < jpginfo.image_height >> ss_y; ++r)
                fwrite(data + offsets[2] + pitches[2] * r, 1, jpginfo.image_width >> ss_x, fdec);
        }
        priv->decoder->unmapData(priv->dec_buffer, maphandle);
        fclose(fdec);
        ALOGV("%s Dumped decode surface into %s", __FUNCTION__, fname);
    }
#endif
    t2 = systemTime();

    if (!(jd_libva_ptr->cap_enabled & JPEG_CAPABILITY_UPSAMPLE)) {
        ALOGV("%s decoded %ux%u %s JPEG for %.2f ms", __FUNCTION__,
            priv->jpg_info.image_width, priv->jpg_info.image_height,
            fourcc2str(priv->jpg_info.image_color_fourcc),
            (t2-t1)/1000000.0);
        // TODO: implement
    }
    else {
        priv->rgba_out = (uint8_t*)memalign(0x1000,
            aligned_width(cinfo->output_width, SURF_TILING_Y)
            * aligned_height(cinfo->output_height, SURF_TILING_Y) * 4);
        if (priv->rgba_out == NULL) {
            ALOGE("%s failed to create RGBA buffer", __FUNCTION__);
            return DECODE_MEMORY_FAIL;
        }

        Decode_Status ret;
        {
            Mutex::Autolock autoLock(jdlock);
            ret = jdva_blit(priv);
            if (ret != DECODE_SUCCESS) {
                ALOGE("%s blit %ux%u (%dx scaling) %s failed", __FUNCTION__,
                    priv->jpg_info.image_width, priv->jpg_info.image_height,
                    priv->scale_factor,
                    fourcc2str(priv->jpg_info.image_color_fourcc));
                goto cleanup;
            }
        }
        t3 = systemTime();
        ALOGI("%s decode+blit %ux%u (%dx scaling) %s JPEG for %.2f+%.2f ms", __FUNCTION__,
            priv->jpg_info.image_width, priv->jpg_info.image_height,
            priv->scale_factor,
            fourcc2str(priv->jpg_info.image_color_fourcc),
            (t2-t1)/1000000.0, (t3-t2)/1000000.0);
    }
    return DECODE_SUCCESS;
cleanup:
    if (priv->rgba_out) {
        free(priv->rgba_out);
        priv->rgba_out = NULL;
    }
    return DECODE_DRIVER_FAIL;
}

Decode_Status jdva_read_scanlines (j_decompress_ptr cinfo, jd_libva_struct * jd_libva_ptr, char ** scanlines, unsigned int* row_ctr, unsigned int max_lines)
{
    if (jd_libva_ptr->cap_enabled & JPEG_CAPABILITY_UPSAMPLE) {
        jdva_private *priv = (jdva_private*)jd_libva_ptr->priv;
        uint32_t scanline = cinfo->output_scanline;
        for (*row_ctr = 0; *row_ctr + scanline < cinfo->output_height && *row_ctr < max_lines; ++*row_ctr) {
            memcpy(scanlines[*row_ctr], priv->rgba_out + (scanline + *row_ctr) * aligned_width(cinfo->output_width, SURF_TILING_Y) * 4, cinfo->output_width * 4);
        }
        return DECODE_SUCCESS;
    }
    else {
        // should not be here
        assert(false);
        return DECODE_DRIVER_FAIL;
    }
}

Decode_Status jdva_create_resource (jd_libva_struct * jd_libva_ptr)
{
    VAStatus va_status = VA_STATUS_SUCCESS;
    Decode_Status status = DECODE_SUCCESS;
    JpegDecodeStatus st;
    Mutex::Autolock autoLock(jdlock);
    jdva_private *priv = (jdva_private*)jd_libva_ptr->priv;
    jd_libva_ptr->image_width = priv->jpg_info.picture_param_buf.picture_width;
    jd_libva_ptr->image_height = priv->jpg_info.picture_param_buf.picture_height;
    create_render_target(&priv->dec_buffer, jd_libva_ptr->image_width,jd_libva_ptr->image_height,priv->jpg_info.image_color_fourcc);
    if (&priv->dec_buffer == NULL) {
        ALOGE("%s failed to create decode render target", __FUNCTION__);
        return DECODE_MEMORY_FAIL;
    }
    RenderTarget *targets = &priv->dec_buffer;
    st = priv->decoder->init(jd_libva_ptr->image_width, jd_libva_ptr->image_height, &targets, 1);
    if (st != JD_SUCCESS) {
        free_render_target(&priv->dec_buffer);
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
        priv->decoder->deinit();
        free_render_target(&priv->dec_buffer);
        if (priv->rgba_out) {
            free(priv->rgba_out);
            priv->rgba_out = NULL;
        }
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
    JpegDecodeStatus st;

    Decode_Status res;
    jpginfo.need_header_only = true;
    do {
        res = jdva_fill_input(cinfo, jd_libva_ptr);
        if (res) {
            return res;
        }
        st = priv->decoder->parse(jpginfo);
    } while (st == JD_INSUFFICIENT_BYTE);
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
    cinfo->out_color_space = JCS_RGB; /* set default colorspace for output */
    cinfo->src->bytes_in_buffer = jd_libva_ptr->file_size;
    cinfo->scale_num = cinfo->scale_denom = 1; /* set default value */
    return DECODE_SUCCESS;
}

Decode_Status jdva_blit(struct jdva_private * priv)
{
    JpegDecodeStatus st;
    nsecs_t t1, t2;

    char fname[256];
    FILE *fdec;
    t1 = systemTime();
    st = priv->decoder->blitToLinearRgba(priv->dec_buffer, priv->rgba_out,
        priv->jpg_info.image_width,
        priv->jpg_info.image_height,
        priv->blit_event, priv->scale_factor);
    if (st != JD_SUCCESS) {
        ALOGE("%s: error blitting to RGBA buffer", __FUNCTION__);
        goto cleanup;
    }
    t2 = systemTime();
#if DUMP_RGBA
    sprintf(fname, RGBA_DUMP_FILE_PATTERN, priv->jpg_info.output_width, priv->jpg_info.output_height, fourcc2str(priv->jpg_info.image_color_fourcc));
    fdec = fopen(fname, "wb");
    if (fdec) {
        fwrite(priv->rgba_out, 1, priv->jpg_info.output_width * priv->jpg_info.output_height * 4, fdec);
        fclose(fdec);
        ALOGV("%s Dumped RGBA output into %s", __FUNCTION__, fname);
    }
#endif
    ALOGV("%s blitted %ux%u RGBA from JPEG %s data for %.2f ms", __FUNCTION__,
        priv->jpg_info.image_width, priv->jpg_info.image_height,
        fourcc2str(priv->jpg_info.image_color_fourcc),
        (t2-t1)/1000000.0);
    return DECODE_SUCCESS;
cleanup:
    return DECODE_DRIVER_FAIL;
}

