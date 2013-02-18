 /*
 * Copyright (c) 2012, the ACDC Project Authors.
 * All rights reserved. Please see the AUTHORS file for details.
 * Use of this source code is governed by a BSD license that
 * can be found in the LICENSE file.
 */

#ifndef METADATA_ALLOCATOR_H
#define METADATA_ALLOCATOR_H

#include <stdlib.h>

void init_metadata_heap(size_t heapsize);
void *malloc_meta(size_t size);
void *calloc_meta(size_t nelem, size_t size);
void *malloc_meta_aligned(size_t size, size_t alignment);
void *calloc_meta_aligned(size_t nelem, size_t size, size_t alignment);

#endif // METADATA_ALLOCATOR_H
