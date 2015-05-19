#include "viddec_parser_ops.h"

#include "viddec_pm.h"

#include "h264.h"
#include "h264parse.h"

#include "h264parse_dpb.h"
#include <vbp_trace.h>
#include <assert.h>

uint32_t viddec_threading_backup_ctx_info(void *parent, h264_Slice_Header_t *next_SliceHeader);
uint32_t viddec_threading_restore_ctx_info(void *parent, h264_Slice_Header_t *next_SliceHeader);

#define MAX_SLICE_HEADER 150

/* Init function which can be called to intialized local context on open and flush and preserve*/
void viddec_h264_init(void *ctxt, uint32_t *persist_mem, uint32_t preserve)
{
    struct h264_viddec_parser* parser = ctxt;
    h264_Info * pInfo = &(parser->info);

    if (!preserve)
    {
        /* we don't initialize this data if we want to preserve
                 sequence and gop information.
              */
        h264_init_sps_pps(parser,persist_mem);
    }
    /* picture level info which will always be initialized */
    h264_init_Info_under_sps_pps_level(pInfo);

    uint32_t i;
    for(i = 0; i < MAX_SLICE_HEADER; i++) {
        pInfo->working_sh[i] = (h264_Slice_Header_t*)malloc(sizeof(h264_Slice_Header_t));
        assert(pInfo->working_sh[i] != NULL);

        pInfo->working_sh[i]->parse_done = 0;
        pInfo->working_sh[i]->bstrm_buf_buf_index = 0;
        pInfo->working_sh[i]->bstrm_buf_buf_st = 0;
        pInfo->working_sh[i]->bstrm_buf_buf_end = 0;
        pInfo->working_sh[i]->bstrm_buf_buf_bitoff = 0;
        pInfo->working_sh[i]->au_pos = 0;
        pInfo->working_sh[i]->list_off = 0;
        pInfo->working_sh[i]->phase = 0;
        pInfo->working_sh[i]->emulation_byte_counter = 0;
        pInfo->working_sh[i]->is_emul_reqd = 0;
        pInfo->working_sh[i]->list_start_offset = 0;
        pInfo->working_sh[i]->list_end_offset = 0;
        pInfo->working_sh[i]->list_total_bytes = 0;
        pInfo->working_sh[i]->slice_group_change_cycle = 0;
    }
    return;
}


