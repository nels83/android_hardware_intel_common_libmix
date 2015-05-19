#ifndef VIDDEC_PARSER_OPS_H
#define VIDDEC_PARSER_OPS_H

#include <stdint.h>

#define VIDDEC_PARSE_INVALID_POS 0xFFFFFFFF

typedef enum
{
    VIDDEC_PARSE_EOS = 0x0FFF, /* Dummy start code to force EOS */
    VIDDEC_PARSE_DISCONTINUITY,  /* Dummy start code to force completion and flush */
} viddec_parser_inband_messages_t;

typedef struct
{
    uint32_t context_size;
    uint32_t persist_size;
} viddec_parser_memory_sizes_t;

typedef    void  (*fn_init)(void *ctxt, uint32_t *persist, uint32_t preserve);
typedef    uint32_t (*fn_parse_sc) (void *ctxt, void *pcxt, void *sc_state);
typedef    uint32_t (*fn_parse_syntax) (void *parent, void *ctxt);
typedef    void (*fn_get_cxt_size) (viddec_parser_memory_sizes_t *size);
typedef    uint32_t (*fn_is_wkld_done)(void *parent, void *ctxt, uint32_t next_sc, uint32_t *codec_specific_errors);
typedef    uint32_t (*fn_is_frame_start)(void *ctxt);
typedef    uint32_t (*fn_gen_contrib_tags)(void *parent, uint32_t ignore_partial);
typedef    uint32_t (*fn_gen_assoc_tags)(void *parent);
typedef    void (*fn_flush_parser) (void *parent, void *ctxt);
#if (defined USE_AVC_SHORT_FORMAT || defined USE_SLICE_HEADER_PARSING)
typedef    uint32_t (*fn_update_data)(void *parent, void *data, uint32_t size);
#endif

typedef    uint32_t (*fn_is_payload_start)(void *parent);
typedef    uint32_t (*fn_parse_syntax_threading) (void *parent, void *ctxt, uint32_t thread_index);
typedef    uint32_t (*fn_post_parse_threading) (void *parent, void *ctxt, uint32_t slice_index);
typedef    uint32_t (*fn_query_thread_parsing_cap) (void);

typedef struct
{
    fn_init init;
    fn_parse_sc parse_sc;
    fn_parse_syntax parse_syntax;
    fn_get_cxt_size get_cxt_size;
    fn_is_wkld_done is_wkld_done;
    fn_is_frame_start is_frame_start;
    fn_gen_contrib_tags gen_contrib_tags;
    fn_gen_assoc_tags gen_assoc_tags;
    fn_flush_parser flush;
#if (defined USE_AVC_SHORT_FORMAT || defined USE_SLICE_HEADER_PARSING)
    fn_update_data update_data;
#endif
    fn_is_payload_start is_payload_start;
    fn_parse_syntax_threading parse_syntax_threading;
    fn_post_parse_threading post_parse_threading;
    fn_query_thread_parsing_cap query_thread_parsing_cap;
} viddec_parser_ops_t;


typedef enum
{
    VIDDEC_PARSE_ERROR = 0xF0,
    VIDDEC_PARSE_SUCESS = 0xF1,
    VIDDEC_PARSE_FRMDONE = 0xF2,
} viddec_parser_error_t;

/*
 *
 *Functions used by Parsers
 *
 */

/* This function returns the requested number of bits(<=32) and increments au byte position.
 */
int32_t viddec_pm_get_bits(void *parent, uint32_t *data, uint32_t num_bits);

/* This function returns requested number of bits(<=32) with out incrementing au byte position
 */
int32_t viddec_pm_peek_bits(void *parent, uint32_t *data, uint32_t num_bits);

/* This function skips requested number of bits(<=32) by incrementing au byte position.
 */
int32_t viddec_pm_skip_bits(void *parent, uint32_t num_bits);


/* This function gets current byte and bit positions and information on whether an emulation byte is present after
current byte.
 */
int32_t viddec_pm_get_au_pos(void *parent, uint32_t *bit, uint32_t *byte, unsigned char *is_emul);

/* This function appends Pixel tag to current work load starting from current position to end of au unit.
 */
int32_t viddec_pm_append_pixeldata(void *parent);

/* This function appends Pixel tag to next work load starting from current position to end of au unit.
 */
int32_t viddec_pm_append_pixeldata_next(void *parent);


/* Returns the current byte value where offset is on */
uint32_t viddec_pm_get_cur_byte(void *parent, uint8_t *byte);

/* Tells us if there is more data that need to parse */
int32_t viddec_pm_is_nomoredata(void *parent);


void viddec_pm_set_next_frame_error_on_eos(void *parent, uint32_t error);

void viddec_pm_set_late_frame_detect(void *parent);

#endif
