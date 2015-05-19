/* ///////////////////////////////////////////////////////////////////////
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2001-2006 Intel Corporation. All Rights Reserved.
//
//  Description:    h264 bistream decoding
//
///////////////////////////////////////////////////////////////////////*/


#include "h264.h"
#include "h264parse.h"
#include "viddec_parser_ops.h"
#include "viddec_pm_utils_bstream.h"
#include "viddec_pm.h"
#include "vbp_trace.h"


/**
   get_codeNum     :Get codenum based on sec 9.1 of H264 spec.
   @param      cxt : Buffer adress & size are part inputs, the cxt is updated
                     with codeNum & sign on sucess.
                     Assumption: codeNum is a max of 32 bits

   @retval       1 : Sucessfuly found a code num, cxt is updated with codeNum, sign, and size of code.
   @retval       0 : Couldn't find a code in the current buffer.
   be freed.
*/

uint32_t h264_get_codeNum(void *parent, h264_Info* pInfo)
{
    int32_t    leadingZeroBits= 0;
    uint32_t   temp = 0, match = 0, noOfBits = 0, count = 0;
    uint32_t   codeNum =0;
    uint32_t   bits_offset =0, byte_offset =0;
    uint8_t    is_emul =0;
    uint8_t    is_first_byte = 1;
    uint32_t   length =0;
    uint32_t   bits_need_add_in_first_byte =0;
    int32_t    bits_operation_result=0;

    viddec_pm_utils_bstream_cxt_t *cxt = &((viddec_pm_cxt_t *)parent)->getbits;
    viddec_pm_utils_bstream_buf_cxt_t* bstream = &cxt->bstrm_buf;
    uint8_t curr_byte;

    bits_offset = bstream->buf_bitoff;

    uint32_t total_bits, act_bytes;
    uint32_t isemul = 0;
    uint8_t *curr_addr = bstream->buf + bstream->buf_index;

    uint32_t i = 0;
    VTRACE("bstream->buf_bitoff = %d", bstream->buf_bitoff);
    VTRACE("bstream->buf_index = %d", bstream->buf_index);
#ifdef PARSER_OPT
    viddec_pm_cxt_t *pm_cxt = (viddec_pm_cxt_t *)parent;
    int32_t cached_word = pm_cxt->cached_word;
    int32_t left_bnt = pm_cxt->left_bnt;
    if (left_bnt != 0) {
        if (cached_word == 0) {
            leadingZeroBits += left_bnt;
            left_bnt = 0;
        } else {
            match = 1;
            count = 1;
            left_bnt --;
            while (((cached_word & 0x80000000) != 0x80000000) && (left_bnt > 0)) {
                cached_word <<= 1;
                count ++;
                left_bnt --;
            }
            cached_word <<= 1;
            leadingZeroBits += count;
        }
        pm_cxt->cached_word = cached_word;
        pm_cxt->left_bnt = left_bnt;
    }
#endif
    while (!match)
    {
        curr_byte = *curr_addr++;
        VTRACE("curr_byte = 0x%x", curr_byte);
        if (cxt->phase >= 2 && curr_byte == 0x03) {
            curr_byte = *curr_addr++;
            isemul = 1;
            cxt->phase = 0;
        }
        noOfBits = 8;
        if (is_first_byte)
        {
            is_first_byte = 0;
            if (bits_offset != 0)
            {
                noOfBits = 8 - bits_offset;
                curr_byte = curr_byte << bits_offset;
            }
        }
        else
        {
            cxt->phase = curr_byte? 0: cxt->phase + 1;
        }

        if (curr_byte != 0)
        {
            count=1;
            VTRACE("curr_byte & 0x80 = 0x%x", curr_byte & 0x80);
            while (((curr_byte & 0x80) != 0x80) && (count <= noOfBits))
            {
                VTRACE("curr_byte & 0x80 = 0x%x", curr_byte & 0x80);
                count++;
                curr_byte = curr_byte <<1;
            }
            match = 1;
            leadingZeroBits += count;
        }
        else
        {
            leadingZeroBits += noOfBits;
        }

        VTRACE("count = %d", count);

        total_bits = match ? count : noOfBits;
        total_bits = noOfBits == 8? total_bits: total_bits + bits_offset;

        VTRACE("total_bits = %d", total_bits);

        act_bytes = 1 + isemul;
        cxt->emulation_byte_counter += isemul;
        isemul = 0;
        if ((total_bits & 0x7) == 0)
        {
            bstream->buf_bitoff = 0;
            bstream->buf_index +=act_bytes;
        }
        else
        {
            bstream->buf_bitoff = total_bits & 0x7;
            bstream->buf_index += (act_bytes - 1);
        }
    }

    VTRACE("leadingZeroBits = %d", leadingZeroBits);
    VTRACE("bstream->buf_bitoff = %x", bstream->buf_bitoff);
    VTRACE("bstream->buf_index = %x", bstream->buf_index);

    if (match)
    {
        length = --leadingZeroBits;
        codeNum = 0;
        if (length > 0)
        {
            bits_operation_result = viddec_pm_get_bits(parent, &temp, leadingZeroBits);
            if (-1 == bits_operation_result)
            {
                VTRACE("h264_get_codeNum: viddec_pm_get_bits error!");
                length = 0;
            }
            codeNum = temp;
        }
        codeNum = codeNum + (1 << length) -1;
    }

    return codeNum;
}


/*---------------------------------------*/
/*---------------------------------------*/
int32_t h264_GetVLCElement(void *parent, h264_Info* pInfo, uint8_t bIsSigned)
{
    int32_t sval = 0;
    signed char sign;

    sval = h264_get_codeNum(parent , pInfo);

    if (bIsSigned) //get signed integer golomb code else the value is unsigned
    {
        sign = (sval & 0x1)?1:-1;
        sval = (sval +1) >> 1;
        sval = sval * sign;
    }

    return sval;
} // Ipp32s H264Bitstream::GetVLCElement(bool bIsSigned)

///
/// Check whether more RBSP data left in current NAL
///
uint8_t h264_More_RBSP_Data(void *parent, h264_Info * pInfo)
{
    uint8_t cnt = 0;

    uint8_t  is_emul =0;
    uint8_t  cur_byte = 0;
    int32_t  shift_bits =0;
    uint32_t ctr_bit = 0;
    uint32_t bits_offset =0, byte_offset =0;

    //remove warning
    pInfo = pInfo;

    if (!viddec_pm_is_nomoredata(parent))
        return 1;

    viddec_pm_get_au_pos(parent, &bits_offset, &byte_offset, &is_emul);

    shift_bits = 7-bits_offset;

#ifndef PARSER_OPT
    // read one byte
    viddec_pm_get_cur_byte(parent, &cur_byte);
#else
    viddec_pm_peek_bits(parent, &cur_byte, 8-bits_offset);
#endif

    ctr_bit = ((cur_byte)>> (shift_bits--)) & 0x01;

    // a stop bit has to be one
    if (ctr_bit == 0)
        return 1;

    while (shift_bits >= 0 && !cnt)
    {
        cnt |= (((cur_byte)>> (shift_bits--)) & 0x01);   // set up control bit
    }

    return (cnt);
}
