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

#include "JPEGParser.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

bool endOfBuffer(CJPEGParse* parser);

uint8_t readNextByte(CJPEGParse* parser) {
    uint8_t byte = 0;

    if (parser->parse_index < parser->buff_size) {
        byte = *( parser->stream_buff + parser->parse_index );
        parser->parse_index++;
    }

    if (parser->parse_index == parser->buff_size) {
        parser->end_of_buff = true;
    }

    return byte;
}

uint32_t readBytes( CJPEGParse* parser, uint32_t bytes_to_read ) {
    uint32_t bytes = 0;

    while (bytes_to_read-- && !endOfBuffer(parser)) {
        bytes |= ( (uint32_t)readNextByte(parser) << ( bytes_to_read * 8 ) );
    }

    return bytes;
}

void burnBytes( CJPEGParse* parser, uint32_t bytes_to_burn ) {
    parser->parse_index += bytes_to_burn;

    if (parser->parse_index >= parser->buff_size) {
        parser->parse_index = parser->buff_size - 1;
        parser->end_of_buff = true;
    }
}

uint8_t getNextMarker(CJPEGParse* parser) {
    while (!endOfBuffer(parser)) {
        if (readNextByte(parser) == 0xff) {
            break;
        }
    }
    /* check the next byte to make sure we don't miss the real marker*/
    uint8_t tempNextByte = readNextByte(parser);
    if (tempNextByte == 0xff)
        return readNextByte(parser);
    else
        return tempNextByte;
}

bool setByteOffset(CJPEGParse* parser, uint32_t byte_offset)
{
    bool offset_found = false;

    if (byte_offset < parser->buff_size) {
        parser->parse_index = byte_offset;
        offset_found = true;
//      end_of_buff = false;
    }

    return offset_found;
}

uint32_t getByteOffset(CJPEGParse* parser) {
    return parser->parse_index;
}

bool endOfBuffer(CJPEGParse* parser) {
    return parser->end_of_buff;
}

uint8_t* getCurrentIndex(CJPEGParse* parser) {
    return parser->stream_buff + parser->parse_index;
}

void parserInitialize(CJPEGParse* parser,  uint8_t* stream_buff, uint32_t buff_size) {
    parser->parse_index = 0;
    parser->buff_size = buff_size;
    parser->stream_buff = stream_buff;
    parser->end_of_buff = false;
    parser->readNextByte = readNextByte;
    parser->readBytes = readBytes;
    parser->burnBytes = burnBytes;
    parser->getNextMarker = getNextMarker;
    parser->getByteOffset = getByteOffset;
    parser->endOfBuffer = endOfBuffer;
    parser->getCurrentIndex = getCurrentIndex;
    parser->setByteOffset= setByteOffset;
}
