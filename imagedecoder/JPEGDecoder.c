/* INTEL CONFIDENTIAL
* Copyright (c) 2012 Intel Corporation.  All rights reserved.
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
*    Nana Guo <nana.n.guo@intel.com>
*
*/

#include "va/va_tpi.h"
#include "JPEGDecoder.h"
#include "ImageDecoderTrace.h"
#include "JPEGParser.h"
#include <string.h>

#define JPEG_MAX_SETS_HUFFMAN_TABLES 2

#define TABLE_CLASS_DC  0
#define TABLE_CLASS_AC  1
#define TABLE_CLASS_NUM 2

/*
 * Initialize VA API related stuff
 *
 * We will check the return value of  jva_initialize
 * to determine which path will be use (SW or HW)
 *
 */
Decode_Status jdva_initialize (jd_libva_struct * jd_libva_ptr) {
  /*
   * Please note that we won't check the input parameters to follow the
   * convention of libjpeg duo to we need these parameters to do error handling,
   * and if these parameters are invalid, means the whole stack is crashed, so check
   * them here and return false is meaningless, same situation for all internal methods
   * related to VA API
  */
    uint32_t va_major_version = 0;
    uint32_t va_minor_version = 0;
    VAStatus va_status = VA_STATUS_SUCCESS;
    Decode_Status status = DECODE_SUCCESS;
    uint32_t index;

    if (jd_libva_ptr->initialized)
        return DECODE_NOT_STARTED;

    jd_libva_ptr->android_display = (Display*)malloc(sizeof(Display));
    if (jd_libva_ptr->android_display == NULL) {
        return DECODE_MEMORY_FAIL;
    }
    jd_libva_ptr->va_display = vaGetDisplay (jd_libva_ptr->android_display);

    if (jd_libva_ptr->va_display == NULL) {
        ETRACE("vaGetDisplay failed.");
        free (jd_libva_ptr->android_display);
        return DECODE_DRIVER_FAIL;
    }
    va_status = vaInitialize(jd_libva_ptr->va_display, &va_major_version, &va_minor_version);
    if (va_status != VA_STATUS_SUCCESS) {
        ETRACE("vaInitialize failed. va_status = 0x%x", va_status);
        status = DECODE_DRIVER_FAIL;
        goto cleanup;
    }

    VAConfigAttrib attrib;
    attrib.type = VAConfigAttribRTFormat;
    va_status = vaGetConfigAttributes(jd_libva_ptr->va_display, VAProfileJPEGBaseline, VAEntrypointVLD, &attrib, 1);
    if (va_status != VA_STATUS_SUCCESS) {
        ETRACE("vaGetConfigAttributes failed. va_status = 0x%x", va_status);
        status = DECODE_DRIVER_FAIL;
        goto cleanup;
    }
    if ((VA_RT_FORMAT_YUV444 & attrib.value) == 0) {
        WTRACE("Format not surportted\n");
        status = DECODE_FAIL;
        goto cleanup;
    }

    va_status = vaCreateConfig(jd_libva_ptr->va_display, VAProfileJPEGBaseline, VAEntrypointVLD, &attrib, 1, &(jd_libva_ptr->va_config));
    if (va_status != VA_STATUS_SUCCESS) {
        ETRACE("vaCreateConfig failed. va_status = 0x%x", va_status);
        status = DECODE_DRIVER_FAIL;
        goto cleanup;
    }
    jd_libva_ptr->initialized = TRUE;
    status = DECODE_SUCCESS;

cleanup:
#if 0
    /*free profiles and entrypoints*/
    if (va_profiles)
        free(va_profiles);

    if (va_entrypoints)
        free (va_entrypoints);
#endif
    if (status) {
        jd_libva_ptr->initialized = TRUE; // make sure we can call into jva_deinitialize()
        jdva_deinitialize (jd_libva_ptr);
        return status;
    }

  return status;
}

void jdva_deinitialize (jd_libva_struct * jd_libva_ptr) {
    if (!(jd_libva_ptr->initialized)) {
        return;
    }

    if (jd_libva_ptr->JPEGParser) {
        free(jd_libva_ptr->JPEGParser);
        jd_libva_ptr->JPEGParser = NULL;
    }

    if (jd_libva_ptr->va_display) {
        vaTerminate(jd_libva_ptr->va_display);
        jd_libva_ptr->va_display = NULL;
    }

    if (jd_libva_ptr->android_display) {
        free(jd_libva_ptr->android_display);
        jd_libva_ptr->android_display = NULL;
    }

    jd_libva_ptr->initialized = FALSE;
    return;
}

