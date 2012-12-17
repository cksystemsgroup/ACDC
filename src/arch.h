/*
 * Copyright (c) 2010 Martin Aigner, Andreas Haas
 * http://cs.uni-salzburg.at/~maigner
 * http://cs.uni-salzburg.at/~ahaas
 *
 * University Salzburg, www.uni-salzburg.at
 * Department of Computer Science, cs.uni-salzburg.at
 */

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _ARCH_H
#define	_ARCH_H

#define atomic_int_inc(atomic) (atomic_int_add ((atomic), 1))
#define atomic_int_dec_and_test(atomic)				\
  (atomic_int_exchange_and_add ((atomic), -1) == 1)

static inline void toggle_bit_at_pos(int *bitmap, int pos) {
    *bitmap = *bitmap ^ (1 << pos);
}


#if defined __i386__ || defined __x86_64__

static inline unsigned long long rdtsc(void) {
    unsigned hi, lo;
    asm volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long) lo) | (((unsigned long long) hi) << 32);
}

/* 32bit bit-map operations */

/* bit scan forward returns the index of the LEAST significant bit
 * or -1 if bitmap==0 */
static inline int bsfl(int bitmap) {

    int result;

    __asm__ ("bsfl %1, %0;"      /* Bit Scan Forward                         */
             "jnz 1f;"           /* if(ZF==1) invalid input of 0; jump to 1: */
             "movl $-1, %0;"     /* set output to error -1                   */
             "1:"                /* jump label for line 2                    */
            : "=r" (result)
            : "g" (bitmap)
            );
    return result;
}

/* bit scan reverse returns the index of the MOST significant bit
 * or -1 if bitmap==0 */
static inline int bsrl(int bitmap) {

    int result;

    __asm__("bsrl %1, %0;"
            "jnz 1f;"
            "movl $-1, %0;"
            "1:"
            : "=r" (result)
            : "g" (bitmap)
            );
    return result;
}

/*code adapted from glib http://ftp.gnome.org/pub/gnome/sources/glib/2.24/
 * g_atomic_*: atomic operations.
 * Copyright (C) 2003 Sebastian Wilhelmi
 * Copyright (C) 2007 Nokia Corporation
 */
static inline int atomic_int_exchange_and_add(volatile int *atomic,
        int val) {

    int result;

    __asm__ __volatile__("lock; xaddl %0,%1"
            : "=r" (result), "=m" (*atomic)
            : "0" (val), "m" (*atomic));
    return result;
}

static inline void atomic_int_add(volatile int *atomic, int val) {
    __asm__ __volatile__("lock; addl %1,%0"
            : "=m" (*atomic)
            : "ir" (val), "m" (*atomic));
}

static inline int atomic_int_compare_and_exchange(volatile int *atomic,
        int oldval, int newval) {

    int result;

    __asm__ __volatile__("lock; cmpxchgl %2, %1"
            : "=a" (result), "=m" (*atomic)
            : "r" (newval), "m" (*atomic), "0" (oldval)
            );

    return result;
}

#endif /* defined __i386__ || defined __x86_64__ */

#endif	/* _ARCH_H */