/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
uint32_t viddec_h264_parse(void *parent, void *ctxt)
{
    struct h264_viddec_parser* parser = ctxt;

    h264_Info * pInfo = &(parser->info);

    h264_Status status = H264_STATUS_ERROR;

    uint8_t nal_ref_idc = 0;
    uint8_t nal_unit_type = 0;

    ///// Parse NAL Unit header
    pInfo->img.g_new_frame = 0;
    pInfo->push_to_cur = 1;
    pInfo->is_current_workload_done =0;
    pInfo->nal_unit_type = 0;

    h264_Parse_NAL_Unit(parent, &nal_unit_type, &nal_ref_idc);
    VTRACE("Start parsing NAL unit, type = %d", pInfo->nal_unit_type);

    pInfo->nal_unit_type = nal_unit_type;
    ///// Check frame bounday for non-vcl elimitter
    h264_check_previous_frame_end(pInfo);

    //////// Parse valid NAL unit
    switch (pInfo->nal_unit_type)
    {
    case h264_NAL_UNIT_TYPE_IDR:
        if (pInfo->got_start)
            pInfo->img.recovery_point_found |= 1;

        pInfo->sei_rp_received = 0;

    case h264_NAL_UNIT_TYPE_SLICE:
        ////////////////////////////////////////////////////////////////////////////
        // Step 1: Check start point
        ////////////////////////////////////////////////////////////////////////////
        //
        /// Slice parsing must start from the valid start point( SPS, PPS,  IDR or recovery point or primary_I)
        /// 1) No start point reached, append current ES buffer to workload and release it
        /// 2) else, start parsing
        //
        //if(pInfo->got_start && ((pInfo->sei_information.recovery_point) || (pInfo->nal_unit_type == h264_NAL_UNIT_TYPE_IDR)))
        //{
        //pInfo->img.recovery_point_found = 1;
        //}
    {

        h264_Slice_Header_t next_SliceHeader;

        /// Reset next slice header
        h264_memset(&next_SliceHeader, 0x0, sizeof(h264_Slice_Header_t));
        next_SliceHeader.nal_ref_idc = nal_ref_idc;

        if ((1 == pInfo->primary_pic_type_plus_one) && (pInfo->got_start))
        {
            pInfo->img.recovery_point_found |= 4;
        }
        pInfo->primary_pic_type_plus_one = 0;


        ////////////////////////////////////////////////////////////////////////////
        // Step 2: Parsing slice header
        ////////////////////////////////////////////////////////////////////////////
        /// PWT
        pInfo->h264_pwt_start_byte_offset = 0;
        pInfo->h264_pwt_start_bit_offset = 0;
        pInfo->h264_pwt_end_byte_offset = 0;
        pInfo->h264_pwt_end_bit_offset = 0;
        pInfo->h264_pwt_enabled = 0;
        /// IDR flag
        next_SliceHeader.idr_flag = (pInfo->nal_unit_type == h264_NAL_UNIT_TYPE_IDR);

        /// Pass slice header
        status = h264_Parse_Slice_Layer_Without_Partitioning_RBSP(parent, pInfo, &next_SliceHeader);

        pInfo->sei_information.recovery_point = 0;

        if (next_SliceHeader.sh_error & 3)
        {
            ETRACE("Slice Header parsing error.\n");
            break;
        }
        pInfo->img.current_slice_num++;


        ////////////////////////////////////////////////////////////////////////////
        // Step 3: Processing if new picture coming
        //  1) if it's the second field
        //	2) if it's a new frame
        ////////////////////////////////////////////////////////////////////////////
        //AssignQuantParam(pInfo);
        if (h264_is_new_picture_start(pInfo, next_SliceHeader, pInfo->SliceHeader))
        {
            //
            ///----------------- New Picture.boundary detected--------------------
            //
            pInfo->img.g_new_pic++;

            //
            // Complete previous picture
            h264_dpb_store_previous_picture_in_dpb(pInfo, 0, 0); //curr old
            //h264_hdr_post_poc(0, 0, use_old);

            //
            // Update slice structures:
            h264_update_old_slice(pInfo, next_SliceHeader);  	//cur->old; next->cur;

            //
            // 1) if resolution change: reset dpb
            // 2) else: init frame store
            h264_update_img_info(pInfo);								//img, dpb

            //
            ///----------------- New frame.boundary detected--------------------
            //
            pInfo->img.second_field = h264_is_second_field(pInfo);
            if (pInfo->img.second_field == 0)
            {
                pInfo->img.g_new_frame = 1;
                h264_dpb_update_queue_dangling_field(pInfo);

                //
                /// DPB management
                ///	1) check the gaps
                ///	2) assign fs for non-exist frames
                ///	3) fill the gaps
                ///	4) store frame into DPB if ...
                //
                //if(pInfo->SliceHeader.redundant_pic_cnt)
                {
                    h264_dpb_gaps_in_frame_num_mem_management(pInfo);
                }
            }
            //
            /// Decoding POC
            h264_hdr_decoding_poc (pInfo, 0, 0);

            //
            /// Init Frame Store for next frame
            h264_dpb_init_frame_store (pInfo);
            pInfo->img.current_slice_num = 1;

            if (pInfo->SliceHeader.first_mb_in_slice != 0)
            {
                ////Come here means we have slice lost at the beginning, since no FMO support
                pInfo->SliceHeader.sh_error |= (pInfo->SliceHeader.structure << 17);
            }

            /// Emit out the New Frame
            if (pInfo->img.g_new_frame)
            {
                h264_parse_emit_start_new_frame(parent, pInfo);
            }

            h264_parse_emit_current_pic(parent, pInfo);
        }
        else ///////////////////////////////////////////////////// If Not a picture start
        {
            //
            /// Update slice structures: cur->old; next->cur;
            h264_update_old_slice(pInfo, next_SliceHeader);

            //
            /// 1) if resolution change: reset dpb
            /// 2) else: update img info
            h264_update_img_info(pInfo);
        }


        //////////////////////////////////////////////////////////////
        // Step 4: DPB reference list init and reordering
        //////////////////////////////////////////////////////////////

        //////////////////////////////////////////////// Update frame Type--- IDR/I/P/B for frame or field
        h264_update_frame_type(pInfo);

#ifndef USE_AVC_SHORT_FORMAT
        h264_dpb_update_ref_lists(pInfo);
#endif
        /// Emit out the current "good" slice
        h264_parse_emit_current_slice(parent, pInfo);

    }
    break;

    ///// * Main profile doesn't support Data Partition, skipped.... *////
    case h264_NAL_UNIT_TYPE_DPA:
    case h264_NAL_UNIT_TYPE_DPB:
    case h264_NAL_UNIT_TYPE_DPC:
        ETRACE("Data Partition is not supported currently\n");
        status = H264_STATUS_NOTSUPPORT;
        break;

        //// * Parsing SEI info *////
    case h264_NAL_UNIT_TYPE_SEI:
        status = H264_STATUS_OK;

        //OS_INFO("*****************************SEI**************************************\n");
        if (pInfo->sps_valid)
        {
            //h264_user_data_t user_data; /// Replace with tmp buffer while porting to FW
            pInfo->number_of_first_au_info_nal_before_first_slice++;
            /// parsing the SEI info
            status = h264_Parse_Supplemental_Enhancement_Information_Message(parent, pInfo);
        }

        //h264_rbsp_trailing_bits(pInfo);
        break;
    case h264_NAL_UNIT_TYPE_SPS:
    {
        //OS_INFO("*****************************SPS**************************************\n");
        ///
        /// Can not define local SPS since the Current local stack size limitation!
        /// Could be changed after the limitation gone
        ///
        uint8_t  old_sps_id=0;
        vui_seq_parameters_t_not_used vui_seq_not_used;

        old_sps_id = pInfo->active_SPS.seq_parameter_set_id;
        h264_memset(&(pInfo->active_SPS), 0x0, sizeof(seq_param_set_used));


        status = h264_Parse_SeqParameterSet(parent, pInfo, &(pInfo->active_SPS), &vui_seq_not_used, (int32_t *)pInfo->TMP_OFFSET_REFFRM_PADDR_GL);
        if (status == H264_STATUS_OK)
        {
            h264_Parse_Copy_Sps_To_DDR(pInfo, &(pInfo->active_SPS), pInfo->active_SPS.seq_parameter_set_id);
            pInfo->sps_valid = 1;

            if (1 == pInfo->active_SPS.pic_order_cnt_type)
            {
                h264_Parse_Copy_Offset_Ref_Frames_To_DDR(pInfo,(int32_t *)pInfo->TMP_OFFSET_REFFRM_PADDR_GL,pInfo->active_SPS.seq_parameter_set_id);
            }
        }
        ///// Restore the active SPS if new arrival's id changed
        if (old_sps_id >= MAX_NUM_SPS) {
            h264_memset(&(pInfo->active_SPS), 0x0, sizeof(seq_param_set_used));
            pInfo->active_SPS.seq_parameter_set_id = 0xff;
        }
        else {
            if (old_sps_id != pInfo->active_SPS.seq_parameter_set_id)  {
                h264_Parse_Copy_Sps_From_DDR(pInfo, &(pInfo->active_SPS), old_sps_id);
            }
            else  {
                //h264_memset(&(pInfo->active_SPS), 0x0, sizeof(seq_param_set));
                pInfo->active_SPS.seq_parameter_set_id = 0xff;
            }
        }

        pInfo->number_of_first_au_info_nal_before_first_slice++;
    }
    break;
    case h264_NAL_UNIT_TYPE_PPS:
    {
        //OS_INFO("*****************************PPS**************************************\n");
        status = H264_STATUS_OK;
        uint32_t old_sps_id = pInfo->active_SPS.seq_parameter_set_id;
        uint32_t old_pps_id = pInfo->active_PPS.pic_parameter_set_id;

        h264_memset(&pInfo->active_PPS, 0x0, sizeof(pic_param_set));
        pInfo->number_of_first_au_info_nal_before_first_slice++;

        if (h264_Parse_PicParameterSet(parent, pInfo, &pInfo->active_PPS)== H264_STATUS_OK)
        {
            h264_Parse_Copy_Sps_From_DDR(pInfo, &(pInfo->active_SPS), pInfo->active_PPS.seq_parameter_set_id);
            if (old_sps_id != pInfo->active_SPS.seq_parameter_set_id)
            {
                pInfo->Is_SPS_updated = 1;
            }
            if (pInfo->active_SPS.seq_parameter_set_id != 0xff) {
                h264_Parse_Copy_Pps_To_DDR(pInfo, &pInfo->active_PPS, pInfo->active_PPS.pic_parameter_set_id);
                pInfo->got_start = 1;
                if (pInfo->sei_information.recovery_point)
                {
                    pInfo->img.recovery_point_found |= 2;

                    //// Enable the RP recovery if no IDR ---Cisco
                    if ((pInfo->img.recovery_point_found & 1)==0)
                        pInfo->sei_rp_received = 1;
                }
            }
            else
            {
                h264_Parse_Copy_Sps_From_DDR(pInfo, &(pInfo->active_SPS), old_sps_id);
            }
        }
        else
        {
            if (old_sps_id < MAX_NUM_SPS)
                h264_Parse_Copy_Sps_From_DDR(pInfo, &(pInfo->active_SPS), old_sps_id);
            if (old_pps_id < MAX_NUM_PPS)
                h264_Parse_Copy_Pps_From_DDR(pInfo, &(pInfo->active_PPS), old_pps_id);
        }

    } //// End of PPS parsing
    break;


    case h264_NAL_UNIT_TYPE_EOSeq:
    case h264_NAL_UNIT_TYPE_EOstream:

        h264_parse_emit_eos(parent, pInfo);
        h264_init_dpb(&(pInfo->dpb));

        pInfo->is_current_workload_done=1;

        /* picture level info which will always be initialized */
        //h264_init_Info_under_sps_pps_level(pInfo);

        ////reset the pInfo here
        //viddec_h264_init(ctxt, (uint32_t *)parser->sps_pps_ddr_paddr, false);


        status = H264_STATUS_OK;
        pInfo->number_of_first_au_info_nal_before_first_slice++;
        break;

    case h264_NAL_UNIT_TYPE_Acc_unit_delimiter:
        ///// primary_pic_type
        {
            uint32_t code = 0xff;
            int32_t ret = 0;
            ret = viddec_pm_get_bits(parent, (uint32_t *)&(code), 3);

            if (ret != -1) {
                //if(pInfo->got_start && (code == 0))
                //{
                //pInfo->img.recovery_point_found |= 4;
                //}
                pInfo->primary_pic_type_plus_one = (uint8_t)(code)+1;
                status = H264_STATUS_OK;
            }
            pInfo->number_of_first_au_info_nal_before_first_slice++;
            break;
        }

    case h264_NAL_UNIT_TYPE_Reserved1:
    case h264_NAL_UNIT_TYPE_Reserved2:
    case h264_NAL_UNIT_TYPE_Reserved3:
    case h264_NAL_UNIT_TYPE_Reserved4:
    case h264_NAL_UNIT_TYPE_Reserved5:
        status = H264_STATUS_OK;
        pInfo->number_of_first_au_info_nal_before_first_slice++;
        break;

    case h264_NAL_UNIT_TYPE_filler_data:
        status = H264_STATUS_OK;
        break;
    case h264_NAL_UNIT_TYPE_ACP:
        break;
    case h264_NAL_UNIT_TYPE_SPS_extension:
    case h264_NAL_UNIT_TYPE_unspecified:
    case h264_NAL_UNIT_TYPE_unspecified2:
        status = H264_STATUS_OK;
        //nothing
        break;
    default:
        status = H264_STATUS_OK;
        break;
    }

    //pInfo->old_nal_unit_type = pInfo->nal_unit_type;
    switch ( pInfo->nal_unit_type )
    {
    case h264_NAL_UNIT_TYPE_IDR:
    case h264_NAL_UNIT_TYPE_SLICE:
    case h264_NAL_UNIT_TYPE_Acc_unit_delimiter:
    case h264_NAL_UNIT_TYPE_SPS:
    case h264_NAL_UNIT_TYPE_PPS:
    case h264_NAL_UNIT_TYPE_SEI:
    case h264_NAL_UNIT_TYPE_EOSeq:
    case h264_NAL_UNIT_TYPE_EOstream:
    case h264_NAL_UNIT_TYPE_Reserved1:
    case h264_NAL_UNIT_TYPE_Reserved2:
    case h264_NAL_UNIT_TYPE_Reserved3:
    case h264_NAL_UNIT_TYPE_Reserved4:
    case h264_NAL_UNIT_TYPE_Reserved5:
    {
        pInfo->old_nal_unit_type = pInfo->nal_unit_type;
        break;
    }
    default:
        break;
    }

    return status;
}


