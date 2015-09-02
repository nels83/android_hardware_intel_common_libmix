/* INTEL CONFIDENTIAL
* Copyright (c) 2009 Intel Corporation.  All rights reserved.
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


#ifndef VBP_H264SECURE_PARSER_H
#define VBP_H264SECURE_PARSER_H

/*
 * setup parser's entry points
 */
uint32 vbp_init_parser_entries_h264secure(vbp_context *pcontext);

/*
 * allocate query data
 */
uint32 vbp_allocate_query_data_h264secure(vbp_context *pcontext);

/*
 * free query data
 */
uint32 vbp_free_query_data_h264secure(vbp_context *pcontext);

/*
 * parse initialization data
 */
uint32 vbp_parse_init_data_h264secure(vbp_context *pcontext);

/*
 * parse start code. Only support lenght prefixed mode. Start
 * code prefixed is not supported.
 */
uint32 vbp_parse_start_code_h264secure(vbp_context *pcontext);

/*
 * process parsing result
 */
uint32 vbp_process_parsing_result_h264secure(vbp_context *pcontext, int list_index);

/*
 * query parsing result
 */
uint32 vbp_populate_query_data_h264secure(vbp_context *pcontext);

/*
 * update the parsing result with extra data
 */

uint32 vbp_update_data_h264secure(vbp_context *pcontext, void *newdata, uint32 size);


#endif /*VBP_H264_PARSER_H*/

