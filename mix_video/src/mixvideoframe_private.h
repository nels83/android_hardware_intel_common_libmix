/*
INTEL CONFIDENTIAL
Copyright 2009 Intel Corporation All Rights Reserved.
The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
*/

#ifndef __MIX_VIDEOFRAME_PRIVATE_H__
#define __MIX_VIDEOFRAME_PRIVATE_H__
#if 0
#include "mixvideoframe.h"
#include "mixsurfacepool.h"

typedef enum _MixFrameType
{
    TYPE_I,
    TYPE_P,
    TYPE_B,
    TYPE_INVALID
} MixFrameType;

class MixVideoFramePrivate
{
public:
    MixVideoFramePrivate()
            :pool(NULL)
            ,is_skipped(FALSE)
            ,real_frame(NULL)
            ,sync_flag(FALSE)
            ,frame_structure(VA_FRAME_PICTURE)
            ,va_display(NULL)
    {}
public:
    /*< private > */

};




/* Private functions */
MIX_RESULT
mix_videoframe_set_pool (MixVideoFrame *obj, MixSurfacePool *pool);

MIX_RESULT
mix_videoframe_set_frame_type (MixVideoFrame *obj,  MixFrameType frame_type);

MIX_RESULT
mix_videoframe_get_frame_type (MixVideoFrame *obj,  MixFrameType *frame_type);

MIX_RESULT
mix_videoframe_set_is_skipped (MixVideoFrame *obj,  bool is_skipped);

MIX_RESULT
mix_videoframe_get_is_skipped (MixVideoFrame *obj,  bool *is_skipped);

MIX_RESULT
mix_videoframe_set_real_frame (MixVideoFrame *obj,  MixVideoFrame *real);

MIX_RESULT
mix_videoframe_get_real_frame (MixVideoFrame *obj,  MixVideoFrame **real);

MIX_RESULT
mix_videoframe_reset(MixVideoFrame *obj);

MIX_RESULT
mix_videoframe_set_sync_flag(MixVideoFrame *obj, bool sync_flag);

MIX_RESULT
mix_videoframe_get_sync_flag(MixVideoFrame *obj, bool *sync_flag);

MIX_RESULT
mix_videoframe_set_frame_structure(MixVideoFrame * obj, uint32 frame_structure);

MIX_RESULT
mix_videoframe_set_displayorder(MixVideoFrame *obj, uint32 displayorder);

MIX_RESULT
mix_videoframe_get_displayorder(MixVideoFrame *obj, uint32 *displayorder);
#endif

#endif /* __MIX_VIDEOFRAME_PRIVATE_H__ */