Decode_Status jdva_create_resource (jd_libva_struct * jd_libva_ptr) {
    VAStatus va_status = VA_STATUS_SUCCESS;
    Decode_Status status = DECODE_SUCCESS;
    jd_libva_ptr->image_width = jd_libva_ptr->picture_param_buf.picture_width;
    jd_libva_ptr->image_height = jd_libva_ptr->picture_param_buf.picture_height;
    jd_libva_ptr->surface_count = 1;
    jd_libva_ptr->va_surfaces = (VASurfaceID *) malloc(sizeof(VASurfaceID)*jd_libva_ptr->surface_count);
    if (jd_libva_ptr->va_surfaces == NULL) {
        return DECODE_MEMORY_FAIL;
    }
    va_status = vaCreateSurfaces(jd_libva_ptr->va_display, VA_RT_FORMAT_YUV444,
                                    jd_libva_ptr->image_width,
                                    jd_libva_ptr->image_height,
                                    jd_libva_ptr->va_surfaces,
                                    jd_libva_ptr->surface_count, NULL, 0);
    if (va_status != VA_STATUS_SUCCESS) {
        ETRACE("vaCreateSurfaces failed. va_status = 0x%x", va_status);
        status = DECODE_DRIVER_FAIL;
        goto cleanup;
    }
    va_status = vaCreateContext(jd_libva_ptr->va_display, jd_libva_ptr->va_config,
                                   jd_libva_ptr->image_width,
                                   jd_libva_ptr->image_height,
                                   0,  //VA_PROGRESSIVE
                                   jd_libva_ptr->va_surfaces,
                                   jd_libva_ptr->surface_count, &(jd_libva_ptr->va_context));

    if (va_status != VA_STATUS_SUCCESS) {
        ETRACE("vaCreateContext failed. va_status = 0x%x", va_status);
        status = DECODE_DRIVER_FAIL;
        goto cleanup;

    }
    jd_libva_ptr->resource_allocated = TRUE;
    return status;
cleanup:

    if (jd_libva_ptr->va_surfaces) {
        free (jd_libva_ptr->va_surfaces);
        jd_libva_ptr->va_surfaces = NULL;
    }
    jdva_deinitialize (jd_libva_ptr);

    return status;
}

Decode_Status jdva_release_resource (jd_libva_struct * jd_libva_ptr) {
    Decode_Status status = DECODE_SUCCESS;
    VAStatus va_status = VA_STATUS_SUCCESS;

    if (!(jd_libva_ptr->resource_allocated)) {
        return status;
    }

    if (!(jd_libva_ptr->va_display)) {
        return status; //most likely the resource are already released and HW jpeg is deinitialize, return directly
    }

  /*
   * It is safe to destroy Surface/Config/Context severl times
   * and it is also safe even their value is NULL
   */

    va_status = vaDestroySurfaces(jd_libva_ptr->va_display, jd_libva_ptr->va_surfaces, jd_libva_ptr->surface_count);
    if (va_status != VA_STATUS_SUCCESS) {
        ETRACE("vaDestroySurfaces failed. va_status = 0x%x", va_status);
        return DECODE_DRIVER_FAIL;
    }

    if (jd_libva_ptr->va_surfaces) {
        free (jd_libva_ptr->va_surfaces);
        jd_libva_ptr->va_surfaces = NULL;
    }

    va_status = vaDestroyConfig(jd_libva_ptr->va_display, jd_libva_ptr->va_config);
    if (va_status != VA_STATUS_SUCCESS) {
        ETRACE("vaDestroyConfig failed. va_status = 0x%x", va_status);
        return DECODE_DRIVER_FAIL;
    }

    jd_libva_ptr->va_config = NULL;

    va_status = vaDestroyContext(jd_libva_ptr->va_display, jd_libva_ptr->va_context);
    if (va_status != VA_STATUS_SUCCESS) {
        ETRACE("vaDestroyContext failed. va_status = 0x%x", va_status);
        return DECODE_DRIVER_FAIL;
    }

    jd_libva_ptr->va_context = NULL;

    jd_libva_ptr->resource_allocated = FALSE;

    return va_status;
}

