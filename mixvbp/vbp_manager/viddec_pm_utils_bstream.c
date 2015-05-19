#include <stdint.h>
#include <vbp_common.h>
#include "viddec_pm_utils_bstream.h"

/* Internal data structure for calculating required bits. */
typedef union
{
    uint8_t byte[8];
    uint32_t word[2];
} viddec_pm_utils_getbits_t;

void viddec_pm_utils_bstream_reload(viddec_pm_utils_bstream_cxt_t *cxt);
uint32_t viddec_pm_utils_bstream_getphys(viddec_pm_utils_bstream_cxt_t *cxt, uint32_t pos, uint32_t lst_index);
extern uint32_t cp_using_dma(uintptr_t ddr_addr, uintptr_t local_addr, uint32_t size, char to_ddr, char swap);

static int32_t viddec_pm_utils_bstream_peekbits_noemul(viddec_pm_utils_bstream_cxt_t *cxt, uint32_t *out, uint32_t num_bits);
static int32_t viddec_pm_utils_bstream_peekbits_emul(viddec_pm_utils_bstream_cxt_t *cxt, uint32_t *out, uint32_t num_bits);
static int32_t viddec_pm_utils_bstream_getbits_noemul(viddec_pm_utils_bstream_cxt_t *cxt, uint32_t *out, uint32_t num_bits);
static int32_t viddec_pm_utils_bstream_getbits_emul(viddec_pm_utils_bstream_cxt_t *cxt, uint32_t *out, uint32_t num_bits);


/* Bytes left in cubby buffer which were not consumed yet */
static inline uint32_t viddec_pm_utils_bstream_bytesincubby(viddec_pm_utils_bstream_buf_cxt_t *cxt)
{
    return (cxt->buf_end - cxt->buf_index);
}

/*
  This function checks to see if we are at the last valid byte for current access unit.
*/
uint8_t viddec_pm_utils_bstream_nomorerbspdata(viddec_pm_utils_bstream_cxt_t *cxt)
{
    uint32_t data_remaining = 0;
    uint8_t ret = 0;

    /* How much data is remaining including current byte to be processed.*/
    data_remaining = cxt->list->total_bytes - (cxt->au_pos + (cxt->bstrm_buf.buf_index - cxt->bstrm_buf.buf_st));

    /* Start code prefix can be 000001 or 0000001. We always only check for 000001.
       data_reamining should be 1 for 000001, as we don't count sc prefix and 1 represents current byte.
       data_reamining should be 2 for 00000001, as we don't count sc prefix its current byte and extra 00 as we check for 000001.
       NOTE: This is used for H264 only.
    */
    switch (data_remaining)
    {
    case 2:
        /* If next byte is 0 and its the last byte in access unit */
        ret = (cxt->bstrm_buf.buf[cxt->bstrm_buf.buf_index+1] == 0x0);
        break;
    case 1:
        /* if the current byte is last byte */
        ret = 1;
        break;
    default:
        break;
    }
    return ret;
}


/* This function initializes scratch buffer, which is used for staging already read data, due to DMA limitations */
static inline void viddec_pm_utils_bstream_scratch_init(viddec_pm_utils_bstream_scratch_cxt_t *cxt)
{
    cxt->st = cxt->size = cxt->bitoff = 0;
}