uint32_t viddec_h264_threading_parse(void *parent, void *ctxt, uint32_t slice_index)
{
    struct h264_viddec_parser* parser = ctxt;

    h264_Info * pInfo = &(parser->info);

    h264_Status status = H264_STATUS_ERROR;

    uint8_t nal_ref_idc = 0;
    uint8_t nal_unit_type = 0;

    h264_Parse_NAL_Unit(parent, &nal_unit_type, &nal_ref_idc);

    pInfo->nal_unit_type = nal_unit_type;


    //////// Parse valid NAL unit
    if (nal_unit_type == h264_NAL_UNIT_TYPE_SLICE) {
        h264_Slice_Header_t* next_SliceHeader = pInfo->working_sh[slice_index];
        memset(next_SliceHeader, 0, sizeof(h264_Slice_Header_t));

        next_SliceHeader->nal_ref_idc = nal_ref_idc;


        ////////////////////////////////////////////////////////////////////////////
        // Step 2: Parsing slice header
        ////////////////////////////////////////////////////////////////////////////
        /// IDR flag
        next_SliceHeader->idr_flag = (pInfo->nal_unit_type == h264_NAL_UNIT_TYPE_IDR);


        /// Pass slice header
        status = h264_Parse_Slice_Layer_Without_Partitioning_RBSP_opt(parent, pInfo, next_SliceHeader);

        viddec_threading_backup_ctx_info(parent, next_SliceHeader);

        if (next_SliceHeader->sh_error & 3)
        {
            ETRACE("Slice Header parsing error.");
            status = H264_STATUS_ERROR;
            return status;
        }

        //h264_Post_Parsing_Slice_Header(parent, pInfo, &next_SliceHeader);
        next_SliceHeader->parse_done  = 1;

    } else {
        ETRACE("Wrong NALU. Multi thread is supposed to just parse slice nalu type.");
        status = H264_STATUS_ERROR;
        return status;
    }

   return status;
}



