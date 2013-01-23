 /*
 * Copyright (c) 2012, the ACDC Project Authors.
 * All rights reserved. Please see the AUTHORS file for details.
 * Use of this source code is governed by a BSD license that
 * can be found in the LICENSE file.
 */

#ifndef _ARCH_H
#define	_ARCH_H

#if defined __i386__ || defined __x86_64__

static inline unsigned long long rdtsc(void) {
    unsigned hi, lo;
    asm volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long) lo) | (((unsigned long long) hi) << 32);
}

#endif /* defined __i386__ || defined __x86_64__ */

#endif	/* _ARCH_H */
