#include "viddec_parser_ops.h"

#include "viddec_pm.h"

#include "h264.h"
#include "h264parse.h"
#include "h264parse.h"
#include "h264parse_dpb.h"
#include <vbp_trace.h>

typedef struct _ParsedSliceHeaderH264Secure
{
    unsigned int size;

    unsigned char nal_ref_idc;
    unsigned char nal_unit_type;
    unsigned char slice_type;
    unsigned char redundant_pic_cnt;

    unsigned short first_mb_in_slice;
    char slice_qp_delta;
    char slice_qs_delta;

    unsigned char luma_log2_weight_denom;
    unsigned char chroma_log2_weight_denom;
    unsigned char cabac_init_idc;
    unsigned char pic_order_cnt_lsb;

    unsigned char pic_parameter_set_id;
    unsigned short idr_pic_id;
    unsigned char colour_plane_id;

    char slice_alpha_c0_offset_div2;
    char slice_beta_offset_div2;
    unsigned char slice_group_change_cycle;
    unsigned char disable_deblocking_filter_idc;

    unsigned int frame_num;
    int delta_pic_order_cnt_bottom;
    int delta_pic_order_cnt[2];

    unsigned char num_reorder_cmds[2];
    unsigned char num_ref_active_minus1[2];

    unsigned int weights_present[2][2];

    unsigned short num_mem_man_ops;

    union {
        struct {
            unsigned field_pic_flag                     : 1;
            unsigned bottom_field_flag                  : 1;
            unsigned num_ref_idx_active_override_flag   : 1;
            unsigned direct_spatial_mv_pred_flag        : 1;
            unsigned no_output_of_prior_pics_flag       : 1;
            unsigned long_term_reference_flag           : 1;
            unsigned idr_flag                           : 1;
            unsigned anchor_pic_flag                    : 1;
            unsigned inter_view_flag                    : 1;
        } bits;

        unsigned short value;
    } flags;
    unsigned short view_id;
    unsigned char priority_id;
    unsigned char temporal_id;
} ParsedSliceHeaderH264Secure;


typedef struct _vbp_h264_sliceheader {
    uint32_t sliceHeaderKey;
    ParsedSliceHeaderH264Secure parsedSliceHeader;
    uint32_t *reorder_cmd;
    int16_t *weight;
    uint32_t *pic_marking;
} vbp_h264_sliceheader;


/* Init function which can be called to intialized local context on open and flush and preserve*/
void viddec_h264secure_init(void *ctxt, uint32_t *persist_mem, uint32_t preserve)
{
    struct h264_viddec_parser* parser = ctxt;
    h264_Info * pInfo = &(parser->info);

    if (!preserve)
    {
        /* we don't initialize this data if we want to preserve
           sequence and gop information */
        h264_init_sps_pps(parser,persist_mem);
    }
    /* picture level info which will always be initialized */
    h264_init_Info_under_sps_pps_level(pInfo);

    return;
}


