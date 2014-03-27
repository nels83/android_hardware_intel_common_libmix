#ifndef VIDEO_FRAME_INFO_H_
#define VIDEO_FRAME_INFO_H_

#define MAX_NUM_NALUS 16

typedef struct {
    uint8_t  type;      // nalu type + nal_ref_idc
    uint32_t offset;    // offset to the pointer of the encrypted data
    uint8_t* data;      // if the nalu is encrypted, this field is useless; if current NALU is SPS/PPS, data is the pointer to clear SPS/PPS data
    uint32_t length;    // nalu length
} nalu_info_t;

typedef struct {
    uint8_t* data;      // pointer to the encrypted data
    uint32_t size;      // encrypted data size
    uint32_t num_nalus; // number of NALU
    nalu_info_t nalus[MAX_NUM_NALUS];
} frame_info_t;

#endif