/* This function populates requested number of bytes into data parameter, skips emulation prevention bytes if needed */
static inline int32_t viddec_pm_utils_getbytes(viddec_pm_utils_bstream_buf_cxt_t *bstream,
        viddec_pm_utils_getbits_t *data,/* gets populated with read bytes*/
        uint32_t *act_bytes, /* actual number of bytes read can be more due to emulation prev bytes*/
        uint32_t *phase,    /* Phase for emulation */
        uint32_t num_bytes,/* requested number of bytes*/
        uint32_t emul_reqd, /* On true we look for emulation prevention */
        uint8_t is_offset_zero /* Are we on aligned byte position for first byte*/
                                              )
{
    int32_t ret = 1;
    uint8_t cur_byte = 0, valid_bytes_read = 0;
    *act_bytes = 0;

    while (valid_bytes_read < num_bytes)
    {
        cur_byte = bstream->buf[bstream->buf_index + *act_bytes];
        if (emul_reqd && (cur_byte == 0x3) && (*phase == 2))
        {/* skip emulation byte. we update the phase only if emulation prevention is enabled */
            *phase = 0;
        }
        else
        {
            data->byte[valid_bytes_read] = cur_byte;
            /*
              We only update phase for first byte if bit offset is 0. If its not 0 then it was already accounted for in the past.
              From second byte onwards we always look to update phase.
             */
            if ((*act_bytes != 0) || (is_offset_zero))
            {
                if (cur_byte == 0)
                {
                    /* Update phase only if emulation prevention is required */
                    *phase += (((*phase < 2) && emul_reqd) ? 1: 0);
                }
                else
                {
                    *phase = 0;
                }
            }
            valid_bytes_read++;
        }
        *act_bytes +=1;
    }
    /* Check to see if we reached end during above operation. We might be out of range buts it's safe since our array
       has at least MIN_DATA extra bytes and the maximum out of bounds we will go is 5 bytes */
    if ((bstream->buf_index + *act_bytes -1) >= bstream->buf_end)
    {
        ret = -1;
    }
    return ret;
}

/*
  This function checks to see if we have minimum amount of data else tries to reload as much as it can.
  Always returns the data left in current buffer in parameter.
*/
static inline void viddec_pm_utils_check_bstream_reload(viddec_pm_utils_bstream_cxt_t *cxt, uint32_t *data_left)
{
    *data_left = viddec_pm_utils_bstream_bytesincubby(&(cxt->bstrm_buf));
}

/*
  This function moves the stream position by N bits(parameter bits). The bytes parameter tells us how many bytes were
  read for this N bits(can be different due to emulation bytes).
*/
static inline void viddec_pm_utils_update_skipoffsets(viddec_pm_utils_bstream_buf_cxt_t *bstream, uint32_t bits, uint32_t bytes)
{
    if ((bits & 0x7) == 0)
    {
        bstream->buf_bitoff = 0;
        bstream->buf_index += bytes;
    }
    else
    {
        bstream->buf_bitoff = bits & 0x7;
        bstream->buf_index += (bytes - 1);
    }
}

/*
  This function skips emulation byte if necessary.
  During Normal flow we skip emulation byte only if we read at least one bit after the the two zero bytes.
  However in some cases we might send data to HW without reading the next bit, in which case we are on
  emulation byte. To avoid sending invalid data, this function has to be called first to skip.
*/

void viddec_pm_utils_skip_if_current_is_emulation(viddec_pm_utils_bstream_cxt_t *cxt)
{
    viddec_pm_utils_bstream_buf_cxt_t *bstream = &(cxt->bstrm_buf);

    if (cxt->is_emul_reqd &&
            (cxt->phase >= 2) &&
            (bstream->buf_bitoff == 0) &&
            (bstream->buf[bstream->buf_index] == 0x3))
    {
        bstream->buf_index += 1;
        cxt->phase = 0;
    }
}


/*
  Init function called by parser manager after sc code detected.
*/
void viddec_pm_utils_bstream_init(viddec_pm_utils_bstream_cxt_t *cxt, viddec_pm_utils_list_t *list, uint32_t is_emul)
{

    cxt->emulation_byte_counter = 0;
    cxt->au_pos = 0;
    cxt->list = list;
    cxt->list_off = 0;
    cxt->phase = 0;
    cxt->is_emul_reqd = is_emul;
    cxt->bstrm_buf.buf_st = cxt->bstrm_buf.buf_end = cxt->bstrm_buf.buf_index = cxt->bstrm_buf.buf_bitoff = 0;
}