Decode_Status jdva_decode (jd_libva_struct * jd_libva_ptr, uint8_t* buf) {
    Decode_Status status = DECODE_SUCCESS;
    VAStatus va_status = VA_STATUS_SUCCESS;
    VABufferID desc_buf[5];
    uint32_t bitstream_buffer_size = 0;
    uint32_t scan_idx = 0;
    uint32_t buf_idx = 0;
    uint32_t chopping = VA_SLICE_DATA_FLAG_ALL;
    uint32_t bytes_remaining = jd_libva_ptr->eoi_offset - jd_libva_ptr->soi_offset;
    uint32_t src_offset = jd_libva_ptr->soi_offset;
    bitstream_buffer_size = 1024*1024*5;

    va_status = vaBeginPicture(jd_libva_ptr->va_display, jd_libva_ptr->va_context, jd_libva_ptr->va_surfaces[0]);
    if (va_status != VA_STATUS_SUCCESS) {
        ETRACE("vaBeginPicture failed. va_status = 0x%x", va_status);
        status = DECODE_DRIVER_FAIL;
        return status;
    }
    va_status = vaCreateBuffer(jd_libva_ptr->va_display, jd_libva_ptr->va_context, VAPictureParameterBufferType, sizeof(VAPictureParameterBufferJPEGBaseline), 1, &jd_libva_ptr->picture_param_buf, &desc_buf[buf_idx]);
    if (va_status != VA_STATUS_SUCCESS) {
        ETRACE("vaCreateBuffer failed. va_status = 0x%x", va_status);
        status = DECODE_DRIVER_FAIL;
        return status;
    }
    buf_idx++;
    va_status = vaCreateBuffer(jd_libva_ptr->va_display, jd_libva_ptr->va_context, VAIQMatrixBufferType, sizeof(VAIQMatrixBufferJPEGBaseline), 1, &jd_libva_ptr->qmatrix_buf, &desc_buf[buf_idx]);

    if (va_status != VA_STATUS_SUCCESS) {
        ETRACE("vaCreateBuffer failed. va_status = 0x%x", va_status);
        status = DECODE_DRIVER_FAIL;
        return status;
    }
    buf_idx++;
    va_status = vaCreateBuffer(jd_libva_ptr->va_display, jd_libva_ptr->va_context, VAHuffmanTableBufferType, sizeof(VAHuffmanTableBufferJPEGBaseline), 1, &jd_libva_ptr->hufman_table_buf, &desc_buf[buf_idx]);
    if (va_status != VA_STATUS_SUCCESS) {
        ETRACE("vaCreateBuffer failed. va_status = 0x%x", va_status);
        status = DECODE_DRIVER_FAIL;
        return status;
    }
    buf_idx++;
    do {
        /* Get Bitstream Buffer */
        uint32_t bytes = ( bytes_remaining < bitstream_buffer_size ) ? bytes_remaining : bitstream_buffer_size;
        bytes_remaining -= bytes;
        /* Get Slice Control Buffer */
        VASliceParameterBufferJPEGBaseline dest_scan_ctrl[JPEG_MAX_COMPONENTS];
        uint32_t src_idx = 0;
        uint32_t dest_idx = 0;
        memset(dest_scan_ctrl, 0, sizeof(dest_scan_ctrl));
        for (src_idx = scan_idx; src_idx < jd_libva_ptr->scan_ctrl_count ; src_idx++) {
            if (jd_libva_ptr->slice_param_buf[ src_idx ].slice_data_offset) {
                /* new scan, reset state machine */
                chopping = VA_SLICE_DATA_FLAG_ALL;
                fprintf(stderr,"Scan:%i FileOffset:%x Bytes:%x \n", src_idx,
                    jd_libva_ptr->slice_param_buf[ src_idx ].slice_data_offset,
                    jd_libva_ptr->slice_param_buf[ src_idx ].slice_data_size );
                /* does the slice end in the buffer */
                if (jd_libva_ptr->slice_param_buf[ src_idx ].slice_data_offset + jd_libva_ptr->slice_param_buf[ src_idx ].slice_data_size > bytes + src_offset) {
                    chopping = VA_SLICE_DATA_FLAG_BEGIN;
                }
            } else {
                if (jd_libva_ptr->slice_param_buf[ src_idx ].slice_data_size > bytes) {
                    chopping = VA_SLICE_DATA_FLAG_MIDDLE;
                } else {
                    if ((chopping == VA_SLICE_DATA_FLAG_BEGIN) || (chopping == VA_SLICE_DATA_FLAG_MIDDLE)) {
                        chopping = VA_SLICE_DATA_FLAG_END;
                    }
                }
            }
            dest_scan_ctrl[dest_idx].slice_data_flag = chopping;
            dest_scan_ctrl[dest_idx].slice_data_offset = ((chopping == VA_SLICE_DATA_FLAG_ALL) ||      (chopping == VA_SLICE_DATA_FLAG_BEGIN) )?
jd_libva_ptr->slice_param_buf[ src_idx ].slice_data_offset : 0;

            const int32_t bytes_in_seg = bytes - dest_scan_ctrl[dest_idx].slice_data_offset;
            const uint32_t scan_data = (bytes_in_seg < jd_libva_ptr->slice_param_buf[src_idx].slice_data_size) ? bytes_in_seg : jd_libva_ptr->slice_param_buf[src_idx].slice_data_size ;
            jd_libva_ptr->slice_param_buf[src_idx].slice_data_offset = 0;
            jd_libva_ptr->slice_param_buf[src_idx].slice_data_size -= scan_data;
            dest_scan_ctrl[dest_idx].slice_data_size = scan_data;
            dest_scan_ctrl[dest_idx].num_components = jd_libva_ptr->slice_param_buf[src_idx].num_components;
            dest_scan_ctrl[dest_idx].restart_interval = jd_libva_ptr->slice_param_buf[src_idx].restart_interval;
            memcpy(&dest_scan_ctrl[dest_idx].components, & jd_libva_ptr->slice_param_buf[ src_idx ].components,
                sizeof(jd_libva_ptr->slice_param_buf[ src_idx ].components) );
            dest_idx++;
            if ((chopping == VA_SLICE_DATA_FLAG_ALL) || (chopping == VA_SLICE_DATA_FLAG_END)) { /* all good good */
            } else {
                break;
            }
        }
        scan_idx = src_idx;
        /* Get Slice Control Buffer */
        va_status = vaCreateBuffer(jd_libva_ptr->va_display, jd_libva_ptr->va_context, VASliceParameterBufferType, sizeof(VASliceParameterBufferJPEGBaseline) * dest_idx, 1, dest_scan_ctrl, &desc_buf[buf_idx]);
        if (va_status != VA_STATUS_SUCCESS) {
            ETRACE("vaCreateBuffer failed. va_status = 0x%x", va_status);
            status = DECODE_DRIVER_FAIL;
            return status;
        }
        buf_idx++;
        va_status = vaCreateBuffer(jd_libva_ptr->va_display, jd_libva_ptr->va_context, VASliceDataBufferType, bytes, 1, &jd_libva_ptr->bitstream_buf[ src_offset ], &desc_buf[buf_idx]);
        buf_idx++;
        if (va_status != VA_STATUS_SUCCESS) {
            ETRACE("vaCreateBuffer failed. va_status = 0x%x", va_status);
            status = DECODE_DRIVER_FAIL;
            return status;
        }
        va_status = vaRenderPicture( jd_libva_ptr->va_display, jd_libva_ptr->va_context, desc_buf, buf_idx);
        if (va_status != VA_STATUS_SUCCESS) {
            ETRACE("vaRenderPicture failed. va_status = 0x%x", va_status);
            status = DECODE_DRIVER_FAIL;
            return status;
        }
        buf_idx = 0;

        src_offset += bytes;
    } while (bytes_remaining);

    va_status = vaEndPicture(jd_libva_ptr->va_display, jd_libva_ptr->va_context);
    if (va_status != VA_STATUS_SUCCESS) {
        ETRACE("vaRenderPicture failed. va_status = 0x%x", va_status);
        status = DECODE_DRIVER_FAIL;
        return status;
    }

    va_status = vaSyncSurface(jd_libva_ptr->va_display, jd_libva_ptr->va_surfaces[0]);
    if (va_status != VA_STATUS_SUCCESS) {
        WTRACE("vaSyncSurface failed. va_status = 0x%x", va_status);
    }
#if 0
    uint8_t* rgb_buf;
    int32_t data_len = 0;
    uint32_t surface_width, surface_height;
    surface_width = (( ( jd_libva_ptr->image_width + 7 ) & ( ~7 )) + 15 ) & ( ~15 );
    surface_height = (( ( jd_libva_ptr->image_height + 7 ) & ( ~7 )) + 15 ) & ( ~15 );

    rgb_buf = (uint8_t*) malloc((surface_width * surface_height) << 2);
    if(rgb_buf == NULL){
        return DECODE_MEMORY_FAIL;
    }
    va_status = vaPutSurfaceBuf(jd_libva_ptr->va_display, jd_libva_ptr->va_surfaces[0], rgb_buf, &data_len, 0, 0, surface_width, surface_height, 0, 0, surface_width, surface_height, NULL, 0, 0);

    buf = rgb_buf;
// dump RGB data
    {
        FILE *pf_tmp = fopen("img_out.rgb", "wb");
        if(pf_tmp == NULL)
            ETRACE("Open file error");
        fwrite(rgb_buf, 1, surface_width * surface_height * 4, pf_tmp);
        fclose(pf_tmp);
    }
#endif
#if 0
    va_status = vaDeriveImage(jd_libva_ptr->va_display, jd_libva_ptr->va_surfaces[0], &(jd_libva_ptr->surface_image));
    if (va_status != VA_STATUS_SUCCESS) {
        ERREXIT1 (cinfo, JERR_VA_DRIVEIMAGE, va_status);
    }

    va_status = vaMapBuffer(jd_libva_ptr->va_display, jd_libva_ptr->surface_image.buf, (void **)& (jd_libva_ptr->image_buf));
    if (va_status != VA_STATUS_SUCCESS) {
        ERREXIT1 (cinfo, JERR_VA_MAPBUFFER, va_status);
    }

    va_status = vaUnmapBuffer(jd_libva_ptr->va_display, jd_libva_ptr->surface_image.buf);
    if (va_status != VA_STATUS_SUCCESS) {
        ERREXIT1(cinfo, JERR_VA_MAPBUFFER, va_status);
    }

    va_status = vaDestroyImage(jd_libva_ptr->va_display, jd_libva_ptr->surface_image.image_id);

    if (va_status != VA_STATUS_SUCCESS) {
        ERREXIT1 (cinfo, JERR_VA_MAPBUFFER, va_status);
    }
#endif
    return status;
}