void viddec_h264_get_context_size(viddec_parser_memory_sizes_t *size)
{
    /* Should return size of my structure */
    size->context_size = sizeof(struct h264_viddec_parser);
    size->persist_size = MAX_NUM_SPS * sizeof(seq_param_set_all)
                         + MAX_NUM_PPS * sizeof(pic_param_set)
                         + MAX_NUM_SPS * sizeof(int32_t) * MAX_NUM_REF_FRAMES_IN_PIC_ORDER_CNT_CYCLE
                         + sizeof(int32_t) * MAX_NUM_REF_FRAMES_IN_PIC_ORDER_CNT_CYCLE;
}

/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
void viddec_h264_flush(void *parent, void *ctxt)
{
    int i;
    struct h264_viddec_parser* parser = ctxt;
    h264_Info * pInfo = &(parser->info);

    /* just flush dpb and disable output */
    h264_dpb_flush_dpb(pInfo, 0, pInfo->img.second_field, pInfo->active_SPS.num_ref_frames);

    /* reset the dpb to the initial state, avoid parser store
       wrong data to dpb in next slice parsing */
    h264_DecodedPictureBuffer *p_dpb = &pInfo->dpb;
    for (i = 0; i < NUM_DPB_FRAME_STORES; i++)
    {
        p_dpb->fs[i].fs_idc = MPD_DPB_FS_NULL_IDC;
        p_dpb->fs_dpb_idc[i] = MPD_DPB_FS_NULL_IDC;
    }
    p_dpb->used_size = 0;
    p_dpb->fs_dec_idc = MPD_DPB_FS_NULL_IDC;
    p_dpb->fs_non_exist_idc = MPD_DPB_FS_NULL_IDC;

    for(i = 0; i < MAX_SLICE_HEADER; i++) {
        free(pInfo->working_sh[i]);
        pInfo->working_sh[i] = NULL;
    }
    return;
}