/* Get the requested byte position. If the byte is already present in cubby its returned
   else we seek forward and get the requested byte.
   Limitation:Once we seek forward we can't return back.
*/
int32_t viddec_pm_utils_bstream_get_current_byte(viddec_pm_utils_bstream_cxt_t *cxt, uint8_t *byte)
{
    int32_t ret = -1;
    uint32_t data_left=0;
    viddec_pm_utils_bstream_buf_cxt_t *bstream;

    bstream = &(cxt->bstrm_buf);
    viddec_pm_utils_check_bstream_reload(cxt, &data_left);
    if (data_left != 0)
    {
        *byte = bstream->buf[bstream->buf_index];
        ret = 1;
    }
    return ret;
}

/*
  Function to skip N bits ( N<= 32).
*/
int32_t viddec_pm_utils_bstream_skipbits(viddec_pm_utils_bstream_cxt_t *cxt, uint32_t num_bits)
{
    int32_t ret = -1;
    uint32_t data_left = 0;
    viddec_pm_utils_bstream_buf_cxt_t *bstream;

    bstream = &(cxt->bstrm_buf);
    viddec_pm_utils_check_bstream_reload(cxt, &data_left);
    if ((num_bits <= 32) && (num_bits > 0) && (data_left != 0))
    {
        uint8_t bytes_required=0;

        bytes_required = (bstream->buf_bitoff + num_bits + 7) >> 3;
        if (bytes_required <= data_left)
        {
            viddec_pm_utils_getbits_t data;
            uint32_t act_bytes =0;
            if (viddec_pm_utils_getbytes(bstream, &data, &act_bytes, &(cxt->phase), bytes_required, cxt->is_emul_reqd, (bstream->buf_bitoff == 0)) != -1)
            {
                uint32_t total_bits = 0;
                total_bits = num_bits + bstream->buf_bitoff;
                viddec_pm_utils_update_skipoffsets(bstream, total_bits, act_bytes);
                ret = 1;

                if (act_bytes > bytes_required)
                {
                    cxt->emulation_byte_counter = act_bytes - bytes_required;
                }
            }
        }
    }
    return ret;
}

/*
  Function to get N bits (N<= 32). This function will update the bitstream position.
*/
int32_t viddec_pm_utils_bstream_getbits(viddec_pm_utils_bstream_cxt_t *cxt, uint32_t *out, uint32_t num_bits)
{
    if (cxt->is_emul_reqd) {
        return viddec_pm_utils_bstream_getbits_emul(cxt, out, num_bits);
    } else {
        return viddec_pm_utils_bstream_getbits_noemul(cxt, out, num_bits);
    }
}

/*
  Function to get N bits (N<= 32).This function will NOT update the bitstream position.
*/
int32_t viddec_pm_utils_bstream_peekbits(viddec_pm_utils_bstream_cxt_t *cxt, uint32_t *out, uint32_t num_bits)
{
    if (cxt->is_emul_reqd) {
        return viddec_pm_utils_bstream_peekbits_emul(cxt, out, num_bits);
    } else {
        return viddec_pm_utils_bstream_peekbits_noemul(cxt, out, num_bits);
    }
}

static inline int32_t getbytes_noemul(viddec_pm_utils_bstream_buf_cxt_t *bstream,
        viddec_pm_utils_getbits_t *data,/* gets populated with read bytes*/
        uint32_t *act_bytes, /* actual number of bytes read can be more due to emulation prev bytes*/
        uint32_t *phase,    /* Phase for emulation */
        uint32_t num_bytes,/* requested number of bytes*/
        uint8_t is_offset_zero /* Are we on aligned byte position for first byte*/)
{
    int32_t ret = 1;
    uint8_t cur_byte = 0, valid_bytes_read = 0;
    *act_bytes = 0;
    while (valid_bytes_read < num_bytes)
    {
        cur_byte = bstream->buf[bstream->buf_index + *act_bytes];
        data->byte[valid_bytes_read] = cur_byte;
        valid_bytes_read++;
        *act_bytes +=1;
    }
    /* Check to see if we reached end during above operation. We might be out of range buts it safe since our array
       has at least MIN_DATA extra bytes and the maximum out of bounds we will go is 5 bytes */
    if ((bstream->buf_index + *act_bytes -1) >= bstream->buf_end)
    {
        ret = -1;
    }
    return ret;
}


