/* INTEL CONFIDENTIAL
* Copyright (c) 2013 Intel Corporation.  All rights reserved.
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
*    Yao Cheng <yao.cheng@intel.com>
*
*/
//#define LOG_NDEBUG 0

#include "JPEGBlitter.h"
#include "JPEGCommon_Gen.h"
#include "JPEGDecoder.h"

#include <va/va.h>
#include <va/va_tpi.h>
#include "ImageDecoderTrace.h"

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <assert.h>

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

const VAProcColorStandardType fourcc2ColorStandard(uint32_t fourcc)
{
    switch(fourcc) {
    case VA_FOURCC_NV12:
    case VA_FOURCC_YUY2:
    case VA_FOURCC_422H:
    case VA_FOURCC_422V:
    case VA_FOURCC_411P:
    case VA_FOURCC_411R:
    case VA_FOURCC_IMC3:
    case VA_FOURCC_444P:
    case VA_FOURCC_YV12:
        return VAProcColorStandardBT601;
    default:
        return VAProcColorStandardNone;
    }
}

void write_to_file(const char *file, const VAImage *pImg, const uint8_t *pSrc)
{
    FILE *fp = fopen(file, "wb");
    if (!fp) {
        return;
    }
    const uint8_t *pY, *pU, *pV, *pYUYV, *pRGBA, *pUV;
    float h_samp_factor, v_samp_factor;
    int row, col;
    char fourccstr[5];
    VTRACE("Dumping %s buffer to %s", fourcc2str(fourccstr, pImg->format.fourcc), file);
    switch (pImg->format.fourcc) {
    case VA_FOURCC_IMC3:
        h_samp_factor = 1;
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
    case VA_FOURCC_YUY2:
    {
        pYUYV = pSrc + pImg->offsets[0];
        VTRACE("YUY2 output width %u stride %u", pImg->width, pImg->pitches[0]);
        for (row = 0; row < pImg->height; ++row) {
            fwrite(pYUYV, 2, pImg->width, fp);
            pYUYV += pImg->pitches[0];
        }
    }
    fclose(fp);
    return;
    case VA_FOURCC_NV12:
    {
        pY = pSrc + pImg->offsets[0];
        pUV = pSrc + pImg->offsets[1];
        VTRACE("NV12 output width %u stride %u, %u", pImg->width, pImg->pitches[0], pImg->pitches[1]);
        for (row = 0; row < pImg->height; ++row) {
            fwrite(pY, 1, pImg->width, fp);
            pY += pImg->pitches[0];
        }
        for (row = 0; row < pImg->height/2; ++row) {
            fwrite(pUV, 1, pImg->width, fp);
            pUV += pImg->pitches[1];
        }
    }
    fclose(fp);
    return;
    case VA_FOURCC_RGBA:
    case VA_FOURCC_BGRA:
    case VA_FOURCC_ARGB:
    case VA_FOURCC('A', 'B', 'G', 'R'):
    {
        pRGBA = pSrc + pImg->offsets[0];
        VTRACE("RGBA output width %u stride %u", pImg->width, pImg->pitches[0]);
        for (row = 0; row < pImg->height; ++row) {
            fwrite(pRGBA, 4, pImg->width, fp);
            pRGBA += pImg->pitches[0];
        }
    }
    fclose(fp);
    return;
    default:
        // non-supported
        {
            char fourccstr[5];
            ETRACE("%s: Not-supported input YUV format", fourcc2str(fourccstr, pImg->format.fourcc));
        }
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

static void write_to_YUY2(uint8_t *pDst,
                          uint32_t dst_w,
                          uint32_t dst_h,
                          uint32_t dst_stride,
                          const VAImage *pImg,
                          const uint8_t *pSrc)
{
    const uint8_t *pY, *pU, *pV;
    float h_samp_factor, v_samp_factor;
    int row, col;
    char fourccstr[5];
    uint32_t copy_w = (dst_w < pImg->width)? dst_w: pImg->width;
    uint32_t copy_h = (dst_h < pImg->height)? dst_h: pImg->height;
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
        ETRACE("%s to YUY2: Not-supported input YUV format", fourcc2str(fourccstr, pImg->format.fourcc));
        return;
    }
    pY = pSrc + pImg->offsets[0];
    pU = pSrc + pImg->offsets[1];
    pV = pSrc + pImg->offsets[2];
    for (row = 0; row < copy_h; ++row) {
        for (col = 0; col < copy_w; ++col) {
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
        pDst += dst_stride;
        pY += pImg->pitches[0];
        uint32_t actual_row = row * v_samp_factor;
        pU = pSrc + pImg->offsets[1] + actual_row * pImg->pitches[1];
        pV = pSrc + pImg->offsets[2] + actual_row * pImg->pitches[2];
    }
}

static void dumpSurface(const char* filename, VADisplay display, VASurfaceID surface)
{
    VAStatus st;
    VAImage img;
    uint8_t *buf;
    st = vaDeriveImage(display, surface, &img);
    if (st) {
        ETRACE("vaDeriveImage failed with %d", st);
        return;
    }
    uint32_t in_fourcc = img.format.fourcc;
    VTRACE("Start dumping %s surface to %s", fourcc2str(NULL, in_fourcc), filename);
    st = vaMapBuffer(display, img.buf, (void **)&buf);
    if (st) {
        ETRACE("vaMapBuffer failed with %d", st);
        vaDestroyImage(display, img.image_id);
        return;
    }
    VTRACE("start write_to_file");
    write_to_file(filename, &img, buf);
    vaUnmapBuffer(display, img.buf);
    vaDestroyImage(display, img.image_id);
}

static void dumpGallocBuffer(const char* filename,
                                buffer_handle_t handle,
                                int width,
                                int height,
                                uint32_t fourcc)
{
    // NOT IMPLEMENTED
}


static JpegDecodeStatus swBlit(VADisplay display, VAContextID context,
                 VASurfaceID in_surf, VARectangle *in_rect, uint32_t in_fourcc,
                 VASurfaceID out_surf, VARectangle *out_rect, uint32_t out_fourcc)
{
    assert(out_fourcc == VA_FOURCC_YUY2);
    assert((in_fourcc == VA_FOURCC_IMC3) || (in_fourcc == VA_FOURCC_422H) || (in_fourcc == VA_FOURCC_444P));
    VAStatus st;
    char str[10];
    JpegDecodeStatus status;
    VAImage in_img, out_img;
    in_img.image_id = VA_INVALID_ID;
    in_img.buf = VA_INVALID_ID;
    out_img.image_id = VA_INVALID_ID;
    out_img.buf = VA_INVALID_ID;
    uint8_t *in_buf, *out_buf;
    in_buf = out_buf = NULL;
    st = vaDeriveImage(display, in_surf, &in_img);
    JD_CHECK_RET(st, cleanup, JD_BLIT_FAILURE);
    st = vaDeriveImage(display, out_surf, &out_img);
    JD_CHECK_RET(st, cleanup, JD_BLIT_FAILURE);
    st = vaMapBuffer(display, in_img.buf, (void **)&in_buf);
    JD_CHECK_RET(st, cleanup, JD_BLIT_FAILURE);
    st = vaMapBuffer(display, out_img.buf, (void **)&out_buf);
    JD_CHECK_RET(st, cleanup, JD_BLIT_FAILURE);
    VTRACE("%s in: %s, %ux%u, size %u, offset=%u,%u,%u, pitch=%u,%u,%u", __FUNCTION__,
        fourcc2str(NULL, in_fourcc),
        in_img.width,
        in_img.height,
        in_img.data_size,
        in_img.offsets[0], in_img.offsets[1], in_img.offsets[2],
        in_img.pitches[0], in_img.pitches[1], in_img.pitches[2]);
    VTRACE("%s out: %s, %ux%u, size %u, offset=%u,%u,%u, pitch=%u,%u,%u", __FUNCTION__,
        fourcc2str(NULL, out_fourcc),
        out_img.width,
        out_img.height,
        out_img.data_size,
        out_img.offsets[0], out_img.offsets[1], out_img.offsets[2],
        out_img.pitches[0], out_img.pitches[1], out_img.pitches[2]);
    write_to_YUY2(out_buf, out_img.width, out_img.height, out_img.pitches[0], &in_img, in_buf);
    vaUnmapBuffer(display, in_img.buf);
    vaUnmapBuffer(display, out_img.buf);
    vaDestroyImage(display, in_img.image_id);
    vaDestroyImage(display, out_img.image_id);
    VTRACE("%s Finished SW CSC %s=>%s", __FUNCTION__, fourcc2str(str, in_fourcc), fourcc2str(str + 5, out_fourcc));
    return JD_SUCCESS;

cleanup:
    ETRACE("%s failed to do swBlit %s=>%s", __FUNCTION__, fourcc2str(str, in_fourcc), fourcc2str(str + 5, out_fourcc));
    if (in_buf != NULL) vaUnmapBuffer(display, in_img.buf);
    if (out_buf != NULL) vaUnmapBuffer(display, out_img.buf);
    if (in_img.image_id != VA_INVALID_ID) vaDestroyImage(display, in_img.image_id);
    if (out_img.image_id != VA_INVALID_ID) vaDestroyImage(display, out_img.image_id);
    return status;
}

static JpegDecodeStatus hwBlit(VADisplay display, VAContextID context,
                 VASurfaceID in_surf, VARectangle *in_rect, uint32_t in_fourcc,
                 VASurfaceID out_surf, VARectangle *out_rect, uint32_t out_fourcc)
{
    VAProcPipelineCaps vpp_pipeline_cap ;
    VABufferID vpp_pipeline_buf = VA_INVALID_ID;
    VAProcPipelineParameterBuffer vpp_param;
    VAStatus vpp_status;
    JpegDecodeStatus status = JD_SUCCESS;
    char str[10];
    nsecs_t t1, t2;

    memset(&vpp_param, 0, sizeof(VAProcPipelineParameterBuffer));
#if PRE_TOUCH_SURFACE
    //zeroSurfaces(display, &out_surf, 1);
#endif
    t1 = systemTime();
    vpp_param.surface                 = in_surf;
    vpp_param.output_region           = out_rect;
    vpp_param.surface_region          = in_rect;
    vpp_param.surface_color_standard  = fourcc2ColorStandard(in_fourcc);
    vpp_param.output_background_color = 0;
    vpp_param.output_color_standard   = fourcc2ColorStandard(out_fourcc);
    vpp_param.filter_flags            = VA_FRAME_PICTURE;
    vpp_param.filters                 = NULL;
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
    JD_CHECK_RET(vpp_status, cleanup, JD_RESOURCE_FAILURE);

    vpp_status = vaBeginPicture(display,
                                context,
                                out_surf);
    JD_CHECK_RET(vpp_status, cleanup, JD_BLIT_FAILURE);

    //Render the picture
    vpp_status = vaRenderPicture(display,
                                 context,
                                 &vpp_pipeline_buf,
                                 1);
    JD_CHECK_RET(vpp_status, cleanup, JD_BLIT_FAILURE);

    vpp_status = vaEndPicture(display, context);
    JD_CHECK_RET(vpp_status, cleanup, JD_BLIT_FAILURE);

    vaDestroyBuffer(display, vpp_pipeline_buf);
    JD_CHECK_RET(vpp_status, cleanup, JD_BLIT_FAILURE);
    t2 = systemTime();
    VTRACE("Finished HW CSC %s(%d,%d,%u,%u)=>%s(%d,%d,%u,%u) for %f ms",
        fourcc2str(str, in_fourcc),
        in_rect->x, in_rect->y, in_rect->width, in_rect->height,
        fourcc2str(str + 5, out_fourcc),
        out_rect->x, out_rect->y, out_rect->width, out_rect->height,
        ns2us(t2 - t1)/1000.0);

    return JD_SUCCESS;
cleanup:
    if (vpp_pipeline_buf != VA_INVALID_ID)
        vaDestroyBuffer(display, vpp_pipeline_buf);
    return status;
}

static JpegDecodeStatus vaBlit(VADisplay display, VAContextID context,
                 VASurfaceID in_surf, VARectangle *in_rect, uint32_t in_fourcc,
                 VASurfaceID out_surf, VARectangle *out_rect, uint32_t out_fourcc)
{
    if (((in_fourcc == VA_FOURCC_422H) ||
        (in_fourcc == VA_FOURCC_NV12) ||
        (in_fourcc == VA_FOURCC_YUY2) ||
        (in_fourcc == VA_FOURCC_YV12) ||
        (in_fourcc == VA_FOURCC_RGBA))
        &&
        ((out_fourcc == VA_FOURCC_422H) ||
        (out_fourcc == VA_FOURCC_NV12) ||
        (out_fourcc == VA_FOURCC_YV12) ||
        (out_fourcc == VA_FOURCC_YUY2) ||
        (out_fourcc == VA_FOURCC_RGBA))) {
        return hwBlit(display, context, in_surf, in_rect, in_fourcc,
               out_surf, out_rect, out_fourcc);
    }
    else {
        return swBlit(display, context, in_surf, in_rect, in_fourcc,
               out_surf, out_rect, out_fourcc);
    }
}

JpegDecodeStatus JpegBlitter::blit(RenderTarget &src, RenderTarget &dst)
{
    if (mDecoder == NULL)
        return JD_UNINITIALIZED;
    JpegDecodeStatus st;
    uint32_t src_fourcc, dst_fourcc;
    char tmp[10];
    src_fourcc = pixelFormat2Fourcc(src.pixel_format);
    dst_fourcc = pixelFormat2Fourcc(dst.pixel_format);
    VASurfaceID src_surf = mDecoder->getSurfaceID(src);
    if (src_surf == VA_INVALID_ID) {
        ETRACE("%s invalid src %s target", __FUNCTION__, fourcc2str(NULL, src_fourcc));
        return JD_INVALID_RENDER_TARGET;
    }
    VASurfaceID dst_surf = mDecoder->getSurfaceID(dst);
    if (dst_surf == VA_INVALID_ID) {
        WTRACE("%s foreign dst target for JpegDecoder, create surface for it, not guaranteed to free it!!!", __FUNCTION__);
        st = mDecoder->createSurfaceFromRenderTarget(dst, &dst_surf);
        if (st != JD_SUCCESS || dst_surf == VA_INVALID_ID) {
            ETRACE("%s failed to create surface for dst target", __FUNCTION__);
            return JD_RESOURCE_FAILURE;
        }
    }

    VTRACE("%s blitting from %s to %s", __FUNCTION__, fourcc2str(tmp, src_fourcc), fourcc2str(tmp + 5, dst_fourcc));
    st = vaBlit(mDecoder->mDisplay, mContextId, src_surf, &src.rect, src_fourcc,
                dst_surf, &dst.rect, dst_fourcc);

    return st;
}

