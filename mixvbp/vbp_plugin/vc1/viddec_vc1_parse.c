#include <vbp_trace.h>
#include "vc1.h"                // For the parser structure
#include "viddec_parser_ops.h"  // For parser helper functions
#include "vc1parse.h"           // For vc1 parser helper functions
#include "viddec_pm.h"
#define vc1_is_frame_start_code( ch )                                   \
    (( vc1_SCField == ch ||vc1_SCSlice == ch || vc1_SCFrameHeader == ch ) ? 1 : 0)

/* init function */
void viddec_vc1_init(void *ctxt, uint32_t *persist_mem, uint32_t preserve)
{
    vc1_viddec_parser_t *parser = ctxt;
    int i;

    persist_mem = persist_mem;

    for (i=0; i<VC1_NUM_REFERENCE_FRAMES; i++)
    {
        parser->ref_frame[i].id   = -1; /* first I frame checks that value */
        parser->ref_frame[i].tff=0;
    }

    parser->is_reference_picture = false;

    memset(&parser->info.picLayerHeader, 0, sizeof(vc1_PictureLayerHeader));

    if (preserve)
    {
        parser->sc_seen &= VC1_EP_MASK;
    }
    else
    {
        parser->sc_seen = VC1_SC_INVALID;
        memset(&parser->info.metadata, 0, sizeof(parser->info.metadata));
    }

    return;
} // viddec_vc1_init

