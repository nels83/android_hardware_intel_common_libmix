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

#ifndef JPEG_BLITTER_H
#define JPEG_BLITTER_H

#include <VideoVPPBase.h>
#include "JPEGCommon.h"
#include <utils/threads.h>

class JpegDecoder;

class JpegBlitter
{
public:
    JpegBlitter();
    virtual ~JpegBlitter();
    virtual void setDecoder(JpegDecoder &decoder);
    virtual JpegDecodeStatus blit(RenderTarget &src, RenderTarget &dst);
private:
    mutable Mutex mLock;
    virtual void destroyContext();
    JpegDecoder *mDecoder;
    VAConfigID mConfigId;
    VAContextID mContextId;
};

#endif
