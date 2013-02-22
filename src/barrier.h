 /*
 * Copyright (c) 2012, the ACDC Project Authors.
 * All rights reserved. Please see the AUTHORS file for details.
 * Use of this source code is governed by a BSD license that
 * can be found in the LICENSE file.
 */

#include <stdint.h>

struct spin_barrier {
  uint64_t count __attribute__ ((aligned(64)));
  unsigned int max __attribute__ ((aligned(64)));
};
typedef struct spin_barrier spin_barrier_t;

int spin_barrier_init(volatile spin_barrier_t *barrier, unsigned int num_threads);
int spin_barrier_wait(volatile spin_barrier_t *barrier);

