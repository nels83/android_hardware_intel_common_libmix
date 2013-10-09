#ifndef VIDDEC_PM_COMMON_LIST_H
#define VIDDEC_PM_COMMON_LIST_H

/* Limitation:This is the maximum numbers of es buffers between start codes. Needs to change if we encounter
   a case if this is not sufficent */
#define MAX_IBUFS_PER_SC 512

/* This structure is for storing information on byte position in the current access unit.
   stpos is the au byte index of first byte in current es buffer.edpos is the au byte index+1 of last
   valid byte in current es buffer.*/
typedef struct
{
    uint32_t stpos;
    uint32_t edpos;
} viddec_pm_utils_au_bytepos_t;

/* this structure is for storing all necessary information for list handling */
typedef struct
{
    uint16_t num_items;                  /* Number of buffers in List */
    uint16_t first_scprfx_length;        /* Length of first sc prefix in this list */
    int32_t start_offset;                /* starting offset of unused data including sc prefix in first buffer */
    int32_t end_offset;                  /* Offset of unsused data in last buffer including 2nd sc prefix */
    //viddec_input_buffer_t sc_ibuf[MAX_IBUFS_PER_SC]; /* Place to store buffer descriptors */
    viddec_pm_utils_au_bytepos_t data[MAX_IBUFS_PER_SC]; /* place to store au byte positions */
    int32_t total_bytes;                 /* total bytes for current access unit including first sc prefix*/
} viddec_pm_utils_list_t;

/* This function initialises the list to default values */
void viddec_pm_utils_list_init(viddec_pm_utils_list_t *cxt);

#endif
