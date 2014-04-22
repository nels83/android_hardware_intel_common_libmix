#include <stdint.h>
#include <vbp_common.h>
#include "viddec_pm.h"
#include "viddec_parser_ops.h"
#include "viddec_pm_utils_bstream.h"
#include "viddec_fw_common_defs.h"
#include <vbp_trace.h>

int32_t viddec_pm_get_bits(void *parent, uint32_t *data, uint32_t num_bits)
{
    int32_t ret = 1;
    viddec_pm_cxt_t *cxt;

    cxt = (viddec_pm_cxt_t *)parent;
#ifdef PARSER_OPT
    if (cxt->left_bnt < num_bits) {
        uint32_t load_word = 0;
        uint32_t load_bits = 32-cxt->left_bnt;
        uint32_t bits_left = (cxt->getbits.bstrm_buf.buf_end-cxt->getbits.bstrm_buf.buf_index) * 8 - cxt->getbits.bstrm_buf.buf_bitoff;
        load_bits = bits_left > load_bits ? load_bits:bits_left;
        ret = viddec_pm_utils_bstream_getbits(&(cxt->getbits), &load_word, load_bits);
        if (ret == -1) {
            VTRACE("FAILURE: getbits returned %d", ret);
            return ret;
        }
        cxt->cached_word |= (load_word << (32-cxt->left_bnt-load_bits));
        cxt->left_bnt  += load_bits;
    }
    *data = (cxt->cached_word >> (32 - num_bits));
    if (num_bits == 32) {
        cxt->cached_word = 0;
    } else {
        cxt->cached_word <<= num_bits;
    }
    cxt->left_bnt -= num_bits;
#else
    ret = viddec_pm_utils_bstream_getbits(&(cxt->getbits), data, num_bits);
    if (ret == -1)
    {
        VTRACE("FAILURE: getbits returned %d", ret);
    }
#endif
    return ret;
}

int32_t viddec_pm_peek_bits(void *parent, uint32_t *data, uint32_t num_bits)
{
    int32_t ret = 1;
    viddec_pm_cxt_t *cxt;

    cxt = (viddec_pm_cxt_t *)parent;
#ifdef PARSER_OPT
    if (cxt->left_bnt < num_bits) {
        uint32_t load_word = 0;
        uint32_t load_bits = 32 - cxt->left_bnt;
        uint32_t bits_left = (cxt->getbits.bstrm_buf.buf_end-cxt->getbits.bstrm_buf.buf_index) * 8
                             - cxt->getbits.bstrm_buf.buf_bitoff;
        load_bits = bits_left > load_bits ? load_bits:bits_left;
        ret = viddec_pm_utils_bstream_getbits(&(cxt->getbits), &load_word, load_bits);
        if (ret == -1) {
            VTRACE("FAILURE: peekbits returned %d, %d, %d", ret, cxt->left_bnt, num_bits);
            return ret;
        }
        cxt->cached_word |= (load_word << (32-cxt->left_bnt-load_bits));
        cxt->left_bnt += load_bits;
    }
    *data = (cxt->cached_word >> (32 - num_bits));
#else
    ret = viddec_pm_utils_bstream_peekbits(&(cxt->getbits), data, num_bits);
    if (ret == -1)
    {
        VTRACE("FAILURE: peekbits returned %d", ret);
    }
#endif
    return ret;
}

int32_t viddec_pm_skip_bits(void *parent, uint32_t num_bits)
{
    int32_t ret = 1;
    viddec_pm_cxt_t *cxt;

    cxt = (viddec_pm_cxt_t *)parent;
#ifdef PARSER_OPT
    if (num_bits <= cxt->left_bnt) {
        cxt->left_bnt -= num_bits;
        if (num_bits < 32) {
           cxt->cached_word <<= num_bits;
        } else {
           cxt->cached_word = 0;
        }
    } else {
        ret = viddec_pm_utils_bstream_skipbits(&(cxt->getbits), num_bits-cxt->left_bnt);
        cxt->left_bnt = 0;
        cxt->cached_word = 0;
    }
#else
    ret = viddec_pm_utils_bstream_skipbits(&(cxt->getbits), num_bits);
#endif
    return ret;
}

int32_t viddec_pm_get_au_pos(void *parent, uint32_t *bit, uint32_t *byte, uint8_t *is_emul)
{
    int32_t ret = 1;
    viddec_pm_cxt_t *cxt;

    cxt = (viddec_pm_cxt_t *)parent;
#ifdef PARSER_OPT
    if (!cxt->left_bnt) {
        viddec_pm_utils_skip_if_current_is_emulation(&(cxt->getbits));
        viddec_pm_utils_bstream_get_au_offsets(&(cxt->getbits), bit, byte, is_emul);
    } else {
        viddec_pm_utils_bstream_get_au_offsets(&(cxt->getbits), bit, byte, is_emul);
        uint32_t offset = *bit + *byte * 8 - cxt->left_bnt;
        *bit = offset & 7;
        *byte = offset >> 3;
    }
#else
    viddec_pm_utils_skip_if_current_is_emulation(&(cxt->getbits));
    viddec_pm_utils_bstream_get_au_offsets(&(cxt->getbits), bit, byte, is_emul);
#endif
    return ret;

}

int32_t viddec_pm_is_nomoredata(void *parent)
{
    int32_t ret=0;
    viddec_pm_cxt_t *cxt;
    cxt = (viddec_pm_cxt_t *)parent;
#ifdef PARSER_OPT
    uint32_t bits_left = (cxt->getbits.bstrm_buf.buf_end-cxt->getbits.bstrm_buf.buf_index) * 8
                   - cxt->getbits.bstrm_buf.buf_bitoff + cxt->left_bnt;
    uint32_t byte_left = (bits_left+7) >> 3;
    switch (byte_left)
    {
        case 2:
           ret = (cxt->getbits.bstrm_buf.buf[cxt->getbits.bstrm_buf.buf_end-1] == 0x0);
           break;
        case 1:
           ret = 1;
           break;
        default:
           break;
    }
#else
    ret = viddec_pm_utils_bstream_nomorerbspdata(&(cxt->getbits));
#endif
    return ret;
}

uint32_t viddec_pm_get_cur_byte(void *parent, uint8_t *byte)
{
    int32_t ret=-1;
    viddec_pm_cxt_t *cxt;
    cxt = (viddec_pm_cxt_t *)parent;
    ret = viddec_pm_utils_bstream_get_current_byte(&(cxt->getbits), byte);
    return ret;
}

void viddec_pm_set_next_frame_error_on_eos(void *parent, uint32_t error)
{
    viddec_pm_cxt_t *cxt;
    cxt = (viddec_pm_cxt_t *)parent;
    cxt->next_workload_error_eos = error;
}

void viddec_pm_set_late_frame_detect(void *parent)
{
    viddec_pm_cxt_t *cxt;
    cxt = (viddec_pm_cxt_t *)parent;
    cxt->late_frame_detect = true;
}