uint32_t viddec_vc1_parse(void *parent, void *ctxt)
{
    vc1_viddec_parser_t *parser = ctxt;
    uint32_t sc=0x0;
    int32_t ret=0, status=0;

    /* This works only if there is one slice and no start codes */
    /* A better fix would be to insert start codes it there aren't any. */
    ret = viddec_pm_peek_bits(parent, &sc, 32);
    if ((sc > 0x0100) && (sc < 0x0200)) /* a Start code will be in this range. */
    {
        ret = viddec_pm_get_bits(parent, &sc, 32);
    }
    else
    {
        /* In cases where we get a buffer with no start codes, we assume */
        /* that this is a frame of data. We may have to fix this later. */
        sc = vc1_SCFrameHeader;
    }
    sc = sc & 0xFF;
    VTRACE("START_CODE = %02x\n", sc);
    switch ( sc )
    {
    case vc1_SCSequenceHeader:
    {
        uint32_t data;
        memset( &parser->info.metadata, 0, sizeof(parser->info.metadata));
        /* look if we have a rcv header for main or simple profile */
        ret = viddec_pm_peek_bits(parent,&data ,2);

        if (data == 3)
        {
            status = vc1_ParseSequenceLayer(parent, &parser->info);
        }
        else
        {
            status = vc1_ParseRCVSequenceLayer(parent, &parser->info);
        }
        parser->sc_seen = VC1_SC_SEQ;
        parser->start_code = VC1_SC_SEQ;
        if (parser->info.metadata.HRD_NUM_LEAKY_BUCKETS == 0)
        {
            if (parser->info.metadata.PROFILE == VC1_PROFILE_SIMPLE)
            {
                switch(parser->info.metadata.LEVEL)
                {
                case 0:
                    parser->info.metadata.hrd_initial_state.sLeakyBucket[0].HRD_RATE = 96000;
                    break;
                case 1:
                    parser->info.metadata.hrd_initial_state.sLeakyBucket[0].HRD_RATE = 384000;
                    break;
                }
            }
            else if (parser->info.metadata.PROFILE == VC1_PROFILE_MAIN)
            {
                switch(parser->info.metadata.LEVEL)
                {
                case 0:
                    parser->info.metadata.hrd_initial_state.sLeakyBucket[0].HRD_RATE = 2000000;
                    break;
                case 1:
                    parser->info.metadata.hrd_initial_state.sLeakyBucket[0].HRD_RATE = 10000000;
                    break;
                case 2:
                    parser->info.metadata.hrd_initial_state.sLeakyBucket[0].HRD_RATE = 20000000;
                    break;
                }
            }
            else if (parser->info.metadata.PROFILE == VC1_PROFILE_ADVANCED)
            {
                switch(parser->info.metadata.LEVEL)
                {
                case 0:
                    parser->info.metadata.hrd_initial_state.sLeakyBucket[0].HRD_RATE = 2000000;
                    break;
                case 1:
                    parser->info.metadata.hrd_initial_state.sLeakyBucket[0].HRD_RATE = 10000000;
                    break;
                case 2:
                    parser->info.metadata.hrd_initial_state.sLeakyBucket[0].HRD_RATE = 20000000;
                    break;
                case 3:
                    parser->info.metadata.hrd_initial_state.sLeakyBucket[0].HRD_RATE = 45000000;
                    break;
                }
            }
        }

        break;
    }

    case vc1_SCEntryPointHeader:
    {
        status = vc1_ParseEntryPointLayer(parent, &parser->info);
        parser->sc_seen |= VC1_SC_EP;
        // Clear all bits indicating data below ep header
        parser->sc_seen &= VC1_EP_MASK;
        parser->start_code = VC1_SC_EP;
        break;
    }

    case vc1_SCFrameHeader:
    {
        memset(&parser->info.picLayerHeader, 0, sizeof(vc1_PictureLayerHeader));
        status = vc1_ParsePictureLayer(parent, &parser->info);
        parser->sc_seen |= VC1_SC_FRM;
        // Clear all bits indicating data below frm header
        parser->sc_seen &= VC1_FRM_MASK;
        parser->start_code = VC1_SC_FRM;
        break;
    }

    case vc1_SCSlice:
    {
        status = vc1_ParseSliceLayer(parent, &parser->info);

        parser->start_code = VC1_SC_SLC;
        break;
    }

    case vc1_SCField:
    {
        parser->info.picLayerHeader.SLICE_ADDR = 0;
        parser->info.picLayerHeader.CurrField = 1;
        parser->info.picLayerHeader.REFFIELD = 0;
        parser->info.picLayerHeader.NUMREF = 0;
        parser->info.picLayerHeader.MBMODETAB = 0;
        parser->info.picLayerHeader.MV4SWITCH = 0;
        parser->info.picLayerHeader.DMVRANGE = 0;
        parser->info.picLayerHeader.MVTAB = 0;
        parser->info.picLayerHeader.MVMODE = 0;
        parser->info.picLayerHeader.MVRANGE = 0;
        parser->info.picLayerHeader.raw_MVTYPEMB = 0;
        parser->info.picLayerHeader.raw_DIRECTMB = 0;
        parser->info.picLayerHeader.raw_SKIPMB = 0;
        parser->info.picLayerHeader.raw_ACPRED = 0;
        parser->info.picLayerHeader.raw_FIELDTX = 0;
        parser->info.picLayerHeader.raw_OVERFLAGS = 0;
        parser->info.picLayerHeader.raw_FORWARDMB = 0;

        memset(&(parser->info.picLayerHeader.MVTYPEMB), 0, sizeof(vc1_Bitplane));
        memset(&(parser->info.picLayerHeader.DIRECTMB), 0, sizeof(vc1_Bitplane));
        memset(&(parser->info.picLayerHeader.SKIPMB), 0, sizeof(vc1_Bitplane));
        memset(&(parser->info.picLayerHeader.ACPRED), 0, sizeof(vc1_Bitplane));
        memset(&(parser->info.picLayerHeader.FIELDTX), 0, sizeof(vc1_Bitplane));
        memset(&(parser->info.picLayerHeader.OVERFLAGS), 0, sizeof(vc1_Bitplane));
        memset(&(parser->info.picLayerHeader.FORWARDMB), 0, sizeof(vc1_Bitplane));

        parser->info.picLayerHeader.ALTPQUANT = 0;
        parser->info.picLayerHeader.DQDBEDGE = 0;

        status = vc1_ParseFieldLayer(parent, &parser->info);
        parser->sc_seen |= VC1_SC_FLD;
        parser->start_code = VC1_SC_FLD;
        break;
    }

    case vc1_SCSequenceUser:
    case vc1_SCEntryPointUser:
    case vc1_SCFrameUser:
    case vc1_SCSliceUser:
    case vc1_SCFieldUser:
    {/* Handle user data */
        parser->start_code = VC1_SC_UD;
        break;
    }

    case vc1_SCEndOfSequence:
    {
        parser->sc_seen = VC1_SC_INVALID;
        parser->start_code = VC1_SC_INVALID;
        break;
    }
    default: /* Any other SC that is not handled */
    {
        WTRACE("SC = %02x - unhandled\n", sc );
        parser->start_code = VC1_SC_INVALID;
        break;
    }
    }

    return VIDDEC_PARSE_SUCESS;
} // viddec_vc1_parse

void viddec_vc1_get_context_size(viddec_parser_memory_sizes_t *size)
{
    size->context_size = sizeof(vc1_viddec_parser_t);
    size->persist_size = 0;
    return;
} // viddec_vc1_get_context_size
