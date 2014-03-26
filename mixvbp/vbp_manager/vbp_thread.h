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
*/

#ifndef VBP_THREAD_H
#define VBP_THREAD_H

#define _GNU_SOURCE
#include <pthread.h>
#include <semaphore.h>

#include <sys/syscall.h>
#include "vbp_utils.h"
#include "include/viddec_pm.h"
#include <sched.h>


void vbp_thread_init(vbp_context *pcontext);

void vbp_thread_free(vbp_context *pcontext);

uint32_t vbp_thread_parse_syntax(void* parent,
                                 void* ctxt,
                                 vbp_context* pcontext);

uint32_t vbp_thread_set_active(vbp_context* pcontext,
                               uint32_t active_count);

uint32_t vbp_thread_get_active(vbp_context* pcontext);

#endif
