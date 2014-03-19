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
#else
// TODO
#endif /* defined __i386__ || defined __x86_64__ */

#if __GNUC__ > 3 || \
        (__GNUC__ == 3 && __GNUC_MINOR__ > 6)

static inline uint64_t atomic_uint64_decrement(uint64_t *value) {
        return __atomic_sub_fetch(value, (uint64_t)1, __ATOMIC_SEQ_CST);
}

#else
// TODO
#endif // __GCC

#endif	/* _ARCH_H */
