#include <stdint.h>
#include <vbp_common.h>
#include "viddec_pm.h"
#include "viddec_parser_ops.h"
#include "viddec_pm_utils_bstream.h"
#include "viddec_fw_common_defs.h"

int32_t viddec_pm_get_bits(void *parent, uint32_t *data, uint32_t num_bits)
{
    int32_t ret = 1;
    viddec_pm_cxt_t *cxt;

    cxt = (viddec_pm_cxt_t *)parent;
    ret = viddec_pm_utils_bstream_peekbits(&(cxt->getbits), data, num_bits, 1);
    if (ret == -1)
    {
        DEB("FAILURE!!!! getbits returned %d\n", ret);
    }

    return ret;
}

int32_t viddec_pm_peek_bits(void *parent, uint32_t *data, uint32_t num_bits)
{
    int32_t ret = 1;
    viddec_pm_cxt_t *cxt;

    cxt = (viddec_pm_cxt_t *)parent;
    ret = viddec_pm_utils_bstream_peekbits(&(cxt->getbits), data, num_bits, 0);
    return ret;
}

int32_t viddec_pm_skip_bits(void *parent, uint32_t num_bits)
{
    int32_t ret = 1;
    viddec_pm_cxt_t *cxt;

    cxt = (viddec_pm_cxt_t *)parent;
    ret = viddec_pm_utils_bstream_skipbits(&(cxt->getbits), num_bits);
    return ret;
}

int32_t viddec_pm_get_au_pos(void *parent, uint32_t *bit, uint32_t *byte, uint8_t *is_emul)
{
    int32_t ret = 1;
    viddec_pm_cxt_t *cxt;

    cxt = (viddec_pm_cxt_t *)parent;
    viddec_pm_utils_skip_if_current_is_emulation(&(cxt->getbits));
    viddec_pm_utils_bstream_get_au_offsets(&(cxt->getbits), bit, byte, is_emul);

    return ret;

}

int32_t viddec_pm_is_nomoredata(void *parent)
{
    int32_t ret=0;
    viddec_pm_cxt_t *cxt;

    cxt = (viddec_pm_cxt_t *)parent;
    ret = viddec_pm_utils_bstream_nomorerbspdata(&(cxt->getbits));
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

