/*
 * Copyright © 2012 Intel Corporation
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Li Zeng <li.zeng@intel.com>
 *    Jian Sun <jianx.sun@intel.com>
 */

#include <emmintrin.h>
#include <x86intrin.h>

inline void stream_memcpy(void* dst_buff, const void* src_buff, size_t size)
{
    bool isAligned = (((size_t)(src_buff) | (size_t)(dst_buff)) & 0xF) == 0;
    if (!isAligned) {
        memcpy(dst_buff, src_buff, size);
        return;
    }

    static const size_t regs_count = 8;

    __m128i xmm_data0, xmm_data1, xmm_data2, xmm_data3;
    __m128i xmm_data4, xmm_data5, xmm_data6, xmm_data7;

    size_t remain_data = size & (regs_count * sizeof(xmm_data0) - 1);
    size_t end_position = 0;

    __m128i* pWb_buff = (__m128i*)dst_buff;
    __m128i* pWb_buff_end = pWb_buff + ((size - remain_data) >> 4);
    __m128i* pWc_buff = (__m128i*)src_buff;

    /*sync the wc memory data*/
    _mm_mfence();

    while (pWb_buff < pWb_buff_end)
    {
        xmm_data0  = _mm_stream_load_si128(pWc_buff);
        xmm_data1  = _mm_stream_load_si128(pWc_buff + 1);
        xmm_data2  = _mm_stream_load_si128(pWc_buff + 2);
        xmm_data3  = _mm_stream_load_si128(pWc_buff + 3);
        xmm_data4  = _mm_stream_load_si128(pWc_buff + 4);
        xmm_data5  = _mm_stream_load_si128(pWc_buff + 5);
        xmm_data6  = _mm_stream_load_si128(pWc_buff + 6);
        xmm_data7  = _mm_stream_load_si128(pWc_buff + 7);

        pWc_buff += regs_count;
        _mm_store_si128(pWb_buff, xmm_data0);
        _mm_store_si128(pWb_buff + 1, xmm_data1);
        _mm_store_si128(pWb_buff + 2, xmm_data2);
        _mm_store_si128(pWb_buff + 3, xmm_data3);
        _mm_store_si128(pWb_buff + 4, xmm_data4);
        _mm_store_si128(pWb_buff + 5, xmm_data5);
        _mm_store_si128(pWb_buff + 6, xmm_data6);
        _mm_store_si128(pWb_buff + 7, xmm_data7);

        pWb_buff += regs_count;
    }

    /*copy data by 16 bytes step from the remainder*/
    if (remain_data >= 16)
    {
        size = remain_data;
        remain_data = size & 15;
        end_position = size >> 4;
        for (size_t i = 0; i < end_position; ++i)
        {
            pWb_buff[i] = _mm_stream_load_si128(pWc_buff + i);
        }
    }

    /*copy the remainder data, if it still existed*/
    if (remain_data)
    {
        __m128i temp_data = _mm_stream_load_si128(pWc_buff + end_position);

        char* psrc_buf = (char*)(&temp_data);
        char* pdst_buf = (char*)(pWb_buff + end_position);

        for (size_t i = 0; i < remain_data; ++i)
        {
            pdst_buf[i] = psrc_buf[i];
        }
    }

}
