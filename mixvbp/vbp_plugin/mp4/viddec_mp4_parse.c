#include <string.h>
#include <vbp_common.h>
#include <vbp_trace.h>

#include "viddec_parser_ops.h"
#include "viddec_mp4_parse.h"
#include "viddec_mp4_decodevideoobjectplane.h"
#include "viddec_mp4_shortheader.h"
#include "viddec_mp4_videoobjectlayer.h"
#include "viddec_mp4_videoobjectplane.h"
#include "viddec_mp4_visualobject.h"

void viddec_mp4_get_context_size(viddec_parser_memory_sizes_t *size)
{
    /* Should return size of my structure */
    size->context_size = sizeof(viddec_mp4_parser_t);
    size->persist_size = 0;
    return;
} // viddec_mp4_get_context_size

void viddec_mp4_init(void *ctxt, uint32_t *persist_mem, uint32_t preserve)
{
    viddec_mp4_parser_t *parser = (viddec_mp4_parser_t *) ctxt;

    persist_mem = persist_mem;
    parser->is_frame_start = false;
    parser->prev_sc = MP4_SC_INVALID;
    parser->current_sc = MP4_SC_INVALID;
    parser->cur_sc_prefix = false;
    parser->next_sc_prefix = false;
    parser->ignore_scs = false;

    if (!preserve)
    {
        parser->sc_seen = MP4_SC_SEEN_INVALID;
        parser->bitstream_error = MP4_BS_ERROR_NONE;
        memset(&(parser->info), 0, sizeof(mp4_Info_t));
    }
    else
    {
        // Need to maintain information till VOL
        parser->sc_seen &= MP4_SC_SEEN_VOL;
        parser->bitstream_error &= MP4_HDR_ERROR_MASK;

        // Reset only frame related data
        memset(&(parser->info.VisualObject.VideoObject.VideoObjectPlane), 0, sizeof(mp4_VideoObjectPlane_t));
        memset(&(parser->info.VisualObject.VideoObject.VideoObjectPlaneH263), 0, sizeof(mp4_VideoObjectPlaneH263));
    }

    return;
} // viddec_mp4_init

static uint32_t viddec_mp4_decodevop_and_emitwkld(void *parent, void *ctxt)
{
    int status = MP4_STATUS_OK;
    viddec_mp4_parser_t *cxt = (viddec_mp4_parser_t *)ctxt;

    status = mp4_DecodeVideoObjectPlane(&(cxt->info));

    return status;
} // viddec_mp4_decodevop_and_emitwkld

