/* ///////////////////////////////////////////////////////////////////////
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2008 Intel Corporation. All Rights Reserved.
//
//  Description: Common functions for parsing VC-1 bitstreams.
//
*/

#ifndef _VC1PARSE_H_
#define _VC1PARSE_H_
#include <vbp_common.h>
#include <vbp_trace.h>
#include "viddec_parser_ops.h"
#include "vc1.h"

/** @weakgroup vc1parse_defs VC-1 Parse Definitions */
/** @ingroup vc1parse_defs */
/*@{*/

extern void *memset(void *s, int32_t c, uint32_t n);

/* This macro gets the next numBits from the bitstream. */
#define VC1_GET_BITS VC1_GET_BITS9
#define VC1_GET_BITS9(numBits, value) \
{   uint32_t __tmp__; \
    viddec_pm_get_bits(ctxt, (uint32_t*)&__tmp__, numBits ); \
    value = __tmp__; \
}

#define VC1_PEEK_BITS(numBits, value) \
{   uint32_t __tmp__; \
    viddec_pm_peek_bits(ctxt, (uint32_t*)&__tmp__, numBits ); \
    value = __tmp__; \
}

/* This macro asserts if the condition is not true. */
#define VC1_ASSERT(condition) \
{ \
    if (! (condition)) \
        ETRACE("Failed " #condition "!\n"); \
}

/*@}*/

/** @weakgroup vc1parse VC-1 Parse Functions */
/** @ingroup vc1parse */
/*@{*/

extern const uint8_t VC1_MVMODE_LOW_TBL[];
extern const uint8_t VC1_MVMODE_HIGH_TBL[];
extern const int32_t VC1_BITPLANE_IMODE_TBL[];
extern const int32_t VC1_BITPLANE_K_TBL[];
extern const int32_t VC1_BFRACTION_TBL[];
extern const int32_t VC1_REFDIST_TBL[];


/* Top-level functions to parse bitstream layers for rcv format. */
vc1_Status vc1_ParseRCVSequenceLayer (void* ctxt, vc1_Info *pInfo);

/* Top-level functions to parse bitstream layers for the various profiles. */
vc1_Status vc1_ParseSequenceLayer(void* ctxt, vc1_Info *pInfo);
vc1_Status vc1_ParseEntryPointLayer(void* ctxt, vc1_Info *pInfo);
vc1_Status vc1_ParseSliceLayer(void* ctxt, vc1_Info *pInfo);
vc1_Status vc1_ParsePictureLayer(void* ctxt, vc1_Info *pInfo);
vc1_Status vc1_ParseFieldLayer(void* ctxt, vc1_Info *pInfo);

/* Top-level functions to parse headers for various picture layers for the
simple and main profiles. */
vc1_Status vc1_ParsePictureHeader(void* ctxt, vc1_Info *pInfo);
vc1_Status vc1_ParsePictureHeader_ProgressiveIpicture(void* ctxt, vc1_Info *pInfo);
vc1_Status vc1_ParsePictureHeader_ProgressivePpicture(void* ctxt, vc1_Info *pInfo);
vc1_Status vc1_ParsePictureHeader_ProgressiveBpicture(void* ctxt, vc1_Info *pInfo);

/* Top-level functions to parse common part of the headers for various picture
layers for the advanced profile. */
vc1_Status vc1_ParsePictureHeader_Adv(void* ctxt, vc1_Info *pInfo);
vc1_Status vc1_ParseFieldHeader_Adv (void* ctxt, vc1_Info *pInfo);
vc1_Status vc1_ParsePictureFieldHeader_Adv(void* ctxt, vc1_Info *pInfo);

/* Functions to parse remainder part of the headers for various progressive
picture layers for the advanced profile. */
vc1_Status vc1_ParsePictureHeader_ProgressiveIpicture_Adv(void* ctxt, vc1_Info *pInfo);
vc1_Status vc1_ParsePictureHeader_ProgressivePpicture_Adv(void* ctxt, vc1_Info *pInfo);
vc1_Status vc1_ParsePictureHeader_ProgressiveBpicture_Adv(void* ctxt, vc1_Info *pInfo);

/* Functions to parse remainder part of the headers for various interlace frame
layers for the advanced profile. */
vc1_Status vc1_ParsePictureHeader_InterlaceIpicture_Adv(void* ctxt, vc1_Info *pInfo);
vc1_Status vc1_ParsePictureHeader_InterlacePpicture_Adv(void* ctxt, vc1_Info *pInfo);
vc1_Status vc1_ParsePictureHeader_InterlaceBpicture_Adv(void* ctxt, vc1_Info *pInfo);

/* Functions to parse remainder part of the headers for various interlace frame
layers for the advanced profile. */
vc1_Status vc1_ParseFieldHeader_InterlaceIpicture_Adv(void* ctxt, vc1_Info *pInfo);
vc1_Status vc1_ParseFieldHeader_InterlacePpicture_Adv(void* ctxt, vc1_Info *pInfo);
vc1_Status vc1_ParseFieldHeader_InterlaceBpicture_Adv(void* ctxt, vc1_Info *pInfo);

/* Functions to parse syntax element in bitstream. */
vc1_Status vc1_MVRangeDecode(void* ctxt, vc1_Info *pInfo);
vc1_Status vc1_DMVRangeDecode(void* ctxt, vc1_Info *pInfo);
vc1_Status vc1_CalculatePQuant(vc1_Info *pInfo);
vc1_Status vc1_VOPDQuant(void* ctxt, vc1_Info *pInfo);
vc1_Status vc1_DecodeBitplane(void* ctxt, vc1_Info *pInfo, uint32_t width, uint32_t height, vc1_bpp_type_t bptype);
vc1_Status vc1_DecodeHuffmanOne(void* ctxt, int32_t *pDst, const int32_t *pDecodeTable);
vc1_Status vc1_DecodeHuffmanPair(void* ctxt, const int32_t *pDecodeTable, int8_t *pFirst, int16_t *pSecond);

/*@}*/

#endif /* _VC1PARSE_H_. */
