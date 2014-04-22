#ifndef VIDDEC_PM_H
#define VIDDEC_PM_H

#include <stdint.h>
#include "viddec_pm_utils_list.h"
#include "viddec_pm_utils_bstream.h"
#include "viddec_pm_parse.h"
#include "viddec_parser_ops.h"

#define SC_DETECT_BUF_SIZE 1024
#define MAX_CODEC_CXT_SIZE 4096




/* This structure holds all necessary data required by parser manager for stream parsing.
 */
typedef struct
{
    /* Actual buffer where data gets DMA'd. 8 padding bytes for alignment */
    uint8_t scbuf[SC_DETECT_BUF_SIZE + 8];
    viddec_sc_parse_cubby_cxt_t parse_cubby;
    viddec_pm_utils_list_t list;
    viddec_pm_utils_bstream_cxt_t getbits;
    //viddec_emitter emitter;
    viddec_sc_prefix_state_t sc_prefix_info;
    uint8_t word_align_dummy;
    uint8_t late_frame_detect;
    uint8_t frame_start_found;
    uint32_t next_workload_error_eos;
#ifdef PARSER_OPT
    uint32_t cached_word;
    uint32_t left_bnt;
#endif
    uint32_t codec_data[MAX_CODEC_CXT_SIZE<<3];
} viddec_pm_cxt_t;


#endif