Decode_Status parseBitstream(jd_libva_struct * jd_libva_ptr) {
    uint32_t component_order = 0 ;
    uint32_t dqt_ind = 0;
    uint32_t dht_ind = 0;
    uint32_t scan_ind = 0;
    boolean frame_marker_found = FALSE;

    uint8_t marker = jd_libva_ptr->JPEGParser->getNextMarker(jd_libva_ptr->JPEGParser);

    while (marker != CODE_EOI &&( !jd_libva_ptr->JPEGParser->endOfBuffer(jd_libva_ptr->JPEGParser))) {
        switch (marker) {
            case CODE_SOI: {
                 jd_libva_ptr->soi_offset = jd_libva_ptr->JPEGParser->getByteOffset(jd_libva_ptr->JPEGParser) - 2;
                break;
            }
            // If the marker is an APP marker skip over the data
            case CODE_APP0:
            case CODE_APP1:
            case CODE_APP2:
            case CODE_APP3:
            case CODE_APP4:
            case CODE_APP5:
            case CODE_APP6:
            case CODE_APP7:
            case CODE_APP8:
            case CODE_APP9:
            case CODE_APP10:
            case CODE_APP11:
            case CODE_APP12:
            case CODE_APP13:
            case CODE_APP14:
            case CODE_APP15: {

                uint32_t bytes_to_burn = jd_libva_ptr->JPEGParser->readBytes(jd_libva_ptr->JPEGParser, 2) - 2;
                jd_libva_ptr->JPEGParser->burnBytes(jd_libva_ptr->JPEGParser, bytes_to_burn);
                    break;
            }
            // Store offset to DQT data to avoid parsing bitstream in user mode
            case CODE_DQT: {
                if (dqt_ind < 4) {
                    jd_libva_ptr->dqt_byte_offset[dqt_ind] = jd_libva_ptr->JPEGParser->getByteOffset(jd_libva_ptr->JPEGParser) - jd_libva_ptr->soi_offset;
                    dqt_ind++;
                    uint32_t bytes_to_burn = jd_libva_ptr->JPEGParser->readBytes( jd_libva_ptr->JPEGParser, 2 ) - 2;
                    jd_libva_ptr->JPEGParser->burnBytes( jd_libva_ptr->JPEGParser, bytes_to_burn );
                } else {
                    ETRACE("ERROR: Decoder does not support more than 4 Quant Tables\n");
                    return DECODE_PARSER_FAIL;
                }
                break;
            }
            // Throw exception for all SOF marker other than SOF0
            case CODE_SOF1:
            case CODE_SOF2:
            case CODE_SOF3:
            case CODE_SOF5:
            case CODE_SOF6:
            case CODE_SOF7:
            case CODE_SOF8:
            case CODE_SOF9:
            case CODE_SOF10:
            case CODE_SOF11:
            case CODE_SOF13:
            case CODE_SOF14:
            case CODE_SOF15: {
                ETRACE("ERROR: unsupport SOF\n");
                break;
            }
            // Parse component information in SOF marker
            case CODE_SOF_BASELINE: {
                frame_marker_found = TRUE;

                jd_libva_ptr->JPEGParser->burnBytes(jd_libva_ptr->JPEGParser, 2); // Throw away frame header length
                uint8_t sample_precision = jd_libva_ptr->JPEGParser->readNextByte(jd_libva_ptr->JPEGParser);
                if (sample_precision != 8) {
                    ETRACE("sample_precision is not supported\n");
                    return DECODE_PARSER_FAIL;
                }
                // Extract pic width and height
                jd_libva_ptr->picture_param_buf.picture_height = jd_libva_ptr->JPEGParser->readBytes(jd_libva_ptr->JPEGParser, 2);
                jd_libva_ptr->picture_param_buf.picture_width = jd_libva_ptr->JPEGParser->readBytes(jd_libva_ptr->JPEGParser, 2);
                jd_libva_ptr->picture_param_buf.num_components = jd_libva_ptr->JPEGParser->readNextByte(jd_libva_ptr->JPEGParser);

                if (jd_libva_ptr->picture_param_buf.num_components > JPEG_MAX_COMPONENTS) {
                    return DECODE_PARSER_FAIL;
                }
                uint8_t comp_ind = 0;
                for (comp_ind = 0; comp_ind < jd_libva_ptr->picture_param_buf.num_components; comp_ind++) {
                    jd_libva_ptr->picture_param_buf.components[comp_ind].component_id = jd_libva_ptr->JPEGParser->readNextByte(jd_libva_ptr->JPEGParser);

                    uint8_t hv_sampling = jd_libva_ptr->JPEGParser->readNextByte(jd_libva_ptr->JPEGParser);
                    jd_libva_ptr->picture_param_buf.components[comp_ind].h_sampling_factor = hv_sampling >> 4;
                    jd_libva_ptr->picture_param_buf.components[comp_ind].v_sampling_factor = hv_sampling & 0xf;
                    jd_libva_ptr->picture_param_buf.components[comp_ind].quantiser_table_selector = jd_libva_ptr->JPEGParser->readNextByte(jd_libva_ptr->JPEGParser);
                }


                break;
            }
            // Store offset to DHT data to avoid parsing bitstream in user mode
            case CODE_DHT: {
                if (dht_ind < 4) {
                    jd_libva_ptr->dht_byte_offset[dht_ind] = jd_libva_ptr->JPEGParser->getByteOffset(jd_libva_ptr->JPEGParser) - jd_libva_ptr->soi_offset;
                    dht_ind++;
                    uint32_t bytes_to_burn = jd_libva_ptr->JPEGParser->readBytes(jd_libva_ptr->JPEGParser, 2) - 2;
                    jd_libva_ptr->JPEGParser->burnBytes(jd_libva_ptr->JPEGParser,  bytes_to_burn );
                } else {
                    ETRACE("ERROR: Decoder does not support more than 4 Huff Tables\n");
                    return DECODE_PARSER_FAIL;
                }
                break;
            }
            // Parse component information in SOS marker
            case CODE_SOS: {
                jd_libva_ptr->JPEGParser->burnBytes(jd_libva_ptr->JPEGParser, 2);
                uint32_t component_in_scan = jd_libva_ptr->JPEGParser->readNextByte(jd_libva_ptr->JPEGParser);
                uint8_t comp_ind = 0;

                for (comp_ind = 0; comp_ind < component_in_scan; comp_ind++) {
                    uint8_t comp_id = jd_libva_ptr->JPEGParser->readNextByte(jd_libva_ptr->JPEGParser);
                    uint8_t comp_data_ind;
                    for (comp_data_ind = 0; comp_data_ind < jd_libva_ptr->picture_param_buf.num_components; comp_data_ind++) {
                        if (comp_id == jd_libva_ptr->picture_param_buf.components[comp_data_ind].component_id) {
                            jd_libva_ptr->slice_param_buf[scan_ind].components[comp_ind].component_selector = comp_data_ind;
                            break;
                        }
                    }
                    uint8_t huffman_tables = jd_libva_ptr->JPEGParser->readNextByte(jd_libva_ptr->JPEGParser);
                    jd_libva_ptr->slice_param_buf[scan_ind].components[comp_ind].dc_table_selector = huffman_tables >> 4;
                    jd_libva_ptr->slice_param_buf[scan_ind].components[comp_ind].ac_table_selector = huffman_tables & 0xf;
                }
                uint32_t curr_byte = jd_libva_ptr->JPEGParser->readNextByte(jd_libva_ptr->JPEGParser); // Ss
                if (curr_byte != 0) {
                    return DECODE_PARSER_FAIL;
                }
                curr_byte = jd_libva_ptr->JPEGParser->readNextByte(jd_libva_ptr->JPEGParser);  // Se
                if (curr_byte != 0x3f) {
                    return DECODE_PARSER_FAIL;
                }
                curr_byte = jd_libva_ptr->JPEGParser->readNextByte(jd_libva_ptr->JPEGParser);  // Ah, Al
                if (curr_byte != 0) {
                    return DECODE_PARSER_FAIL;
                }
                // Set slice control variables needed
                jd_libva_ptr->slice_param_buf[scan_ind].slice_data_offset = jd_libva_ptr->JPEGParser->getByteOffset(jd_libva_ptr->JPEGParser) - jd_libva_ptr->soi_offset;
                jd_libva_ptr->slice_param_buf[scan_ind].num_components = component_in_scan;
                if (scan_ind) {
                    /* If there is more than one scan, the slice for all but the final scan should only run up to the beginning of the next scan */
                    jd_libva_ptr->slice_param_buf[scan_ind - 1].slice_data_size =
                        (jd_libva_ptr->slice_param_buf[scan_ind].slice_data_offset - jd_libva_ptr->slice_param_buf[scan_ind - 1].slice_data_offset );;
                    }
                    scan_ind++;
                    jd_libva_ptr->scan_ctrl_count++;   // gsDXVA2Globals.uiScanCtrlCount
                    break;
                }
            case CODE_DRI: {
                uint32_t size =  jd_libva_ptr->JPEGParser->readBytes(jd_libva_ptr->JPEGParser, 2);
                jd_libva_ptr->slice_param_buf[scan_ind].restart_interval =  jd_libva_ptr->JPEGParser->readBytes(jd_libva_ptr->JPEGParser, 2);
                jd_libva_ptr->JPEGParser->burnBytes(jd_libva_ptr->JPEGParser, (size - 4));
                break;
            }
            default:
                break;
        }

        marker = jd_libva_ptr->JPEGParser->getNextMarker(jd_libva_ptr->JPEGParser);
        // If the EOI code is found, store the byte offset before the parsing finishes
        if( marker == CODE_EOI ) {
            jd_libva_ptr->eoi_offset = jd_libva_ptr->JPEGParser->getByteOffset(jd_libva_ptr->JPEGParser);
        }

    }

    jd_libva_ptr->quant_tables_num = dqt_ind;
    jd_libva_ptr->huffman_tables_num = dht_ind;

    /* The slice for the last scan should run up to the end of the picture */
    jd_libva_ptr->slice_param_buf[scan_ind - 1].slice_data_size = (jd_libva_ptr->eoi_offset - jd_libva_ptr->slice_param_buf[scan_ind - 1].slice_data_offset);

    // throw AppException if SOF0 isn't found
    if (!frame_marker_found) {
        ETRACE("EEORR: Reached end of bitstream while trying to parse headers\n");
        return DECODE_PARSER_FAIL;
    }

    parseTableData(jd_libva_ptr);

    return DECODE_SUCCESS;

}