uint32_t viddec_mp4_parse(void *parent, void *ctxt)
{
    uint32_t sc=0;
    viddec_mp4_parser_t *cxt;
    uint8_t is_svh=0;
    int32_t getbits=0;
    int32_t status = 0;

    cxt = (viddec_mp4_parser_t *)ctxt;
    is_svh = (cxt->cur_sc_prefix) ? false: true;

    if (!is_svh)
    {
        if (viddec_pm_get_bits(parent, &sc, 32) != -1)
        {
            sc = sc & 0xFF;
            cxt->current_sc = sc;
            cxt->current_sc |= 0x100;
            VTRACE("current_sc=0x%.8X, prev_sc=0x%x\n", sc, cxt->prev_sc);

            switch (sc)
            {
            case MP4_SC_VISUAL_OBJECT_SEQUENCE:
            {
                status = mp4_Parse_VisualSequence(parent, cxt);
                cxt->prev_sc = MP4_SC_VISUAL_OBJECT_SEQUENCE;
                VTRACE("MP4_VISUAL_OBJECT_SEQUENCE_SC: \n");
                break;
            }
            case MP4_SC_VISUAL_OBJECT_SEQUENCE_EC:
            {/* Not required to do anything */
                VTRACE("MP4_SC_VISUAL_OBJECT_SEQUENCE_EC");
                break;
            }
            case MP4_SC_USER_DATA:
            {   /* Copy userdata to user-visible buffer (EMIT) */
                VTRACE("MP4_USER_DATA_SC: \n");
                break;
            }
            case MP4_SC_GROUP_OF_VOP:
            {
                status = mp4_Parse_GroupOfVideoObjectPlane(parent, cxt);
                cxt->prev_sc = MP4_SC_GROUP_OF_VOP;
                VTRACE("MP4_GROUP_OF_VOP_SC:0x%.8X\n", status);
                break;
            }
            case MP4_SC_VIDEO_SESSION_ERROR:
            {/* Not required to do anything?? */
                VTRACE("MP4_SC_VIDEO_SESSION_ERROR");
                break;
            }
            case MP4_SC_VISUAL_OBJECT:
            {
                status = mp4_Parse_VisualObject(parent, cxt);
                cxt->prev_sc = MP4_SC_VISUAL_OBJECT;
                VTRACE("MP4_VISUAL_OBJECT_SC: status=%.8X\n", status);
                break;
            }
            case MP4_SC_VIDEO_OBJECT_PLANE:
            {
                /* We must decode the VOP Header information, it does not end  on a byte boundary, so we need to emit
                   a starting bit offset after parsing the header. */
                status = mp4_Parse_VideoObjectPlane(parent, cxt);
                status = viddec_mp4_decodevop_and_emitwkld(parent, cxt);
                // TODO: Fix this for interlaced
                cxt->is_frame_start = true;
                cxt->sc_seen |= MP4_SC_SEEN_VOP;

                VTRACE("MP4_VIDEO_OBJECT_PLANE_SC: status=0x%.8X\n", status);
                break;
            }
            case MP4_SC_STUFFING:
            {
                VTRACE("MP4_SC_STUFFING");
                break;
            }
            default:
            {
                if ( (sc >=  MP4_SC_VIDEO_OBJECT_LAYER_MIN) && (sc <=  MP4_SC_VIDEO_OBJECT_LAYER_MAX) )
                {
                    status = mp4_Parse_VideoObjectLayer(parent, cxt);
                    cxt->sc_seen = MP4_SC_SEEN_VOL;
                    cxt->prev_sc = MP4_SC_VIDEO_OBJECT_LAYER_MIN;
                    VTRACE("MP4_VIDEO_OBJECT_LAYER_MIN_SC:status=0x%.8X\n", status);
                    sc = MP4_SC_VIDEO_OBJECT_LAYER_MIN;
                }
                // sc is unsigned and will be >= 0, so no check needed for sc >= MP4_SC_VIDEO_OBJECT_MIN
                else if (sc <= MP4_SC_VIDEO_OBJECT_MAX)
                {
                    // If there is more data, it is short video header, else the next start code is expected to be VideoObjectLayer
                    getbits = viddec_pm_get_bits(parent, &sc, 22);
                    if (getbits != -1)
                    {
                        cxt->current_sc = sc;
                        status = mp4_Parse_VideoObject_svh(parent, cxt);
                        status = viddec_mp4_decodevop_and_emitwkld(parent, cxt);
                        cxt->sc_seen = MP4_SC_SEEN_SVH;
                        cxt->is_frame_start = true;
                        VTRACE("MP4_SCS_SVH: status=0x%.8X 0x%.8X %.8X\n", status, cxt->current_sc, sc);
                    }
                }
                else
                {
                    ETRACE("UNKWON Cod:0x%08X\n", sc);
                }
            }
            break;
            }
        }
        else
        {
            ETRACE("Start code not found\n");
            return VIDDEC_PARSE_ERROR;
        }
    }
    else
    {
        if (viddec_pm_get_bits(parent, &sc, 22) != -1)
        {
            cxt->current_sc = sc;
            VTRACE("current_sc=0x%.8X, prev_sc=0x%x\n", sc, cxt->prev_sc);
            status = mp4_Parse_VideoObject_svh(parent, cxt);
            status = viddec_mp4_decodevop_and_emitwkld(parent, cxt);
            cxt->sc_seen = MP4_SC_SEEN_SVH;
            cxt->is_frame_start = true;
            VTRACE("SVH: MP4_SCS_SVH: status=0x%.8X 0x%.8X %.8X\n", status, cxt->current_sc, sc);
        }
        else
        {
            ETRACE("Start code not found\n");
            return VIDDEC_PARSE_ERROR;
        }
    }

    // Current sc becomes the previous sc
    cxt->prev_sc = sc;

    return VIDDEC_PARSE_SUCESS;
} // viddec_mp4_parse

