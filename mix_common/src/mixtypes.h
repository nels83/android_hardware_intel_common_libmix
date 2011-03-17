#ifndef __MIX_TYPES_H__
#define __MIX_TYPES_H__

/* Provide type definitions for commonly used types.
 *  These are useful because a "int8" can be adjusted
 *  to be 1 byte (8 bits) on all platforms. Similarly and
 *  more importantly, "int32" can be adjusted to be
 *  4 bytes (32 bits) on all platforms.
 */
typedef unsigned char   uchar;
typedef unsigned short  ushort;
typedef unsigned long   ulong;
typedef unsigned int    uint;

typedef signed char int8;
typedef unsigned char uint8;
typedef signed short int16;
typedef unsigned short uint16;

typedef signed int int32;
typedef unsigned int uint32;

typedef signed long long int64;
typedef unsigned long long uint64;

#define TRUE true
#define FALSE false

#define return_if_fail(expr) do{ (void)0; }while(0)
#define return_val_if_fail(expr,val) do{ (void)0; }while(0)

#define INT64_CONSTANT(val)	(val##LL)

#define INT64_FORMAT "lli"
#define UINT64_FORMAT "llu"

#undef	CLAMP
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

#ifndef NULL
#ifdef __cplusplus
#define NULL (0L)
#else /* !__cplusplus */
#define NULL ((void*) 0)
#endif /* !__cplusplus */
#endif


#endif /* __MIX_TYPES_H__ */