Decode_Status parseTableData(jd_libva_struct * jd_libva_ptr) {
    CJPEGParse* parser = (CJPEGParse*)malloc(sizeof(CJPEGParse));
    if (parser == NULL)
        return DECODE_MEMORY_FAIL;

    parserInitialize(parser, jd_libva_ptr->bitstream_buf, jd_libva_ptr->file_size);

    // Parse Quant tables
    memset(&jd_libva_ptr->qmatrix_buf, 0, sizeof(jd_libva_ptr->qmatrix_buf));
    uint32_t dqt_ind = 0;
    for (dqt_ind = 0; dqt_ind < jd_libva_ptr->quant_tables_num; dqt_ind++) {
        if (parser->setByteOffset(parser, jd_libva_ptr->dqt_byte_offset[dqt_ind])) {
            // uint32_t uiTableBytes = parser->readBytes( 2 ) - 2;
            uint32_t table_bytes = parser->readBytes( parser, 2 ) - 2;
            do {
                uint32_t table_info = parser->readNextByte(parser);
                table_bytes--;
                uint32_t table_length = table_bytes > 64 ? 64 : table_bytes;
                uint32_t table_precision = table_info >> 4;
                if (table_precision != 0) {
                    return DECODE_PARSER_FAIL;
                }
                uint32_t table_id = table_info & 0xf;

                jd_libva_ptr->qmatrix_buf.load_quantiser_table[dqt_ind] = table_id;

                if (table_id < JPEG_MAX_QUANT_TABLES) {
                    // Pull Quant table data from bitstream
                    uint32_t byte_ind;
                    for (byte_ind = 0; byte_ind < table_length; byte_ind++) {
                        jd_libva_ptr->qmatrix_buf.quantiser_table[table_id][byte_ind] = parser->readNextByte(parser);
                    }
                } else {
                    ETRACE("DQT table ID is not supported");
                    parser->burnBytes(parser, table_length);
                }
                table_bytes -= table_length;
            } while (table_bytes);
        }
    }

    // Parse Huffman tables
    memset(&jd_libva_ptr->hufman_table_buf, 0, sizeof(jd_libva_ptr->hufman_table_buf));
    uint32_t dht_ind = 0;
    for (dht_ind = 0; dht_ind < jd_libva_ptr->huffman_tables_num; dht_ind++) {
        if (parser->setByteOffset(parser, jd_libva_ptr->dht_byte_offset[dht_ind])) {
            uint32_t table_bytes = parser->readBytes( parser, 2 ) - 2;
            do {
                uint32_t table_info = parser->readNextByte(parser);
                table_bytes--;
                uint32_t table_class = table_info >> 4; // Identifies whether the table is for AC or DC
                uint32_t table_id = table_info & 0xf;

                if ((table_class < TABLE_CLASS_NUM) && (table_id < JPEG_MAX_SETS_HUFFMAN_TABLES)) {
                    if (table_class == 0) {
                        uint8_t* bits = parser->getCurrentIndex(parser);
                        // Find out the number of entries in the table
                        uint32_t table_entries = 0;
                        uint32_t bit_ind;
                        for (bit_ind = 0; bit_ind < 16; bit_ind++) {
                            jd_libva_ptr->hufman_table_buf.huffman_table[table_id].num_dc_codes[bit_ind] = bits[bit_ind];
                            table_entries += jd_libva_ptr->hufman_table_buf.huffman_table[table_id].num_dc_codes[bit_ind];
                        }

                        // Create table of code values
                        parser->burnBytes(parser, 16);
                        table_bytes -= 16;
                        uint32_t tbl_ind;
                        for (tbl_ind = 0; tbl_ind < table_entries; tbl_ind++) {
                            jd_libva_ptr->hufman_table_buf.huffman_table[table_id].dc_values[tbl_ind] = parser->readNextByte(parser);
                            table_bytes--;
                        }

                    } else { // for AC class
                        uint8_t* bits = parser->getCurrentIndex(parser);
                        // Find out the number of entries in the table
                        uint32_t table_entries = 0;
                        uint32_t bit_ind = 0;
                        for (bit_ind = 0; bit_ind < 16; bit_ind++) {
                            jd_libva_ptr->hufman_table_buf.huffman_table[table_id].num_ac_codes[bit_ind] = bits[bit_ind];
                            table_entries += jd_libva_ptr->hufman_table_buf.huffman_table[table_id].num_ac_codes[bit_ind];
                        }

                        // Create table of code values
                        parser->burnBytes(parser, 16);
                        table_bytes -= 16;
                        uint32_t tbl_ind = 0;
                        for (tbl_ind = 0; tbl_ind < table_entries; tbl_ind++) {
                            jd_libva_ptr->hufman_table_buf.huffman_table[table_id].ac_values[tbl_ind] = parser->readNextByte(parser);
                            table_bytes--;
                        }
                    }//end of else
                } else {
                    // Find out the number of entries in the table
                    ETRACE("DHT table ID is not supported");
                    uint32_t table_entries = 0;
                    uint32_t bit_ind = 0;
                    for(bit_ind = 0; bit_ind < 16; bit_ind++) {
                        table_entries += parser->readNextByte(parser);
                        table_bytes--;
                    }
                    parser->burnBytes(parser, table_entries);
                    table_bytes -= table_entries;
		}

            } while (table_bytes);
        }
    }

    if (parser) {
        free(parser);
        parser = NULL;
    }
    return DECODE_SUCCESS;
}

