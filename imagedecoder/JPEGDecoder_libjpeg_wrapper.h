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

#ifndef JDLIBVA_H
#define JDLIBVA_H

#include <pthread.h>
#include <stdio.h>
#include "jpeglib.h"

#define Display unsigned int
#define BOOL int

#define JPEG_MAX_COMPONENTS 4
#define JPEG_MAX_QUANT_TABLES 4

typedef struct {
    const uint8_t* bitstream_buf;
    uint32_t image_width;
    uint32_t image_height;

    boolean hw_state_ready;
    boolean hw_caps_ready;
    boolean hw_path;
    boolean initialized;
    boolean resource_allocated;

    uint32_t file_size;
    uint32_t rotation;
    int      tile_mode;

    uint32_t cap_available;
    uint32_t cap_enabled;

    uint32_t priv;
} jd_libva_struct;

typedef enum {
    DECODE_NOT_STARTED = -7,
    DECODE_INVALID_DATA = -6,
    DECODE_DRIVER_FAIL = -5,
    DECODE_PARSER_FAIL = -4,
    DECODE_PARSER_INSUFFICIENT_BYTES = -3,
    DECODE_MEMORY_FAIL = -2,
    DECODE_FAIL = -1,
    DECODE_SUCCESS = 0,

} IMAGE_DECODE_STATUS;

#define JPEG_CAPABILITY_DECODE     0x0
#define JPEG_CAPABILITY_UPSAMPLE   0x1
#define JPEG_CAPABILITY_DOWNSCALE  0x2

/*********************** for libjpeg ****************************/
typedef int32_t Decode_Status;
extern jd_libva_struct jd_libva;
#ifdef __cplusplus
extern "C" {
#endif
Decode_Status jdva_initialize (jd_libva_struct * jd_libva_ptr);
void jdva_deinitialize (jd_libva_struct * jd_libva_ptr);
Decode_Status jdva_fill_input(j_decompress_ptr cinfo, jd_libva_struct * jd_libva_ptr);
void jdva_drain_input(j_decompress_ptr cinfo, jd_libva_struct * jd_libva_ptr);
Decode_Status jdva_decode (j_decompress_ptr cinfo, jd_libva_struct * jd_libva_ptr);
Decode_Status jdva_read_scanlines (j_decompress_ptr cinfo, jd_libva_struct * jd_libva_ptr, char ** scanlines, unsigned int* row_ctr, unsigned int max_lines);
Decode_Status jdva_init_read_tile_scanline(j_decompress_ptr cinfo, jd_libva_struct * jd_libva_ptr, int *x, int *y, int *w, int *h);
Decode_Status jdva_read_tile_scanline (j_decompress_ptr cinfo, jd_libva_struct * jd_libva_ptr, char ** scanlines, unsigned int* row_ctr);
Decode_Status jdva_create_resource (jd_libva_struct * jd_libva_ptr);
Decode_Status jdva_release_resource (jd_libva_struct * jd_libva_ptr);
Decode_Status jdva_parse_bitstream(j_decompress_ptr cinfo, jd_libva_struct * jd_libva_ptr);
#ifdef __cplusplus
}
#endif


#endif

