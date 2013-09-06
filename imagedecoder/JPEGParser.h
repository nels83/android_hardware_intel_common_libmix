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

#ifndef _JPEG_PARSE_H_
#define _JPEG_PARSE_H_

#include <stdint.h>

// Marker Codes
#define CODE_SOF_BASELINE 0xC0
#define CODE_SOF1         0xC1
#define CODE_SOF2         0xC2
#define CODE_SOF3         0xC3
#define CODE_SOF5         0xC5
#define CODE_SOF6         0xC6
#define CODE_SOF7         0xC7
#define CODE_SOF8         0xC8
#define CODE_SOF9         0xC9
#define CODE_SOF10        0xCA
#define CODE_SOF11        0xCB
#define CODE_SOF13        0xCD
#define CODE_SOF14        0xCE
#define CODE_SOF15        0xCF
#define CODE_DHT          0xC4
#define CODE_RST0         0xD0
#define CODE_RST1         0xD1
#define CODE_RST2         0xD2
#define CODE_RST3         0xD3
#define CODE_RST4         0xD4
#define CODE_RST5         0xD5
#define CODE_RST6         0xD6
#define CODE_RST7         0xD7
#define CODE_SOI          0xD8
#define CODE_EOI          0xD9
#define CODE_SOS          0xDA
#define CODE_DQT          0xDB
#define CODE_DRI          0xDD
#define CODE_APP0         0xE0
#define CODE_APP1         0xE1
#define CODE_APP2         0xE2
#define CODE_APP3         0xE3
#define CODE_APP4         0xE4
#define CODE_APP5         0xE5
#define CODE_APP6         0xE6
#define CODE_APP7         0xE7
#define CODE_APP8         0xE8
#define CODE_APP9         0xE9
#define CODE_APP10        0xEA
#define CODE_APP11        0xEB
#define CODE_APP12        0xEC
#define CODE_APP13        0xED
#define CODE_APP14        0xEE
#define CODE_APP15        0xEF

struct CJPEGParse {
    uint8_t* stream_buff;
    uint32_t parse_index;
    uint32_t buff_size;
    bool end_of_buff;
    uint8_t (*readNextByte)(CJPEGParse* parser);
    uint32_t (*readBytes)( CJPEGParse* parser, uint32_t bytes_to_read );
    void (*burnBytes)( CJPEGParse* parser, uint32_t bytes_to_burn );
    uint8_t (*getNextMarker)(CJPEGParse* parser);
    uint32_t (*getByteOffset)(CJPEGParse* parser);
    bool (*endOfBuffer)(CJPEGParse* parser);
    uint8_t* (*getCurrentIndex)(CJPEGParse* parser);
    bool (*setByteOffset)( CJPEGParse* parser, uint32_t byte_offset );
};

void parserInitialize(CJPEGParse* parser,  uint8_t* stream_buff, uint32_t buff_size);
#endif // _JPEG_PARSE_H_