/* This function populates requested number of bytes into data parameter, skips emulation prevention bytes if needed */
static inline int32_t getbytes_emul(viddec_pm_utils_bstream_buf_cxt_t *bstream,
        viddec_pm_utils_getbits_t *data,/* gets populated with read bytes*/
        uint32_t *act_bytes, /* actual number of bytes read can be more due to emulation prev bytes*/
        uint32_t *phase,    /* Phase for emulation */
        uint32_t num_bytes,/* requested number of bytes*/
        uint8_t is_offset_zero /* Are we on aligned byte position for first byte*/)
{
    int32_t ret = 1;
    uint8_t cur_byte = 0, valid_bytes_read = 0;
    uint32_t actual_bytes = 0;
    *act_bytes = 0;

    uint8_t *curr_pos = (uint8_t *)(bstream->buf + bstream->buf_index);

    while (valid_bytes_read < num_bytes)
    {
        cur_byte = *curr_pos++;
   //     ITRACE("getbytes_emul cur_byte = 0x%x", cur_byte);
        if ((cur_byte == 0x3) && (*phase == 2))
        {/* skip emulation byte. we update the phase only if emulation prevention is enabled */
            *phase = 0;
        }
        else
        {
            data->byte[valid_bytes_read] = cur_byte;
            /*
                    We only update phase for first byte if bit offset is 0. If its not 0 then it was already accounted for in the past.
                    From second byte onwards we always look to update phase.
                    */
            if ((actual_bytes != 0) || (is_offset_zero))
            {
                if (cur_byte == 0)
                {
                    /* Update phase only if emulation prevention is required */
                    *phase += (*phase < 2 ? 1:0 );
                }
                else
                {
                    *phase=0;
                }
            }
            valid_bytes_read++;
        }
        actual_bytes++;
    }
    /*
        Check to see if we reached end during above operation. We might be out of range buts it safe since our array
        has at least MIN_DATA extra bytes and the maximum out of bounds we will go is 5 bytes
       */

    if ((bstream->buf_index + actual_bytes -1) >= bstream->buf_end)
    {
        ret = -1;
    }
    *act_bytes = actual_bytes;
    return ret;
}

static int32_t viddec_pm_utils_bstream_getbits_emul(viddec_pm_utils_bstream_cxt_t *cxt, uint32_t *out, uint32_t num_bits)
{

    uint32_t data_left=0;
    int32_t ret = -1;

    /* STEP 1: Make sure that we have at least minimum data before we calculate bits */
    viddec_pm_utils_bstream_buf_cxt_t *bstream;

    bstream = &(cxt->bstrm_buf);
    data_left = bstream->buf_end - bstream->buf_index;

    uint32_t bytes_required=0;
    uint32_t act_bytes = 0;
    uint32_t phase;
    viddec_pm_utils_getbits_t data;

    if ((num_bits <= 32) && (num_bits > 0) && (data_left != 0))
    {
        bytes_required = (bstream->buf_bitoff + num_bits + 7)>>3;

        /* Step 2: Make sure we have bytes for requested bits */
        if (bytes_required <= data_left)
        {
            phase = cxt->phase;
            /* Step 3: Due to emualtion prevention bytes sometimes the bytes_required > actual_required bytes */
            if (getbytes_emul(bstream, &data, &act_bytes, &phase, bytes_required, (bstream->buf_bitoff == 0)) != -1)
            {
                uint32_t total_bits=0;
                uint32_t shift_by=0;
                /* zero out upper bits */
                /* LIMITATION:For some reason compiler is optimizing it to NOP if i do both shifts
                   in single statement */
                data.byte[0] <<= bstream->buf_bitoff;
                data.byte[0] >>= bstream->buf_bitoff;
                data.word[0] = SWAP_WORD(data.word[0]);
                data.word[1] = SWAP_WORD(data.word[1]);
                total_bits = num_bits+bstream->buf_bitoff;

                if (total_bits > 32)
                {
                    /* We have to use both the words to get required data */
                    shift_by = total_bits - 32;
                    data.word[0] = (data.word[0] << shift_by) | ( data.word[1] >> (32 - shift_by));
                }
                else
                {
                    shift_by = 32 - total_bits;
                    data.word[0] = data.word[0] >> shift_by;
                }
                *out = data.word[0];

                /* update au byte position if needed */
                if ((total_bits & 0x7) == 0)
                {
                    bstream->buf_bitoff = 0;
                    bstream->buf_index +=act_bytes;
                }
                else
                {
                    bstream->buf_bitoff = total_bits & 0x7;
                    bstream->buf_index +=(act_bytes - 1);
                }
                cxt->phase = phase;
                if (act_bytes > bytes_required)
                {
                    cxt->emulation_byte_counter += act_bytes - bytes_required;
                }

                ret=1;
            }
        }
    }
    return ret;

}

