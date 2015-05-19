#ifndef VBP_COMMON
#define VBP_COMMON

#define SWAP_BYTE(x,y,z)   (( ( (x) >> ((y) << 3))& 0xFF)  << ((z) << 3))
#define SWAP_WORD(x)      ( SWAP_BYTE((x),0,3) | SWAP_BYTE((x),1,2) |SWAP_BYTE((x),2,1) |SWAP_BYTE((x),3,0))

#define DEB

#endif
