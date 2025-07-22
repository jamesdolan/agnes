#ifndef common_h
#define common_h

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef AGNES_AMALGAMATED
#define AGNES_INTERNAL static
#else
#define AGNES_INTERNAL
#endif

#define AGNES_GET_BIT(byte, bit_ix) (((byte) >> (bit_ix)) & 1)

#endif /* common_h */
