/*************************************************************************************
 * INTEL CONFIDENTIAL
 * Copyright 2011 Intel Corporation All Rights Reserved.
 * The source code contained or described herein and all documents related
 * to the source code ("Material") are owned by Intel Corporation or its
 * suppliers or licensors. Title to the Material remains with Intel
 * Corporation or its suppliers and licensors. The Material contains trade
 * secrets and proprietary and confidential information of Intel or its
 * suppliers and licensors. The Material is protected by worldwide copyright
 * and trade secret laws and treaty provisions. No part of the Material may
 * be used, copied, reproduced, modified, published, uploaded, posted,
 * transmitted, distributed, or disclosed in any way without Intelâ~@~Ys prior
 * express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery
 * of the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be express
 * and approved by Intel in writing.
 ************************************************************************************/
/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef EDITVIDEO_NV12_H
#define EDITVIDEO_NV12_H

M4OSA_ERR M4VSS3GPP_intSetNv12PlaneFromARGB888(
    M4VSS3GPP_InternalEditContext *pC, M4VSS3GPP_ClipContext* pClipCtxt);

M4OSA_ERR M4VSS3GPP_intRotateVideo_NV12(M4VIFI_ImagePlane* pPlaneIn,
    M4OSA_UInt32 rotationDegree);

M4OSA_ERR M4VSS3GPP_intApplyRenderingMode_NV12(M4VSS3GPP_InternalEditContext *pC,
    M4xVSS_MediaRendering renderingMode, M4VIFI_ImagePlane* pInplane,
    M4VIFI_ImagePlane* pOutplane);

unsigned char M4VFL_modifyLumaWithScale_NV12(M4ViComImagePlane *plane_in,
    M4ViComImagePlane *plane_out, unsigned long lum_factor,
    void *user_data);

unsigned char M4VIFI_ImageBlendingonNV12 (void *pUserData,
    M4ViComImagePlane *pPlaneIn1, M4ViComImagePlane *pPlaneIn2,
    M4ViComImagePlane *pPlaneOut, UInt32 Progress);

#endif
