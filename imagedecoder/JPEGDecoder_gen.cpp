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

#include "va/va.h"
#include "va/va_vpp.h"
#include "va/va_drmcommon.h"
#include "JPEGDecoder.h"
#include "ImageDecoderTrace.h"
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include "JPEGCommon_Gen.h"

int fourcc2PixelFormat(uint32_t fourcc)
{
    switch(fourcc) {
    case VA_FOURCC_YV12:
        return HAL_PIXEL_FORMAT_YV12;
    case VA_FOURCC_422H:
        return HAL_PIXEL_FORMAT_YCbCr_422_H_INTEL;
    case VA_FOURCC_YUY2:
        return HAL_PIXEL_FORMAT_YCbCr_422_I;
    case VA_FOURCC_NV12:
        return HAL_PIXEL_FORMAT_NV12_TILED_INTEL;
    case VA_FOURCC_RGBA:
        return HAL_PIXEL_FORMAT_RGBA_8888;
    case VA_FOURCC_IMC3:
        return HAL_PIXEL_FORMAT_IMC3;
    case VA_FOURCC_444P:
        return HAL_PIXEL_FORMAT_444P;
    case VA_FOURCC_422V:
    case VA_FOURCC_411P:
    default:
        return -1;
    }
}
uint32_t pixelFormat2Fourcc(int pixel_format)
{
    switch(pixel_format) {
    case HAL_PIXEL_FORMAT_YV12:
        return VA_FOURCC_YV12;
    case HAL_PIXEL_FORMAT_YCbCr_422_H_INTEL:
        return VA_FOURCC_422H;
    case HAL_PIXEL_FORMAT_YCbCr_422_I:
        return VA_FOURCC_YUY2;
    case HAL_PIXEL_FORMAT_NV12_TILED_INTEL:
        return VA_FOURCC_NV12;
    case HAL_PIXEL_FORMAT_RGBA_8888:
        return VA_FOURCC_RGBA;
    case HAL_PIXEL_FORMAT_444P:
        return VA_FOURCC_444P;
    case HAL_PIXEL_FORMAT_IMC3:
        return VA_FOURCC_IMC3;
    default:
        return 0;
    }
}

//#define LOG_TAG "ImageDecoder"

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

bool JpegDecoder::jpegColorFormatSupported(JpegInfo &jpginfo) const
{
    return (jpginfo.image_color_fourcc == VA_FOURCC_IMC3) ||
        (jpginfo.image_color_fourcc == VA_FOURCC_422H) ||
        (jpginfo.image_color_fourcc == VA_FOURCC_444P);
}

JpegDecodeStatus JpegDecoder::createSurfaceDrm(int width, int height, int pixel_format, unsigned long boname, int stride, VASurfaceID *surf_id)
{
    VAStatus st;
    VASurfaceAttrib                 attrib_list;
    VASurfaceAttribExternalBuffers  vaSurfaceExternBuf;
    uint32_t fourcc = pixelFormat2Fourcc(pixel_format);
    vaSurfaceExternBuf.pixel_format = fourcc;
    VTRACE("%s extBuf.pixel_format is %s", __FUNCTION__, fourcc2str(NULL, fourcc));
    vaSurfaceExternBuf.width        = width;
    vaSurfaceExternBuf.height       = height;
    vaSurfaceExternBuf.pitches[0]   = stride;
    vaSurfaceExternBuf.buffers      = &boname;
    vaSurfaceExternBuf.num_buffers  = 1;
    vaSurfaceExternBuf.flags        = VA_SURFACE_ATTRIB_MEM_TYPE_KERNEL_DRM;
    attrib_list.type          = VASurfaceAttribExternalBufferDescriptor;
    attrib_list.flags         = VA_SURFACE_ATTRIB_SETTABLE;
    attrib_list.value.type    = VAGenericValueTypePointer;
    attrib_list.value.value.p = (void *)&vaSurfaceExternBuf;

    st = vaCreateSurfaces(mDisplay,
            fourcc2VaFormat(fourcc),
            width,
            height,
            surf_id,
            1,
            &attrib_list,
            1);
    VTRACE("%s createSurface DRM for vaformat %u, fourcc %s", __FUNCTION__, fourcc2VaFormat(fourcc), fourcc2str(NULL, fourcc));
    if (st != VA_STATUS_SUCCESS) {
        ETRACE("%s: vaCreateSurfaces returns %d", __PRETTY_FUNCTION__, st);
        return JD_RESOURCE_FAILURE;
    }
    return JD_SUCCESS;
}

JpegDecodeStatus JpegDecoder::createSurfaceGralloc(int width, int height, int pixel_format, buffer_handle_t handle, int stride, VASurfaceID *surf_id)
{
    unsigned long boname;
    hw_module_t const* module = NULL;
    alloc_device_t *allocdev = NULL;
    struct gralloc_module_t *gralloc_module = NULL;
    JpegDecodeStatus st;

    uint32_t fourcc = pixelFormat2Fourcc(pixel_format);
    VTRACE("enter %s, pixel_format 0x%x, fourcc %s", __FUNCTION__, pixel_format, fourcc2str(NULL, fourcc));
    if ((fourcc != VA_FOURCC_422H) ||
        (fourcc != VA_FOURCC_YUY2) ||
        (fourcc != VA_FOURCC_RGBA)){
        VASurfaceAttrib attrib;
        attrib.type = VASurfaceAttribPixelFormat;
        attrib.flags = VA_SURFACE_ATTRIB_SETTABLE;
        attrib.value.type = VAGenericValueTypeInteger;
        attrib.value.value.i = fourcc;
        VAStatus va_status = vaCreateSurfaces(mDisplay,
                                    fourcc2VaFormat(fourcc),
                                    width,
                                    height,
                                    surf_id,
                                    1,
                                    &attrib,
                                    1);
        VTRACE("%s createSurface for %s", __FUNCTION__, fourcc2str(NULL, fourcc));
        if (va_status != VA_STATUS_SUCCESS)
            return JD_RESOURCE_FAILURE;
        return JD_SUCCESS;
    }

    int err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);
    if (err) {
        ETRACE("%s failed to get gralloc module", __PRETTY_FUNCTION__);
        st = JD_RESOURCE_FAILURE;
    }
    JD_CHECK(err, cleanup);
    gralloc_module = (struct gralloc_module_t *)module;
    err = gralloc_open(module, &allocdev);
    if (err) {
        ETRACE("%s failed to open alloc device", __PRETTY_FUNCTION__);
        st = JD_RESOURCE_FAILURE;
    }
    JD_CHECK(err, cleanup);
    err = gralloc_module->perform(gralloc_module,
        INTEL_UFO_GRALLOC_MODULE_PERFORM_GET_BO_NAME,
        handle,
        &boname);
    if (err) {
        ETRACE("%s failed to get boname via gralloc->perform", __PRETTY_FUNCTION__);
        st = JD_RESOURCE_FAILURE;
    }
    JD_CHECK(err, cleanup);
    VTRACE("YAO %s fourcc %s luma_stride is %d", __FUNCTION__,
        fourcc2str(NULL, fourcc), stride);

    gralloc_close(allocdev);
    return createSurfaceDrm(width, height, pixel_format, boname, stride, surf_id);
cleanup:
    if (allocdev)
        gralloc_close(allocdev);
    return st;
}





