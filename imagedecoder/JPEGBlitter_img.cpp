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

#include "JPEGBlitter.h"

JpegDecodeStatus JpegBlitter::blit(RenderTarget &src, RenderTarget &dst, int scale_factor)
{
    return JD_OUTPUT_FORMAT_UNSUPPORTED;
}

JpegDecodeStatus JpegBlitter::blitToLinearRgba(RenderTarget &src, uint8_t *sysmem, uint32_t width, uint32_t height, BlitEvent &event, int scale_factor)
{
    return JD_OUTPUT_FORMAT_UNSUPPORTED;
}
JpegDecodeStatus JpegBlitter::getRgbaTile(RenderTarget &src,
                                     uint8_t *sysmem,
                                     int left, int top, int width, int height, int scale_factor)
{
    return JD_OUTPUT_FORMAT_UNSUPPORTED;
}
void JpegBlitter::init(JpegDecoder& /*dec*/)
{
    // Do nothing
}
void JpegBlitter::deinit()
{
    // Do nothing
}
void JpegBlitter::syncBlit(BlitEvent &event)
{
    // Do nothing
}
JpegDecodeStatus JpegBlitter::blitToCameraSurfaces(RenderTarget &src,
                                                   buffer_handle_t dst_nv12,
                                                   buffer_handle_t dst_yuy2,
                                                   uint8_t *dst_nv21,
                                                   uint8_t *dst_yv12,
                                                   uint32_t width, uint32_t height,
                                                   BlitEvent &event)
{
    return JD_OUTPUT_FORMAT_UNSUPPORTED;
}