static int32_t viddec_pm_utils_bstream_getbits_noemul(viddec_pm_utils_bstream_cxt_t *cxt, uint32_t *out, uint32_t num_bits)
{
    uint32_t data_left=0;
    int32_t ret = -1;
    /* STEP 1: Make sure that we have at least minimum data before we calculate bits */

    viddec_pm_utils_bstream_buf_cxt_t *bstream;

    bstream = &(cxt->bstrm_buf);
    data_left = bstream->buf_end - bstream->buf_index;
    uint32_t bytes_required=0;
    if ((num_bits <= 32) && (num_bits > 0) && (data_left != 0))
    {
        bytes_required = (bstream->buf_bitoff + num_bits + 7)>>3;

        /* Step 2: Make sure we have bytes for requested bits */
        if (bytes_required <= data_left)
        {
            uint32_t act_bytes, phase;
            viddec_pm_utils_getbits_t data;
            phase = cxt->phase;
            /* Step 3: Due to emualtion prevention bytes sometimes the bytes_required > actual_required bytes */
            if (getbytes_noemul(bstream, &data, &act_bytes, &phase, bytes_required, (bstream->buf_bitoff == 0)) != -1)
            {
                uint32_t total_bits=0;
                uint32_t shift_by=0;
                /* zero out upper bits */
                /* LIMITATION:For some reason compiler is optimizing it to NOP if i do both shifts
                   in single statement */
                data.byte[0] <<= bstream->buf_bitoff;
                data.byte[0] >>= bstream->buf_bitoff;
                data.word[0] = SWAP_WORD(data.word[0]);
                data.word[1] = SWAP_WORD(data.word[1]);
                total_bits = num_bits+bstream->buf_bitoff;
                if (total_bits > 32)
                {
                    /* We have to use both the words to get required data */
                    shift_by = total_bits - 32;
                    data.word[0] = (data.word[0] << shift_by) | ( data.word[1] >> (32 - shift_by));
                }
                else
                {
                    shift_by = 32 - total_bits;
                    data.word[0] = data.word[0] >> shift_by;
                }
                *out = data.word[0];

                /* update au byte position if needed */
                if ((total_bits & 0x7) == 0)
                {
                    bstream->buf_bitoff = 0;
                    bstream->buf_index +=act_bytes;
                }
                else
                {
                    bstream->buf_bitoff = total_bits & 0x7;
                    bstream->buf_index +=(act_bytes - 1);
                }
                cxt->phase = phase;
                if (act_bytes > bytes_required)
                {
                    cxt->emulation_byte_counter += act_bytes - bytes_required;
                }

                ret =1;
            }
        }
    }
    return ret;

}