uint32_t viddec_h264_payload_start(void *parent)
{

    uint32_t code;
    uint8_t nal_unit_type = 0;
    if ( viddec_pm_peek_bits(parent, &code, 8) != -1)
    {
        nal_unit_type = (uint8_t)((code >> 0) & 0x1f);
    }
    //check that whether slice data starts
    if (nal_unit_type == h264_NAL_UNIT_TYPE_SLICE)
    {
        return 1;
    } else {
        return 0;
    }
}

uint32_t viddec_h264_post_parse(void *parent, void *ctxt, uint32_t slice_index)
{
    struct h264_viddec_parser* parser = ctxt;
    h264_Info * pInfo = &(parser->info);
    h264_Status status = H264_STATUS_ERROR;

    h264_Slice_Header_t* next_SliceHeader = pInfo->working_sh[slice_index];

    while (next_SliceHeader->parse_done != 1) {
        sleep(0);
        //WTRACE("slice header[%d] parse not finish, block to wait.", slice_index);
    }

    viddec_threading_restore_ctx_info(parent, next_SliceHeader);
    status = h264_Post_Parsing_Slice_Header(parent, pInfo, next_SliceHeader);

    next_SliceHeader->parse_done = 0;

    return status;
}


uint32_t viddec_h264_query_thread_parsing_cap(void)
{
    // current implementation of h.264 is capable to enable multi-thread parsing
    return 1;
}

