/*
 * Copyright (c) 2012, the ACDC Project Authors.
 * All rights reserved. Please see the AUTHORS file for details.
 * Use of this source code is governed by a BSD license that
 * can be found in the LICENSE file.
 */

#ifndef REFERENCE_MAP_H
#define REFERENCE_MAP_H

#include <assert.h>
#include <pthread.h>


#define MAX_NUM_THREADS ((1UL << 14)) //must be power of 2 larger than 64
#define BITS_PER_LONG (( sizeof(unsigned long) * 8 ))
#define THREAD_MAP_SIZE ((MAX_NUM_THREADS / BITS_PER_LONG ))

typedef unsigned long bitfield_t[THREAD_MAP_SIZE];

typedef struct reference_map ReferenceMap;
struct reference_map {
  pthread_spinlock_t lock;
  unsigned long reference_count;
  bitfield_t thread_map;
};

unsigned int get_bit(bitfield_t __bitfield, unsigned int __index);
void set_bit(bitfield_t __bitfield, unsigned int __index);
void clear_bit(bitfield_t __bitfield, unsigned int __index);
void addReference(ReferenceMap *map, unsigned long thread_id);
unsigned long deleteReference(ReferenceMap *map, unsigned long thread_id);


#endif // REFERENCE_MAP_H
