#include "viddec_parser_ops.h"
#include "h264.h"
#include "h264parse.h"
#include "h264parse_dpb.h"


extern void* h264_memcpy( void* dest, void* src, uint32_t num );

uint32_t cp_using_dma(uintptr_t ddr_addr, uintptr_t local_addr, uint32_t size, char to_ddr, char swap)
{
    if (swap != 0)
    {
        //g_warning("swap copying is not implemented.");
    }

    if (to_ddr)
    {
        memcpy((void*)ddr_addr, (void*)local_addr, size);
    }
    else
    {
        memcpy((void*)local_addr, (void*)ddr_addr, size);
    }

    return (0);
}

#if 0
void h264_parse_emit_start_new_frame( void *parent, h264_Info *pInfo )
{

    if (pInfo->Is_first_frame_in_stream) //new stream, fill new frame in cur
    {

        pInfo->img.g_new_frame = 0;
        pInfo->Is_first_frame_in_stream =0;
        pInfo->push_to_cur = 1;

    }
    else  // move to next for new frame
    {
        pInfo->push_to_cur = 0;
    }



    //fill dpb managemnt info




    pInfo->dpb.frame_numbers_need_to_be_displayed =0;
    pInfo->dpb.frame_numbers_need_to_be_removed =0;
    pInfo->dpb.frame_numbers_need_to_be_allocated =0;


}

void h264_parse_emit_eos( void *parent, h264_Info *pInfo )
{
    ////
    //// Now we can flush out all frames in DPB fro display
    if (pInfo->dpb.fs[pInfo->dpb.fs_dec_idc].is_used != 3)
    {
        h264_dpb_mark_dangling_field(&pInfo->dpb, pInfo->dpb.fs_dec_idc);  //, DANGLING_TYPE_GAP_IN_FRAME
    }

    h264_dpb_store_previous_picture_in_dpb(pInfo, 0,0);
    h264_dpb_flush_dpb(pInfo, 1, 0, pInfo->active_SPS.num_ref_frames);


    pInfo->dpb.frame_numbers_need_to_be_displayed =0;
    pInfo->dpb.frame_numbers_need_to_be_removed =0;

}

void h264_parse_emit_current_pic( void *parent, h264_Info *pInfo )
{
    pInfo->qm_present_list=0;
}

void h264_parse_emit_current_slice( void *parent, h264_Info *pInfo )
{
#if 1
    uint32_t  i, nitems=0;


    if ( (h264_PtypeB==pInfo->SliceHeader.slice_type)||(h264_PtypeP==pInfo->SliceHeader.slice_type) )
    {
        if (pInfo->SliceHeader.sh_refpic_l0.ref_pic_list_reordering_flag)
        {
            nitems = pInfo->SliceHeader.num_ref_idx_l0_active;

            for (i=0; i<nitems; i++)
            {
                if (viddec_h264_get_is_non_existent(&(pInfo->dpb.fs[pInfo->slice_ref_list0[i]&0x1f]))==0)
                {
                    pInfo->h264_list_replacement = (pInfo->slice_ref_list0[i]&0xFF)|0x80;
                    break;
                }
            }
        }
        else
        {
            nitems = pInfo->dpb.listXsize[0];

            for (i=0; i<nitems; i++)
            {
                if (viddec_h264_get_is_non_existent(&(pInfo->dpb.fs[pInfo->dpb.listX_0[i]&0x1f]))==0)
                {
                    pInfo->h264_list_replacement = (pInfo->dpb.listX_0[i]&0xFF)|0x80;
                    break;
                }
            }
        }

    }
    else
    {
        nitems =0;
    }
#endif
}
#else


