/*
 * Copyright (c) 2016, the ACDC Project Authors.
 * All rights reserved. Please see the AUTHORS file for details.
 * Use of this source code is governed by a BSD license that
 * can be found in the LICENSE file.
 */

#ifndef LOCALIZER_H_
#define LOCALIZER_H_

typedef int LocalizerPMemType;
typedef int LocalizerVMemAddr;
typedef struct vmem_range LocalizerVMemRange;
typedef struct context LocalizerContext;

struct context {
  int *vmem;                  // virtual memory: maps virtual to physical addresses
  LocalizerPMemType *pmem;    // physical memory: simulates physical memory in uses
  size_t vmem_available;      // size of available virtual memory
  size_t pmem_available;      // size of available physical memory
  size_t pmem_allocated;      // size of currently allocated physical memory
  int pmem_bump_ptr;          // current bump pointer position
  int pmem_free_list_head;    // head of free list (initial NULL)
};

struct vmem_range {
  LocalizerVMemAddr start;
  LocalizerVMemAddr end;
};

static const LocalizerPMemType kEmpty = -1;
static const LocalizerPMemType kAllocated = -2;
static const int kBitmask = 255;
static const int kLocalizerGranularity = sizeof(void*);    // current granularity = one word
static const int kVMemOversizeFactor = 10;


// initialize with expected memory size
void localizer_init(LocalizerContext *ctx, unsigned int nr_objs, size_t obj_size);
// call on first access of address
void localizer_alloc(LocalizerContext *ctx, LocalizerVMemRange *range, size_t size);
// call on last access of address
void localizer_free(LocalizerContext *ctx, LocalizerVMemAddr vaddr);
// call to read a virtual address
LocalizerPMemType localizer_read(LocalizerContext *ctx, LocalizerVMemAddr vaddr);
// call to write a virtual address
void localizer_write(LocalizerContext *ctx, LocalizerVMemAddr vaddr, LocalizerPMemType value);
#endif // LOCALIZER_H_
