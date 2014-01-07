/* INTEL CONFIDENTIAL
* Copyright (c) 2013 Intel Corporation.  All rights reserved.
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

#ifndef JPEG_BLITTER_H
#define JPEG_BLITTER_H

#include "JPEGCommon.h"
#include <utils/threads.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <hardware/gralloc.h>

using namespace android;

class JpegDecoder;
typedef void* BlitEvent;

class JpegBlitter
{
public:
    JpegBlitter(VADisplay display, VAConfigID vpCfgId, VAContextID vpCtxId);
    virtual ~JpegBlitter();
    virtual void init(JpegDecoder &dec);
    virtual void deinit();
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
private:
    mutable Mutex mLock;
    JpegDecoder *mDecoder;
    VADisplay mDisplay;
    VAConfigID mConfigId;
    VAContextID mContextId;
    void *mPrivate;
    bool mInitialized;
};

#endif