void h264_parse_emit_current_slice( void *parent, h264_Info *pInfo )
{

    h264_slice_data 				slice_data = {};

    uint32_t		i=0, nitems=0, data=0;
    uint32_t 	bits_offset =0, byte_offset =0;
    uint8_t    	is_emul =0;


    ////////////////////// Update Reference list //////////////////
    if ( (h264_PtypeB==pInfo->SliceHeader.slice_type)||(h264_PtypeP==pInfo->SliceHeader.slice_type) )
    {
        if (pInfo->SliceHeader.sh_refpic_l0.ref_pic_list_reordering_flag)
        {
            nitems = pInfo->SliceHeader.num_ref_idx_l0_active;

            for (i=0; i<nitems; i++)
            {
                if (viddec_h264_get_is_non_existent(&(pInfo->dpb.fs[pInfo->slice_ref_list0[i]&0x1f]))==0)
                {
                    pInfo->h264_list_replacement = (pInfo->slice_ref_list0[i]&0xFF)|0x80;
                    break;
                }
            }
        }
        else
        {
            nitems = pInfo->dpb.listXsize[0];

            for (i=0; i<nitems; i++)
            {
                if (viddec_h264_get_is_non_existent(&(pInfo->dpb.fs[pInfo->dpb.listX_0[i]&0x1f]))==0)
                {
                    pInfo->h264_list_replacement = (pInfo->dpb.listX_0[i]&0xFF)|0x80;
                    break;
                }
            }
        }

    }
    else
    {
        nitems =0;
    }
    /////file ref list 0
    // h264_parse_emit_ref_list(parent, pInfo, 0);

    /////file ref list 1
    //h264_parse_emit_ref_list(parent, pInfo, 1);

    ////////////////////////////////// Update ES Buffer for Slice ///////////////////////
    viddec_pm_get_au_pos(parent, &bits_offset, &byte_offset, &is_emul);

    //OS_INFO("DEBUG---entropy_coding_mode_flag:%d, bits_offset: %d\n", pInfo->active_PPS.entropy_coding_mode_flag, bits_offset);

    return;
}


void h264_parse_emit_current_pic( void *parent, h264_Info *pInfo )
{

    const uint32_t             *pl;
    uint32_t                   i=0,nitems=0;

    h264_pic_data pic_data;

    pInfo->qm_present_list=0;
    // How many payloads must be generated
    nitems = (sizeof(h264_pic_data) + 7) / 8; // In QWORDs rounded up

    pl = (const uint32_t *) &pic_data;

    return;
}

void h264_parse_emit_start_new_frame( void *parent, h264_Info *pInfo )
{

    uint32_t i=0,nitems=0;

    ///////////////////////// Frame attributes//////////////////////////

// Remove workload related stuff
# if 0
    //Push data into current workload if first frame or frame_boundary already detected by non slice nal
    if ( (pInfo->Is_first_frame_in_stream)||(pInfo->is_frame_boundary_detected_by_non_slice_nal))
    {
        //viddec_workload_t			*wl_cur = viddec_pm_get_header( parent );
        //pInfo->img.g_new_frame = 0;
        pInfo->Is_first_frame_in_stream =0;
        pInfo->is_frame_boundary_detected_by_non_slice_nal=0;
        pInfo->push_to_cur = 1;
        //h264_translate_parser_info_to_frame_attributes(wl_cur, pInfo);
    }
    else  // move to cur if frame boundary detected by previous non slice nal, or move to next if not
    {
        //viddec_workload_t        *wl_next = viddec_pm_get_next_header (parent);

        pInfo->push_to_cur = 0;
        //h264_translate_parser_info_to_frame_attributes(wl_next, pInfo);

        pInfo->is_current_workload_done=1;
    }
#endif

    ///////////////////// SPS/////////////////////
    // h264_parse_emit_sps(parent, pInfo);

    pInfo->dpb.frame_numbers_need_to_be_displayed =0;


    /////////////////////release frames/////////////////////
    pInfo->dpb.frame_numbers_need_to_be_removed =0;

    /////////////////////flust frames (do not display)/////////////////////
    pInfo->dpb.frame_numbers_need_to_be_dropped =0;

    /////////////////////updata DPB frames/////////////////////

    /////////////////////Alloc buffer for current Existing frame/////////////////////
    pInfo->dpb.frame_numbers_need_to_be_allocated =0;
    return;
}



void h264_parse_emit_eos( void *parent, h264_Info *pInfo )
{

    uint32_t nitems=0, i=0;
    ////
    //// Now we can flush out all frames in DPB fro display
    if (viddec_h264_get_is_used(&(pInfo->dpb.fs[pInfo->dpb.fs_dec_idc])) != 3)
    {
        h264_dpb_mark_dangling_field(&pInfo->dpb, pInfo->dpb.fs_dec_idc);  //, DANGLING_TYPE_GAP_IN_FRAME
    }

    h264_dpb_store_previous_picture_in_dpb(pInfo, 0,0);
    h264_dpb_flush_dpb(pInfo, 1, 0, pInfo->active_SPS.num_ref_frames);


    /////////////////////display frames/////////////////////
    pInfo->dpb.frame_numbers_need_to_be_displayed =0;


    /////////////////////release frames/////////////////////
    pInfo->dpb.frame_numbers_need_to_be_removed =0;

    return;
}
#endif