static int32_t viddec_pm_utils_bstream_peekbits_emul(viddec_pm_utils_bstream_cxt_t *cxt, uint32_t *out, uint32_t num_bits)
{
    uint32_t data_left=0;
    int32_t ret = -1;
    /* STEP 1: Make sure that we have at least minimum data before we calculate bits */

    viddec_pm_utils_bstream_buf_cxt_t *bstream;

    bstream = &(cxt->bstrm_buf);
    data_left = bstream->buf_end - bstream->buf_index;

    uint32_t act_bytes = 0, phase;
    viddec_pm_utils_getbits_t data;
    uint32_t bytes_required=0;

    if ((num_bits <= 32) && (num_bits > 0) && (data_left != 0))
    {
        uint32_t bytes_required=0;
        viddec_pm_utils_bstream_buf_cxt_t *bstream;

        bstream = &(cxt->bstrm_buf);
        bytes_required = (bstream->buf_bitoff + num_bits + 7)>>3;

        /* Step 2: Make sure we have bytes for requested bits */
        if (bytes_required <= data_left)
        {
            phase = cxt->phase;
            /* Step 3: Due to emualtion prevention bytes sometimes the bytes_required > actual_required bytes */
            if (getbytes_emul(bstream, &data, &act_bytes, &phase, bytes_required, (bstream->buf_bitoff == 0)) != -1)
            {
                uint32_t total_bits=0;
                uint32_t shift_by=0;
                /* zero out upper bits */
                /* LIMITATION:For some reason compiler is optimizing it to NOP if i do both shifts
                   in single statement */
                data.byte[0] <<= bstream->buf_bitoff;
                data.byte[0] >>= bstream->buf_bitoff;

                data.word[0] = SWAP_WORD(data.word[0]);
                data.word[1] = SWAP_WORD(data.word[1]);
                total_bits = num_bits+bstream->buf_bitoff;
                if (total_bits > 32)
                {
                    /* We have to use both the words to get required data */
                    shift_by = total_bits - 32;
                    data.word[0] = (data.word[0] << shift_by) | ( data.word[1] >> (32 - shift_by));
                }
                else
                {
                    shift_by = 32 - total_bits;
                    data.word[0] = data.word[0] >> shift_by;
                }
                *out = data.word[0];

                ret =1;
            }
        }
    }
    return ret;
}

static int32_t viddec_pm_utils_bstream_peekbits_noemul(viddec_pm_utils_bstream_cxt_t *cxt, uint32_t *out, uint32_t num_bits)
{
    uint32_t data_left=0;
    int32_t ret = -1;
    /* STEP 1: Make sure that we have at least minimum data before we calculate bits */
    //viddec_pm_utils_check_bstream_reload(cxt, &data_left);

    viddec_pm_utils_bstream_buf_cxt_t *bstream;

    bstream = &(cxt->bstrm_buf);
    data_left = bstream->buf_end - bstream->buf_index;
    uint32_t bytes_required=0;

    if ((num_bits <= 32) && (num_bits > 0) && (data_left != 0))
    {
        uint32_t bytes_required=0;
        viddec_pm_utils_bstream_buf_cxt_t *bstream;

        bstream = &(cxt->bstrm_buf);
        bytes_required = (bstream->buf_bitoff + num_bits + 7)>>3;

        /* Step 2: Make sure we have bytes for requested bits */
        if (bytes_required <= data_left)
        {
            uint32_t act_bytes, phase;
            viddec_pm_utils_getbits_t data;
            phase = cxt->phase;
            /* Step 3: Due to emualtion prevention bytes sometimes the bytes_required > actual_required bytes */
            if (getbytes_noemul(bstream, &data, &act_bytes, &phase, bytes_required, (bstream->buf_bitoff == 0)) != -1)
            {
                uint32_t total_bits=0;
                uint32_t shift_by=0;
                /* zero out upper bits */
                /* LIMITATION:For some reason compiler is optimizing it to NOP if i do both shifts
                   in single statement */
                data.byte[0] <<= bstream->buf_bitoff;
                data.byte[0] >>= bstream->buf_bitoff;

                data.word[0] = SWAP_WORD(data.word[0]);
                data.word[1] = SWAP_WORD(data.word[1]);
                total_bits = num_bits+bstream->buf_bitoff;
                if (total_bits > 32)
                {
                    /* We have to use both the words to get required data */
                    shift_by = total_bits - 32;
                    data.word[0] = (data.word[0] << shift_by) | ( data.word[1] >> (32 - shift_by));
                }
                else
                {
                    shift_by = 32 - total_bits;
                    data.word[0] = data.word[0] >> shift_by;
                }
                *out = data.word[0];

                ret = 1;
            }
        }
    }
    return ret;
}
