 /*
 * Copyright (c) 2012, the ACDC Project Authors.
 * All rights reserved. Please see the AUTHORS file for details.
 * Use of this source code is governed by a BSD license that
 * can be found in the LICENSE file.
 */

#ifndef _ARCH_H
#define	_ARCH_H

#include <stdint.h>

#if defined __i386__ || defined __x86_64__

static inline unsigned long long rdtsc(void) {
    unsigned hi, lo;
    asm volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long) lo) | (((unsigned long long) hi) << 32);
}

#ifdef __GCC_HAVE_SYNC_COMPARE_AND_SWAP_16
typedef unsigned int uint128_t __attribute__((mode(TI)));

static inline uint64_t high(uint128_t word) {
  return (uint64_t)(word >> 64);
}
static inline uint64_t low(uint128_t word) {
  return (uint64_t)(word);
}

static inline unsigned int map_bits(uint128_t map) {
  return __builtin_popcountl(high(map)) + __builtin_popcountl(low(map));
}

#else

static inline uint64_t high(uint64_t word) {
  return 0UL;
}
static inline uint64_t low(uint64_t word) {
  return word;
}

static inline unsigned int map_bits(uint64_t map) {
  return __builtin_popcountl(map);
}

#endif /* __GCC_HAVE_SYNC_COMPARE_AND_SWAP_16 */



#endif /* defined __i386__ || defined __x86_64__ */

#endif	/* _ARCH_H */