/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
uint32_t viddec_h264secure_parse(void *parent, void *ctxt)
{
    struct h264_viddec_parser* parser = ctxt;

    h264_Info * pInfo = &(parser->info);

    h264_Status status = H264_STATUS_ERROR;


    uint8_t nal_ref_idc = 0;

    ///// Parse NAL Unit header
    pInfo->img.g_new_frame = 0;
    pInfo->push_to_cur = 1;
    pInfo->is_current_workload_done =0;
    pInfo->nal_unit_type = 0;

    uint8_t nal_unit_type = 0;

    h264_Parse_NAL_Unit(parent, &nal_unit_type, &nal_ref_idc);
    pInfo->nal_unit_type = nal_unit_type;

    ///// Check frame bounday for non-vcl elimitter
    h264_check_previous_frame_end(pInfo);

    pInfo->has_slice = 0;

    //////// Parse valid NAL unit
    switch ( pInfo->nal_unit_type )
    {
    case h264_NAL_UNIT_TYPE_IDR:
        if (pInfo->got_start) {
            pInfo->img.recovery_point_found |= 1;
        }

        pInfo->sei_rp_received = 0;

    case h264_NAL_UNIT_TYPE_SLICE:
        {
            pInfo->has_slice = 1;
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
            //  2) if it's a new frame
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
                h264_update_old_slice(pInfo, next_SliceHeader);     //cur->old; next->cur;

                //
                // 1) if resolution change: reset dpb
                // 2) else: init frame store
                h264_update_img_info(pInfo);                                //img, dpb

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
                    /// 1) check the gaps
                    /// 2) assign fs for non-exist frames
                    /// 3) fill the gaps
                    /// 4) store frame into DPB if ...
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

            h264_dpb_update_ref_lists(pInfo);
            /// Emit out the current "good" slice
            h264_parse_emit_current_slice(parent, pInfo);
        }
        break;

    ///// * Main profile doesn't support Data Partition, skipped.... *////
    case h264_NAL_UNIT_TYPE_DPA:
    case h264_NAL_UNIT_TYPE_DPB:
    case h264_NAL_UNIT_TYPE_DPC:
        //OS_INFO("***********************DP feature, not supported currently*******************\n");
        status = H264_STATUS_NOTSUPPORT;
        break;

        //// * Parsing SEI info *////
    case h264_NAL_UNIT_TYPE_SEI:
        status = H264_STATUS_OK;

        //OS_INFO("*****************************SEI**************************************\n");
        if (pInfo->sps_valid) {
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
        VTRACE("h264_NAL_UNIT_TYPE_SPS +++");
        uint8_t  old_sps_id=0;
        vui_seq_parameters_t_not_used vui_seq_not_used;

        old_sps_id = pInfo->active_SPS.seq_parameter_set_id;
        h264_memset(&(pInfo->active_SPS), 0x0, sizeof(seq_param_set_used));

        VTRACE("old_sps_id = %d", old_sps_id);
        status = h264_Parse_SeqParameterSet(parent, pInfo, &(pInfo->active_SPS), &vui_seq_not_used, (int32_t *)pInfo->TMP_OFFSET_REFFRM_PADDR_GL);
        if (status == H264_STATUS_OK) {
            VTRACE("pInfo->active_SPS.seq_parameter_set_id = %d", pInfo->active_SPS.seq_parameter_set_id);
            h264_Parse_Copy_Sps_To_DDR(pInfo, &(pInfo->active_SPS), pInfo->active_SPS.seq_parameter_set_id);
            pInfo->sps_valid = 1;

            if (1==pInfo->active_SPS.pic_order_cnt_type) {
                h264_Parse_Copy_Offset_Ref_Frames_To_DDR(pInfo,(int32_t *)pInfo->TMP_OFFSET_REFFRM_PADDR_GL,pInfo->active_SPS.seq_parameter_set_id);
            }
        }
        ///// Restore the active SPS if new arrival's id changed
        if (old_sps_id>=MAX_NUM_SPS) {
            h264_memset(&(pInfo->active_SPS), 0x0, sizeof(seq_param_set_used));
            pInfo->active_SPS.seq_parameter_set_id = 0xff;
        }
        else {
            if (old_sps_id!=pInfo->active_SPS.seq_parameter_set_id)  {
                h264_Parse_Copy_Sps_From_DDR(pInfo, &(pInfo->active_SPS), old_sps_id);
            }
            else  {
                //h264_memset(&(pInfo->active_SPS), 0x0, sizeof(seq_param_set));
              //  h264_Parse_Copy_Sps_From_DDR(pInfo, &(pInfo->active_SPS), old_sps_id);
                VTRACE("old_sps_id==pInfo->active_SPS.seq_parameter_set_id");
                pInfo->active_SPS.seq_parameter_set_id = 0xff;
            }
        }

        pInfo->number_of_first_au_info_nal_before_first_slice++;
        VTRACE("h264_NAL_UNIT_TYPE_SPS ---");
    }
    break;
    case h264_NAL_UNIT_TYPE_PPS:
    {
        //OS_INFO("*****************************PPS**************************************\n");
        VTRACE("h264_NAL_UNIT_TYPE_PPS +++");
        uint32_t old_sps_id = pInfo->active_SPS.seq_parameter_set_id;
        uint32_t old_pps_id = pInfo->active_PPS.pic_parameter_set_id;
        VTRACE("old_sps_id = %d, old_pps_id = %d", old_sps_id, old_pps_id);

        h264_memset(&pInfo->active_PPS, 0x0, sizeof(pic_param_set));
        pInfo->number_of_first_au_info_nal_before_first_slice++;

        if (h264_Parse_PicParameterSet(parent, pInfo, &pInfo->active_PPS)== H264_STATUS_OK)
        {
            h264_Parse_Copy_Sps_From_DDR(pInfo, &(pInfo->active_SPS), pInfo->active_PPS.seq_parameter_set_id);
            VTRACE("pInfo->active_PPS.seq_parameter_set_id = %d", pInfo->active_PPS.seq_parameter_set_id);
            VTRACE("pInfo->active_SPS.seq_parameter_set_id = %d", pInfo->active_SPS.seq_parameter_set_id);
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
        } else {
            if (old_sps_id<MAX_NUM_SPS)
                h264_Parse_Copy_Sps_From_DDR(pInfo, &(pInfo->active_SPS), old_sps_id);
            if (old_pps_id<MAX_NUM_PPS)
                h264_Parse_Copy_Pps_From_DDR(pInfo, &(pInfo->active_PPS), old_pps_id);
        }
        VTRACE("pInfo->active_PPS.seq_parameter_set_id = %d", pInfo->active_PPS.seq_parameter_set_id);
        VTRACE("pInfo->active_SPS.seq_parameter_set_id = %d", pInfo->active_SPS.seq_parameter_set_id);
        VTRACE("h264_NAL_UNIT_TYPE_PPS ---");
    } //// End of PPS parsing
    break;


    case h264_NAL_UNIT_TYPE_EOSeq:
    case h264_NAL_UNIT_TYPE_EOstream:

        h264_init_dpb(&(pInfo->dpb));

        pInfo->is_current_workload_done=1;

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

/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */

void viddec_h264secure_get_context_size(viddec_parser_memory_sizes_t *size)
{
    /* Should return size of my structure */
    size->context_size = sizeof(struct h264_viddec_parser);
    size->persist_size = MAX_NUM_SPS * sizeof(seq_param_set_all)
                         + MAX_NUM_PPS * sizeof(pic_param_set)
                         + MAX_NUM_SPS * sizeof(int32_t) * MAX_NUM_REF_FRAMES_IN_PIC_ORDER_CNT_CYCLE
                         + sizeof(int32_t) * MAX_NUM_REF_FRAMES_IN_PIC_ORDER_CNT_CYCLE;
}



/*--------------------------------------------------------------------------------------------------*/
//
// The syntax elements reordering_of_pic_nums_idc, abs_diff_pic_num_minus1, and long_term_pic_num
// specify the change from the initial reference picture lists to the reference picture lists to be used
// for decoding the slice

// reordering_of_pic_nums_idc:
// 0: abs_diff_pic_num_minus1 is present and corresponds to a difference to subtract from a picture number prediction value
// 1: abs_diff_pic_num_minus1 is present and corresponds to a difference to add to a picture number prediction value
// 2: long_term_pic_num is present and specifies the long-term picture number for a reference picture
// 3: End loop for reordering of the initial reference picture list
//
/*--------------------------------------------------------------------------------------------------*/

h264_Status h264secure_Parse_Ref_Pic_List_Reordering(h264_Info* pInfo, void *newdata, h264_Slice_Header_t *SliceHeader)
{
    //h264_Slice_Header_t* SliceHeader = &pInfo->SliceHeader;
    int32_t reorder= -1;
    uint32_t code;
    vbp_h264_sliceheader* sliceheader_p = (vbp_h264_sliceheader*) newdata;

    if ((SliceHeader->slice_type != h264_PtypeI) && (SliceHeader->slice_type != h264_PtypeSI))
    {
        SliceHeader->sh_refpic_l0.ref_pic_list_reordering_flag = (uint8_t)(sliceheader_p->parsedSliceHeader.num_reorder_cmds[0] > 0);
        VTRACE("sliceheader_p->parsedSliceHeader.num_reorder_cmds[0] = %d",
            sliceheader_p->parsedSliceHeader.num_reorder_cmds[0]);
        if (SliceHeader->sh_refpic_l0.ref_pic_list_reordering_flag)
        {
            if(sliceheader_p->parsedSliceHeader.num_reorder_cmds[0] > MAX_NUM_REF_FRAMES) {
                return H264_SliceHeader_ERROR;
            }
            for (reorder = 0; reorder < sliceheader_p->parsedSliceHeader.num_reorder_cmds[0]; reorder++) {
                code = sliceheader_p->reorder_cmd[reorder];
                SliceHeader->sh_refpic_l0.reordering_of_pic_nums_idc[reorder] = code >> 24;
                VTRACE("SliceHeader->sh_refpic_l0.reordering_of_pic_nums_idc[%d] = %d", 
                    reorder,
                    SliceHeader->sh_refpic_l0.reordering_of_pic_nums_idc[reorder]
                    );
                if ((SliceHeader->sh_refpic_l0.reordering_of_pic_nums_idc[reorder] == 0) || (SliceHeader->sh_refpic_l0.reordering_of_pic_nums_idc[reorder] == 1))
                {
                    SliceHeader->sh_refpic_l0.list_reordering_num[reorder].abs_diff_pic_num_minus1 = code & 0xFFFFFF;
                    VTRACE("abs_diff_pic_num_minus1 = %d", SliceHeader->sh_refpic_l0.list_reordering_num[reorder].abs_diff_pic_num_minus1);
                }
                else if (SliceHeader->sh_refpic_l0.reordering_of_pic_nums_idc[reorder] == 2)
                {
                    SliceHeader->sh_refpic_l0.list_reordering_num[reorder].long_term_pic_num = code & 0xFFFFFF;
                    VTRACE("long_term_pic_num = %d", SliceHeader->sh_refpic_l0.list_reordering_num[reorder].long_term_pic_num);
                }
                if (SliceHeader->sh_refpic_l0.reordering_of_pic_nums_idc[reorder] == 3)
                {
                    VTRACE("break here");
                    break;
                }
            }
            SliceHeader->sh_refpic_l0.reordering_of_pic_nums_idc[reorder] = 3;
        }
    }

    if (SliceHeader->slice_type == h264_PtypeB)
    {
        SliceHeader->sh_refpic_l1.ref_pic_list_reordering_flag = (uint8_t)(sliceheader_p->parsedSliceHeader.num_reorder_cmds[1] > 0);
        VTRACE("sliceheader_p->parsedSliceHeader.num_reorder_cmds[1] = %d",
            sliceheader_p->parsedSliceHeader.num_reorder_cmds[1]);
        if (SliceHeader->sh_refpic_l1.ref_pic_list_reordering_flag)
        {
            if (sliceheader_p->parsedSliceHeader.num_reorder_cmds[1] > MAX_NUM_REF_FRAMES) {
                return H264_SliceHeader_ERROR;
            }
            for (reorder = 0; reorder < sliceheader_p->parsedSliceHeader.num_reorder_cmds[1]; reorder++) {
                code = *(sliceheader_p->reorder_cmd + sliceheader_p->parsedSliceHeader.num_reorder_cmds[0] + reorder);
                SliceHeader->sh_refpic_l1.reordering_of_pic_nums_idc[reorder] = code >> 24;
                if ((SliceHeader->sh_refpic_l1.reordering_of_pic_nums_idc[reorder] == 0) || (SliceHeader->sh_refpic_l1.reordering_of_pic_nums_idc[reorder] == 1))
                {
                    SliceHeader->sh_refpic_l1.list_reordering_num[reorder].abs_diff_pic_num_minus1 = code & 0xFFFFFF;
                }
                else if (SliceHeader->sh_refpic_l1.reordering_of_pic_nums_idc[reorder] == 2)
                {
                    SliceHeader->sh_refpic_l1.list_reordering_num[reorder].long_term_pic_num = code & 0xFFFFFF;
                }
                if (SliceHeader->sh_refpic_l1.reordering_of_pic_nums_idc[reorder] == 3)
                {
                    break;
                }
            }
            SliceHeader->sh_refpic_l1.reordering_of_pic_nums_idc[reorder] = 3;
        }
    }
    return H264_STATUS_OK;
}

h264_Status h264secure_Parse_Pred_Weight_Table(h264_Info* pInfo, void *newdata, h264_Slice_Header_t *SliceHeader)
{
    uint32_t i =0, j=0;
    uint8_t flag;
    uint32_t weightidx = 0;
    vbp_h264_sliceheader* sliceheader_p = (vbp_h264_sliceheader*) newdata;

    SliceHeader->sh_predwttbl.luma_log2_weight_denom = sliceheader_p->parsedSliceHeader.luma_log2_weight_denom;

    if (pInfo->active_SPS.sps_disp.chroma_format_idc != 0)
    {
        SliceHeader->sh_predwttbl.chroma_log2_weight_denom = sliceheader_p->parsedSliceHeader.chroma_log2_weight_denom;
    }
    for (i=0; i< SliceHeader->num_ref_idx_l0_active; i++)
    {
        flag = ((sliceheader_p->parsedSliceHeader.weights_present[0][0] >> i) & 0x01);
        SliceHeader->sh_predwttbl.luma_weight_l0_flag = flag;
        if (SliceHeader->sh_predwttbl.luma_weight_l0_flag)
        {
            SliceHeader->sh_predwttbl.luma_weight_l0[i] = sliceheader_p->weight[weightidx++];
            SliceHeader->sh_predwttbl.luma_offset_l0[i] = sliceheader_p->weight[weightidx++];
        }
        else
        {
            SliceHeader->sh_predwttbl.luma_weight_l0[i] = (1 << SliceHeader->sh_predwttbl.luma_log2_weight_denom);
            SliceHeader->sh_predwttbl.luma_offset_l0[i] = 0;
        }

        if (pInfo->active_SPS.sps_disp.chroma_format_idc != 0)
        {
            flag = ((sliceheader_p->parsedSliceHeader.weights_present[0][1] >> i) & 0x01);
            SliceHeader->sh_predwttbl.chroma_weight_l0_flag = flag;
            if (SliceHeader->sh_predwttbl.chroma_weight_l0_flag)
            {
                for (j=0; j <2; j++)
                {
                    SliceHeader->sh_predwttbl.chroma_weight_l0[i][j] = sliceheader_p->weight[weightidx++];
                    SliceHeader->sh_predwttbl.chroma_offset_l0[i][j] = sliceheader_p->weight[weightidx++];
                }
            }
            else
            {
                for (j=0; j <2; j++)
                {
                    SliceHeader->sh_predwttbl.chroma_weight_l0[i][j] = (1 << SliceHeader->sh_predwttbl.chroma_log2_weight_denom);
                    SliceHeader->sh_predwttbl.chroma_offset_l0[i][j] = 0;
                }
            }
        }

    }

    if (SliceHeader->slice_type == h264_PtypeB)
    {
        for (i=0; i< SliceHeader->num_ref_idx_l1_active; i++)
        {
            flag = ((sliceheader_p->parsedSliceHeader.weights_present[1][0] >> i) & 0x01);
            SliceHeader->sh_predwttbl.luma_weight_l1_flag = flag;
            if (SliceHeader->sh_predwttbl.luma_weight_l1_flag)
            {
                SliceHeader->sh_predwttbl.luma_weight_l1[i] = sliceheader_p->weight[weightidx++];
                SliceHeader->sh_predwttbl.luma_offset_l1[i] = sliceheader_p->weight[weightidx++];
            }
            else
            {
                SliceHeader->sh_predwttbl.luma_weight_l1[i] = (1 << SliceHeader->sh_predwttbl.luma_log2_weight_denom);
                SliceHeader->sh_predwttbl.luma_offset_l1[i] = 0;
            }

            if (pInfo->active_SPS.sps_disp.chroma_format_idc != 0)
            {
                flag = ((sliceheader_p->parsedSliceHeader.weights_present[1][1] >> i) & 0x01);
                SliceHeader->sh_predwttbl.chroma_weight_l1_flag = flag;
                if (SliceHeader->sh_predwttbl.chroma_weight_l1_flag)
                {
                    for (j=0; j <2; j++)
                    {
                        SliceHeader->sh_predwttbl.chroma_weight_l1[i][j] = sliceheader_p->weight[weightidx++];
                        SliceHeader->sh_predwttbl.chroma_offset_l1[i][j] = sliceheader_p->weight[weightidx++];
                    }
                }
                else
                {
                    for (j=0; j <2; j++)
                    {
                        SliceHeader->sh_predwttbl.chroma_weight_l1[i][j] = (1 << SliceHeader->sh_predwttbl.chroma_log2_weight_denom);
                        SliceHeader->sh_predwttbl.chroma_offset_l1[i][j] = 0;
                    }
                }
            }

        }
    }

    return H264_STATUS_OK;
} ///// End of h264_Parse_Pred_Weight_Table

h264_Status h264secure_Parse_Dec_Ref_Pic_Marking(h264_Info* pInfo, void *newdata,h264_Slice_Header_t *SliceHeader)
{
    vbp_h264_sliceheader* sliceheader_p = (vbp_h264_sliceheader*) newdata;

    uint8_t i = 0;
    uint32_t idx = 0;
    uint32_t code;
    if (pInfo->nal_unit_type == h264_NAL_UNIT_TYPE_IDR)
    {
        SliceHeader->sh_dec_refpic.no_output_of_prior_pics_flag = (uint8_t)sliceheader_p->parsedSliceHeader.flags.bits.no_output_of_prior_pics_flag;
        SliceHeader->sh_dec_refpic.long_term_reference_flag = (uint8_t)sliceheader_p->parsedSliceHeader.flags.bits.long_term_reference_flag;
        pInfo->img.long_term_reference_flag = SliceHeader->sh_dec_refpic.long_term_reference_flag;
    }
    else
    {
        SliceHeader->sh_dec_refpic.adaptive_ref_pic_marking_mode_flag = (uint8_t)(sliceheader_p->parsedSliceHeader.num_mem_man_ops > 0);
        VTRACE("SliceHeader->sh_dec_refpic.adaptive_ref_pic_marking_mode_flag = %d", SliceHeader->sh_dec_refpic.adaptive_ref_pic_marking_mode_flag);
        ///////////////////////////////////////////////////////////////////////////////////////
        //adaptive_ref_pic_marking_mode_flag Reference picture marking mode specified
        //                              Sliding window reference picture marking mode: A marking mode
        //                              providing a first-in first-out mechanism for short-term reference pictures.
        //                              Adaptive reference picture marking mode: A reference picture
        //                              marking mode providing syntax elements to specify marking of
        //                              reference pictures as unused for reference?and to assign long-term
        //                              frame indices.
        ///////////////////////////////////////////////////////////////////////////////////////

        if (SliceHeader->sh_dec_refpic.adaptive_ref_pic_marking_mode_flag)
        {
            do
            {
                if (i < NUM_MMCO_OPERATIONS)
                {
                    code = sliceheader_p->pic_marking[idx++];
                    SliceHeader->sh_dec_refpic.memory_management_control_operation[i] = (uint8_t)(code >> 24);
                    if ((SliceHeader->sh_dec_refpic.memory_management_control_operation[i] == 1) || (SliceHeader->sh_dec_refpic.memory_management_control_operation[i] == 3))
                    {
                        SliceHeader->sh_dec_refpic.difference_of_pic_num_minus1[i] = code & 0xFFFFFF;
                    }

                    if (SliceHeader->sh_dec_refpic.memory_management_control_operation[i] == 2)
                    {
                        SliceHeader->sh_dec_refpic.long_term_pic_num[i] = code & 0xFFFFFF;
                    }

                    if (SliceHeader->sh_dec_refpic.memory_management_control_operation[i] == 6)
                    {
                        SliceHeader->sh_dec_refpic.long_term_frame_idx[i] = code & 0xFFFFFF;
                    }

                    if (SliceHeader->sh_dec_refpic.memory_management_control_operation[i] == 3) {
                        SliceHeader->sh_dec_refpic.long_term_frame_idx[i] = sliceheader_p->pic_marking[idx++];
                    }

                    if (SliceHeader->sh_dec_refpic.memory_management_control_operation[i] == 4)
                    {
                        SliceHeader->sh_dec_refpic.max_long_term_frame_idx_plus1[i] = code & 0xFFFFFF;
                    }

                    if (SliceHeader->sh_dec_refpic.memory_management_control_operation[i] == 5)
                    {
                        pInfo->img.curr_has_mmco_5 = 1;
                    }
                }

                if (i >= NUM_MMCO_OPERATIONS) {
                    return H264_STATUS_ERROR;
                }

            } while (SliceHeader->sh_dec_refpic.memory_management_control_operation[i++] != 0);
        }
    }

    SliceHeader->sh_dec_refpic.dec_ref_pic_marking_count = i;

    return H264_STATUS_OK;
}


uint32_t h264secure_Update_Slice_Header(h264_Info* pInfo, void *newdata, h264_Slice_Header_t *SliceHeader)
{
    h264_Status retStatus = H264_STATUS_OK;
    uint8_t data;
    vbp_h264_sliceheader* sliceheader_p = (vbp_h264_sliceheader*) newdata;
    ///// first_mb_in_slice
    SliceHeader->first_mb_in_slice = sliceheader_p->parsedSliceHeader.first_mb_in_slice;

    ///// slice_type
    data = sliceheader_p->parsedSliceHeader.slice_type;
    SliceHeader->slice_type = (data%5);
    if (SliceHeader->slice_type > h264_PtypeI) {
        retStatus = H264_STATUS_NOTSUPPORT;
        return retStatus;
    }

    SliceHeader->pic_parameter_id  = (uint8_t)sliceheader_p->parsedSliceHeader.pic_parameter_set_id;
    retStatus = h264_active_par_set(pInfo, SliceHeader);

    switch (pInfo->active_SPS.profile_idc)
    {
        case h264_ProfileBaseline:
        case h264_ProfileMain:
        case h264_ProfileExtended:
            pInfo->active_PPS.transform_8x8_mode_flag=0;
            pInfo->active_PPS.pic_scaling_matrix_present_flag =0;
            pInfo->active_PPS.second_chroma_qp_index_offset = pInfo->active_PPS.chroma_qp_index_offset;
        default:
            break;
    }

    uint32_t code;
    int32_t max_mb_num=0;

    SliceHeader->frame_num = (int32_t)sliceheader_p->parsedSliceHeader.frame_num;

    /// Picture structure
    SliceHeader->structure = FRAME;
    SliceHeader->field_pic_flag = 0;
    SliceHeader->bottom_field_flag = 0;

    if (!(pInfo->active_SPS.sps_disp.frame_mbs_only_flag))
    {
        /// field_pic_flag
        SliceHeader->field_pic_flag = (uint8_t)sliceheader_p->parsedSliceHeader.flags.bits.field_pic_flag;

        if (SliceHeader->field_pic_flag)
        {
            SliceHeader->bottom_field_flag = (uint8_t)sliceheader_p->parsedSliceHeader.flags.bits.bottom_field_flag;
            SliceHeader->structure = SliceHeader->bottom_field_flag? BOTTOM_FIELD: TOP_FIELD;
        }
    }

    ////// Check valid or not of first_mb_in_slice
    if (SliceHeader->structure == FRAME) {
        max_mb_num = pInfo->img.FrameHeightInMbs * pInfo->img.PicWidthInMbs;
    } else {
        max_mb_num = pInfo->img.FrameHeightInMbs * pInfo->img.PicWidthInMbs/2;
    }


    if (pInfo->active_SPS.sps_disp.mb_adaptive_frame_field_flag & (!(pInfo->SliceHeader.field_pic_flag))) {
        SliceHeader->first_mb_in_slice <<=1;
    }

    if (SliceHeader->first_mb_in_slice >= max_mb_num) {
        retStatus = H264_STATUS_NOTSUPPORT;
        return retStatus;
    }


    if (pInfo->nal_unit_type == h264_NAL_UNIT_TYPE_IDR)
    {
        SliceHeader->idr_pic_id = sliceheader_p->parsedSliceHeader.idr_pic_id;
    }

    if (pInfo->active_SPS.pic_order_cnt_type == 0)
    {
        SliceHeader->pic_order_cnt_lsb = (uint32_t)sliceheader_p->parsedSliceHeader.pic_order_cnt_lsb;

        if ((pInfo->active_PPS.pic_order_present_flag) && !(SliceHeader->field_pic_flag))
        {
            SliceHeader->delta_pic_order_cnt_bottom = sliceheader_p->parsedSliceHeader.delta_pic_order_cnt_bottom;
        }
        else
        {
            SliceHeader->delta_pic_order_cnt_bottom = 0;
        }
    }

    if ((pInfo->active_SPS.pic_order_cnt_type == 1) && !(pInfo->active_SPS.delta_pic_order_always_zero_flag))
    {
        SliceHeader->delta_pic_order_cnt[0] = sliceheader_p->parsedSliceHeader.delta_pic_order_cnt[0];
        if ((pInfo->active_PPS.pic_order_present_flag) && !(SliceHeader->field_pic_flag))
        {
            SliceHeader->delta_pic_order_cnt[1] = sliceheader_p->parsedSliceHeader.delta_pic_order_cnt[1];
        }
    }

    if (pInfo->active_PPS.redundant_pic_cnt_present_flag)
    {
        SliceHeader->redundant_pic_cnt = sliceheader_p->parsedSliceHeader.redundant_pic_cnt;
        if (SliceHeader->redundant_pic_cnt > 127) {
            retStatus = H264_STATUS_NOTSUPPORT;
            return retStatus;
        }
    } else {
        SliceHeader->redundant_pic_cnt = 0;
    }

    int32_t  slice_alpha_c0_offset, slice_beta_offset;
    uint32_t bits_offset =0, byte_offset =0;
    uint8_t  is_emul =0;

    /// direct_spatial_mv_pred_flag
    if (SliceHeader->slice_type == h264_PtypeB)
    {
        SliceHeader->direct_spatial_mv_pred_flag = (uint8_t)sliceheader_p->parsedSliceHeader.flags.bits.direct_spatial_mv_pred_flag;
    }
    else
    {
        SliceHeader->direct_spatial_mv_pred_flag = 0;
    }
    //
    // Reset ref_idx and Overide it if exist
    //
    SliceHeader->num_ref_idx_l0_active = pInfo->active_PPS.num_ref_idx_l0_active;
    SliceHeader->num_ref_idx_l1_active = pInfo->active_PPS.num_ref_idx_l1_active;

    if ((SliceHeader->slice_type == h264_PtypeP) || (SliceHeader->slice_type == h264_PtypeSP) || (SliceHeader->slice_type == h264_PtypeB))
    {
        SliceHeader->num_ref_idx_active_override_flag  = (uint8_t)sliceheader_p->parsedSliceHeader.flags.bits.num_ref_idx_active_override_flag;
        if (SliceHeader->num_ref_idx_active_override_flag)
        {
            SliceHeader->num_ref_idx_l0_active = sliceheader_p->parsedSliceHeader.num_ref_active_minus1[0]+ 1;
            if (SliceHeader->slice_type == h264_PtypeB)
            {
                SliceHeader->num_ref_idx_l1_active = sliceheader_p->parsedSliceHeader.num_ref_active_minus1[1]+1;
            }
        }
    }

    if (SliceHeader->slice_type != h264_PtypeB) {
        SliceHeader->num_ref_idx_l1_active = 0;
    }

    if ((SliceHeader->num_ref_idx_l0_active > MAX_NUM_REF_FRAMES) || (SliceHeader->num_ref_idx_l1_active > MAX_NUM_REF_FRAMES))
    {
        retStatus = H264_STATUS_NOTSUPPORT;
        return retStatus;
    }

    if (h264secure_Parse_Ref_Pic_List_Reordering(pInfo,newdata,SliceHeader) != H264_STATUS_OK)
    {
        retStatus = H264_STATUS_NOTSUPPORT;
        return retStatus;
    }


    ////
    //// Parse Pred_weight_table but not store it becasue it will be reparsed in HW
    ////
    if (((pInfo->active_PPS.weighted_pred_flag) && ((SliceHeader->slice_type == h264_PtypeP) || (SliceHeader->slice_type == h264_PtypeSP))) || ((pInfo->active_PPS.weighted_bipred_idc == 1) && (SliceHeader->slice_type == h264_PtypeB)))
    {
        if (h264secure_Parse_Pred_Weight_Table(pInfo,newdata, SliceHeader) != H264_STATUS_OK)
        {
            retStatus = H264_STATUS_NOTSUPPORT;
            return retStatus;
        }
    }



    ////
    //// Parse Ref_pic marking if there
    ////
    if (SliceHeader->nal_ref_idc != 0)
    {
        if (h264secure_Parse_Dec_Ref_Pic_Marking(pInfo, newdata, SliceHeader) != H264_STATUS_OK)
        {
            retStatus = H264_STATUS_NOTSUPPORT;
            return retStatus;
        }
    }

    if ((pInfo->active_PPS.entropy_coding_mode_flag) && (SliceHeader->slice_type != h264_PtypeI) && (SliceHeader->slice_type != h264_PtypeSI))
    {
        SliceHeader->cabac_init_idc = sliceheader_p->parsedSliceHeader.cabac_init_idc;
    }
    else
    {
        SliceHeader->cabac_init_idc = 0;
    }

    if (SliceHeader->cabac_init_idc > 2)
    {
        retStatus = H264_STATUS_NOTSUPPORT;
        return retStatus;
    }

    SliceHeader->slice_qp_delta = sliceheader_p->parsedSliceHeader.slice_qp_delta;

    if ( (SliceHeader->slice_qp_delta > (25-pInfo->active_PPS.pic_init_qp_minus26)) || (SliceHeader->slice_qp_delta < -(26+pInfo->active_PPS.pic_init_qp_minus26)))
    {
        retStatus = H264_STATUS_NOTSUPPORT;
        return retStatus;
    }

    if ((SliceHeader->slice_type == h264_PtypeSP)|| (SliceHeader->slice_type == h264_PtypeSI) )
    {
        if (SliceHeader->slice_type == h264_PtypeSP)
        {
            SliceHeader->sp_for_switch_flag  = 0;
        }
        SliceHeader->slice_qs_delta = sliceheader_p->parsedSliceHeader.slice_qs_delta;
        if ( (SliceHeader->slice_qs_delta > (25-pInfo->active_PPS.pic_init_qs_minus26)) || (SliceHeader->slice_qs_delta < -(26+pInfo->active_PPS.pic_init_qs_minus26)) )
        {
            retStatus = H264_STATUS_NOTSUPPORT;
            return retStatus;
        }
    }
    if (pInfo->active_PPS.deblocking_filter_control_present_flag)
    {
        SliceHeader->disable_deblocking_filter_idc = sliceheader_p->parsedSliceHeader.disable_deblocking_filter_idc;
        if (SliceHeader->disable_deblocking_filter_idc != 1)
        {
            SliceHeader->slice_alpha_c0_offset_div2 = sliceheader_p->parsedSliceHeader.slice_alpha_c0_offset_div2;
            slice_alpha_c0_offset = SliceHeader->slice_alpha_c0_offset_div2 << 1;
            if (slice_alpha_c0_offset < -12 || slice_alpha_c0_offset > 12)
            {
                retStatus = H264_STATUS_NOTSUPPORT;
                return retStatus;
            }

            SliceHeader->slice_beta_offset_div2 = sliceheader_p->parsedSliceHeader.slice_beta_offset_div2;
            slice_beta_offset = SliceHeader->slice_beta_offset_div2 << 1;
            if (slice_beta_offset < -12 || slice_beta_offset > 12)
            {
                retStatus = H264_STATUS_NOTSUPPORT;
                return retStatus;
            }
        }
        else
        {
            SliceHeader->slice_alpha_c0_offset_div2 = 0;
            SliceHeader->slice_beta_offset_div2 = 0;
        }
    }

    retStatus = H264_STATUS_OK;
    return retStatus;
}
uint32_t viddec_h264secure_update(void *parent, void *data, uint32_t size)
{
    viddec_pm_cxt_t * parser_cxt = (viddec_pm_cxt_t *)parent;
    struct h264_viddec_parser* parser = (struct h264_viddec_parser*) &parser_cxt->codec_data[0];
    h264_Info * pInfo = &(parser->info);

    h264_Status status = H264_STATUS_ERROR;
    vbp_h264_sliceheader* sliceheader_p = (vbp_h264_sliceheader*) data;

    pInfo->img.g_new_frame = 0;
    pInfo->push_to_cur = 1;
    pInfo->is_current_workload_done =0;
    pInfo->nal_unit_type = 0;
    pInfo->nal_unit_type = sliceheader_p->parsedSliceHeader.nal_unit_type;

    h264_Slice_Header_t next_SliceHeader;

    /// Reset next slice header
    h264_memset(&next_SliceHeader, 0x0, sizeof(h264_Slice_Header_t));
    next_SliceHeader.nal_ref_idc = sliceheader_p->parsedSliceHeader.nal_ref_idc;

    if ( (1==pInfo->primary_pic_type_plus_one)&&(pInfo->got_start))
    {
        pInfo->img.recovery_point_found |=4;
    }
    pInfo->primary_pic_type_plus_one = 0;

    ////////////////////////////////////////////////////////////////////////////
    // Step 2: Parsing slice header
    ////////////////////////////////////////////////////////////////////////////
    /// PWT
    pInfo->h264_pwt_start_byte_offset=0;
    pInfo->h264_pwt_start_bit_offset=0;
    pInfo->h264_pwt_end_byte_offset=0;
    pInfo->h264_pwt_end_bit_offset=0;
    pInfo->h264_pwt_enabled =0;
    /// IDR flag
    next_SliceHeader.idr_flag = (pInfo->nal_unit_type == h264_NAL_UNIT_TYPE_IDR);

    /// Pass slice header
    status = h264secure_Update_Slice_Header(pInfo, sliceheader_p, &next_SliceHeader);

    pInfo->sei_information.recovery_point = 0;
    pInfo->img.current_slice_num++;


    ////////////////////////////////////////////////////////////////////////////
    // Step 3: Processing if new picture coming
    //  1) if it's the second field
    //  2) if it's a new frame
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

        //
        // Update slice structures:
        h264_update_old_slice(pInfo, next_SliceHeader);  //cur->old; next->cur;

        //
        // 1) if resolution change: reset dpb
        // 2) else: init frame store
        h264_update_img_info(pInfo);  //img, dpb

        //
        ///----------------- New frame.boundary detected--------------------
        //
        pInfo->img.second_field = h264_is_second_field(pInfo);
        if (pInfo->img.second_field == 0)
        {
            pInfo->img.g_new_frame = 1;
            h264_dpb_update_queue_dangling_field(pInfo);
            h264_dpb_gaps_in_frame_num_mem_management(pInfo);
        }
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
    }
    else ///////////////////////////////////////////////////// If Not a picture start
    {
        /// Update slice structures: cur->old; next->cur;
        h264_update_old_slice(pInfo, next_SliceHeader);
        /// 1) if resolution change: reset dpb
        /// 2) else: update img info
        h264_update_img_info(pInfo);
    }
    //////////////////////////////////////////////////////////////
    // Step 4: DPB reference list init and reordering
    //////////////////////////////////////////////////////////////

    //////////////////////////////////////////////// Update frame Type--- IDR/I/P/B for frame or field
    h264_update_frame_type(pInfo);

    h264_dpb_update_ref_lists( pInfo);

    return status;
}


