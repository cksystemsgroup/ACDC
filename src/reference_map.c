/*
 * Copyright (c) 2012, the ACDC Project Authors.
 * All rights reserved. Please see the AUTHORS file for details.
 * Use of this source code is governed by a BSD license that
 * can be found in the LICENSE file.
 */

#include <assert.h>
#include <pthread.h>

#include "reference_map.h"

unsigned int get_bit(bitfield_t __bitfield, unsigned int __index) {
  assert(__index <= MAX_NUM_THREADS);
  return (                                                   
    (unsigned long)__bitfield[__index / BITS_PER_LONG] &       
    ( 1UL << (__index % BITS_PER_LONG) )        
  ) > 0 ? 1 : 0;
}

void set_bit(bitfield_t __bitfield, unsigned int __index) {
  assert(__index <= MAX_NUM_THREADS);
  __bitfield[__index / BITS_PER_LONG] |=
  ( 1UL << (__index % BITS_PER_LONG) );
}

void clear_bit(bitfield_t __bitfield, unsigned int __index) {
  assert(__index <= MAX_NUM_THREADS);
  __bitfield[__index / BITS_PER_LONG] &=
  ~( 1UL << (__index % BITS_PER_LONG) );
}

void addReference(ReferenceMap *map, unsigned long thread_id) {
  assert(thread_id < MAX_NUM_THREADS);

  pthread_spin_lock(&(map->lock));
  if (!get_bit(map->thread_map, thread_id)) {
    set_bit(map->thread_map, thread_id);
    map->reference_count++;
  }
  pthread_spin_unlock(&(map->lock));
}

unsigned long deleteReference(ReferenceMap *map, unsigned long thread_id) {
  assert(thread_id < MAX_NUM_THREADS);
  
  unsigned long reference_count_after_update;
  pthread_spin_lock(&(map->lock));
  if (get_bit(map->thread_map, thread_id)) {
    clear_bit(map->thread_map, thread_id);
    map->reference_count--;
  }
  reference_count_after_update = map->reference_count;
  pthread_spin_unlock(&(map->lock));
  return reference_count_after_update;
}

