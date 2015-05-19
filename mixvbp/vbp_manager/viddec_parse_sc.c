#include "viddec_pm_parse.h"

uint32_t viddec_parse_sc(void *in, void *pcxt, void *sc_state)
{
    uint8_t *ptr;
    uint32_t data_left=0, phase = 0, ret = 0;
    uint32_t single_byte_table[3][2] = {{1, 0}, {2, 0}, {2, 3}};
    viddec_sc_parse_cubby_cxt_t *cxt;
    /* What is phase?: phase is a value between [0-4], we keep track of consecutive '0's with this.
       Any time a '0' is found its incremented by 1(uptp 2) and reset to '0' if a zero not found.
       if 0xXX code is found and current phase is 2, its changed to 3 which means we found the pattern
       we are looking for. Its incremented to 4 once we see a byte after this pattern */
    cxt = ( viddec_sc_parse_cubby_cxt_t *)in;
    data_left = cxt->size;
    ptr = cxt->buf;
    phase = cxt->phase;
    cxt->sc_end_pos = -1;
    pcxt=pcxt;

    /* parse until there is more data and start code not found */
    while ((data_left > 0) && (phase < 3))
    {
        /* Check if we are 16 bytes aligned & phase=0 & more than 16 bytes left,
           if thats the case we can check work at a time instead of byte */

        if (((((uintptr_t)ptr) & 0xF) == 0) && (phase == 0) && (data_left > 0xF))
        {
            // 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00      -- check 16 bytes at one time
            // 00 ?? 00 ?? 00 ?? 00 ?? 00 ?? 00 ?? 00 ?? 00 ??      -- if no 00 at byte position: 15,13,11,09,07,05,03,01
            // it is impossible to have 0x010000 at these 16 bytes.
            // so we cound drop 16 bytes one time (increase ptr, decrease data_left and keep phase = 0)
            __asm__(
            //Data input
            "movl %1, %%ecx\n\t"                   //ptr-->ecx
            "movl %0, %%eax\n\t"                   //data_left-->eax

            //Main compare loop
            "MATCH_8_ZERO:\n\t"
            "pxor %%xmm0,%%xmm0\n\t"               //0 --> xmm0
            "pcmpeqb (%%ecx),%%xmm0\n\t"           //uint128_data[ptr] eq xmm0 --> xmm0 , For each byte do calculation,  (byte == 0x00)?0xFF:0x00
            "pmovmskb %%xmm0, %%edx\n\t"           //xmm0(128)-->edx(32), edx[0]=xmm0[7], edx[1]=xmm0[15], ... , edx[15]=xmm0[127], edx[31-16]=0x0000
            "test $0xAAAA, %%edx\n\t"              //edx& 1010 1010 1010 1010b
            "jnz DATA_RET\n\t"                     //Not equal to zero means that at least one byte is 0x00.

            "PREPARE_NEXT_MATCH:\n\t"
            "add $0x10, %%ecx\n\t"                 //16 + ecx --> ecx
            "sub $0x10, %%eax\n\t"                 //eax-16 --> eax
            "cmp $0x10, %%eax\n\t"                 //eax >= 16?
            "jge MATCH_8_ZERO\n\t"                 //search next 16 bytes
            "jmp END\n\t"

            "DATA_RET:\n\t"
            "test $0xd555, %%edx\n\t"
            "jz PREPARE_NEXT_MATCH\n\t"

            "END:\n\t"
            "movl %%ecx, %1\n\t"                   //ecx --> ptr
            "movl %%eax, %0\n\t"                   //eax --> data_left
            : "+m"(data_left), "+m"(ptr)           //data_left --> eax, ptr -> ecx
            :
            :"eax", "ecx", "edx", "xmm0"
            );

            if (data_left <= 0)
            {
                 break;
            }
        }

        //check byte one by one
        //  (*ptr)    0       1      >=2
        // phase=0    1       0      0
        // phase=1    2       0      0
        // phase=2    2       3      0
        if (*ptr >= 2)
        {
            phase = 0;
        }
        else
        {
            phase = single_byte_table[phase][*ptr];
        }
        ptr ++;
        data_left --;
    }
    if ((data_left > 0) && (phase == 3))
    {
        viddec_sc_prefix_state_t *state = (viddec_sc_prefix_state_t *)sc_state;
        cxt->sc_end_pos = cxt->size - data_left;
        state->next_sc = cxt->buf[cxt->sc_end_pos];
        state->second_scprfx_length = 3;
        phase++;
        ret = 1;
    }
    cxt->phase = phase;
    /* Return SC found only if phase is 4, else always success */
    return ret;
}