uint32_t viddec_threading_backup_ctx_info(void *parent, h264_Slice_Header_t *next_SliceHeader)
{
    h264_Status retStatus = H264_STATUS_OK;

    viddec_pm_cxt_t* pm_cxt = (viddec_pm_cxt_t*) parent;

    next_SliceHeader->bstrm_buf_buf_index = pm_cxt->getbits.bstrm_buf.buf_index;
    next_SliceHeader->bstrm_buf_buf_st = pm_cxt->getbits.bstrm_buf.buf_st;
    next_SliceHeader->bstrm_buf_buf_end = pm_cxt->getbits.bstrm_buf.buf_end;
    next_SliceHeader->bstrm_buf_buf_bitoff = pm_cxt->getbits.bstrm_buf.buf_bitoff;

    next_SliceHeader->au_pos = pm_cxt->getbits.au_pos;
    next_SliceHeader->list_off = pm_cxt->getbits.list_off;
    next_SliceHeader->phase = pm_cxt->getbits.phase;
    next_SliceHeader->emulation_byte_counter = pm_cxt->getbits.emulation_byte_counter;
    next_SliceHeader->is_emul_reqd = pm_cxt->getbits.is_emul_reqd;

    next_SliceHeader->list_start_offset = pm_cxt->list.start_offset;
    next_SliceHeader->list_end_offset = pm_cxt->list.end_offset;
    next_SliceHeader->list_total_bytes = pm_cxt->list.total_bytes;

    return retStatus;
}

uint32_t viddec_threading_restore_ctx_info(void *parent, h264_Slice_Header_t *next_SliceHeader)
{
    h264_Status retStatus = H264_STATUS_OK;

    viddec_pm_cxt_t* pm_cxt = (viddec_pm_cxt_t*) parent;

    pm_cxt->getbits.bstrm_buf.buf_index = next_SliceHeader->bstrm_buf_buf_index;
    pm_cxt->getbits.bstrm_buf.buf_st = next_SliceHeader->bstrm_buf_buf_st;
    pm_cxt->getbits.bstrm_buf.buf_end = next_SliceHeader->bstrm_buf_buf_end;
    pm_cxt->getbits.bstrm_buf.buf_bitoff = next_SliceHeader->bstrm_buf_buf_bitoff;

    pm_cxt->getbits.au_pos = next_SliceHeader->au_pos;
    pm_cxt->getbits.list_off = next_SliceHeader->list_off;
    pm_cxt->getbits.phase = next_SliceHeader->phase;
    pm_cxt->getbits.emulation_byte_counter = next_SliceHeader->emulation_byte_counter;
    pm_cxt->getbits.is_emul_reqd = next_SliceHeader->is_emul_reqd;

    pm_cxt->list.start_offset = next_SliceHeader->list_start_offset;
    pm_cxt->list.end_offset = next_SliceHeader->list_end_offset;
    pm_cxt->list.total_bytes = next_SliceHeader->list_total_bytes;

    return retStatus;
}

