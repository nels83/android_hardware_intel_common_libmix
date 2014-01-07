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


#ifndef JPEGDEC_H
#define JPEGDEC_H

#include <utils/KeyedVector.h>
#include <utils/threads.h>
#include <hardware/gralloc.h>
#include "JPEGCommon.h"
#include <va/va.h>
#include <va/va_drmcommon.h>
#include <va/va_vpp.h>
#include <va/va_android.h>
#include <va/va_tpi.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

using namespace android;

struct CJPEGParse;
class JpegBitstreamParser;
class JpegBlitter;
typedef void* BlitEvent;

extern int generateHandle();

// Non thread-safe
class JpegDecoder
{
friend class JpegBlitter;
public:
    typedef uint32_t MapHandle;
    JpegDecoder(VADisplay display = NULL, VAConfigID vpCfgId = VA_INVALID_ID, VAContextID vpCtxId = VA_INVALID_ID, bool use_blitter = false);
    virtual ~JpegDecoder();
    virtual JpegDecodeStatus init(int width, int height, RenderTarget **targets, int num);
    virtual void deinit();
    virtual JpegDecodeStatus parse(JpegInfo &jpginfo);
    virtual JpegDecodeStatus decode(JpegInfo &jpginfo, RenderTarget &target);
    virtual JpegDecodeStatus sync(RenderTarget &target);
    virtual bool busy(RenderTarget &target) const;
    virtual JpegDecodeStatus blit(RenderTarget &src, RenderTarget &dst, int scale_factor);
    virtual JpegDecodeStatus getRgbaTile(RenderTarget &src,
                                         uint8_t *sysmem,
                                         int left, int top, int width, int height, int scale_factor);
    virtual JpegDecodeStatus blitToLinearRgba(RenderTarget &src,
                                              uint8_t *sysmem,
                                              uint32_t width, uint32_t height,
                                              BlitEvent &event, int scale_factor);
    virtual JpegDecodeStatus blitToCameraSurfaces(RenderTarget &src,
                                                   buffer_handle_t dst_nv12,
                                                   buffer_handle_t dst_yuy2,
                                                   uint8_t *dst_nv21,
                                                   uint8_t *dst_yv12,
                                                   uint32_t width, uint32_t height,
                                                   BlitEvent &event);
    virtual void syncBlit(BlitEvent &event);
    virtual MapHandle mapData(RenderTarget &target, void ** data, uint32_t * offsets, uint32_t * pitches);
    virtual void unmapData(RenderTarget &target, MapHandle maphandle);
    virtual VASurfaceID getSurfaceID(RenderTarget &target) const;
    virtual JpegDecodeStatus createSurfaceFromRenderTarget(RenderTarget &target, VASurfaceID *surf_id);
    virtual JpegDecodeStatus destroySurface(RenderTarget &target);
    virtual JpegDecodeStatus destroySurface(VASurfaceID surf_id);
protected:
    bool mInitialized;
    mutable Mutex mLock;
    VADisplay mDisplay;
    VAConfigID mConfigId;
    VAContextID mContextId;
    CJPEGParse *mParser;
    JpegBitstreamParser *mBsParser;
    bool mParserInitialized;
    JpegBlitter *mBlitter;
    bool mDispCreated;
    KeyedVector<buffer_handle_t, VASurfaceID> mGrallocSurfaceMap;
    KeyedVector<unsigned long, VASurfaceID> mDrmSurfaceMap;
    KeyedVector<int, VASurfaceID> mNormalSurfaceMap;
    KeyedVector<int, VASurfaceID> mUserptrSurfaceMap;
    virtual JpegDecodeStatus parseHeader(JpegInfo &jpginfo);
    virtual JpegDecodeStatus parseTableData(JpegInfo &jpginfo);
    virtual bool jpegColorFormatSupported(JpegInfo &jpginfo) const;
    virtual JpegDecodeStatus createSurfaceInternal(int width, int height, uint32_t fourcc, int handle, VASurfaceID *surf_id);
    virtual JpegDecodeStatus createSurfaceUserptr(int width, int height, uint32_t fourcc, uint8_t* ptr, VASurfaceID *surf_id);
    virtual JpegDecodeStatus createSurfaceDrm(int width, int height, uint32_t fourcc, unsigned long boname, int stride, VASurfaceID *surf_id);
    virtual JpegDecodeStatus createSurfaceGralloc(int width, int height, uint32_t fourcc, buffer_handle_t handle, int stride, VASurfaceID *surf_id);
};


#endif

