/*
 * Copyright (c) 2016, the ACDC Project Authors.
 * All rights reserved. Please see the AUTHORS file for details.
 * Use of this source code is governed by a BSD license that
 * can be found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include "localizer.h"
#include "metadata-allocator.h"


void printMem(LocalizerContext *ctx) {
  int i;
  for(i= 0; i < ctx->vmem_available; i++) {
    int pmem_idx = ctx->vmem[i];
    int pmem_value = ctx->pmem[pmem_idx];
    printf("vmem[%d] = %d, pmem[vmem[%d]] = %d\n", i, pmem_idx, i, pmem_value);
  }

  printf("VMEM\n");
  for(i= 0; i < ctx->vmem_available; i++)
    printf("vmem[%d] = %d\n", i, ctx->vmem[i]);

  printf("PMEM\n");
  for(i= 0; i < ctx->pmem_available; i++)
    printf("pmem[%d] = %d\n", i, ctx->pmem[i]);
}


void 
localizer_init(LocalizerContext *ctx, unsigned int nr_objs, size_t obj_size) 
{
  int units = (obj_size > kLocalizerGranularity) 
                ? (obj_size % kLocalizerGranularity) 
                  ? obj_size / kLocalizerGranularity + 1
                  : obj_size / kLocalizerGranularity
                : kLocalizerGranularity;

  size_t mem_required = nr_objs * units;
  printf(" -- localizer_init: nr_objs=%d, obj_size=%zu, mem_required=%zu", nr_objs, obj_size, mem_required);

  ctx->vmem_available = mem_required;
  ctx->vmem = malloc_meta(ctx->vmem_available);

  ctx->pmem_available = mem_required;
  ctx->pmem = malloc_meta(ctx->pmem_available);

  int i;
  for(i= 0; i < ctx->vmem_available; i++)
    ctx->vmem[i] = kEmpty;

  for(i= 0; i < ctx->pmem_available; i++)
    ctx->pmem[i] = kEmpty;

  ctx->pmem_allocated = 0;
  ctx->pmem_bump_ptr = 0;
  ctx->pmem_free_list_head = kEmpty;
  printf(" ... DONE\n");

}

void
localizer_alloc(LocalizerContext *ctx, LocalizerVMemRange *range, size_t size) {
  printf(" -- localizer_alloc: size=%zu\n", size);
  int nr_of_vmem_entries = size / kLocalizerGranularity;
  if(nr_of_vmem_entries < 1) {
    printf("LOCALIZER ERROR (localizer_alloc): "
      "object cannot be allocated, to small. (obj.size: %zu, pmem granularity: %d)\n", 
      size, kLocalizerGranularity);
    exit(EXIT_FAILURE);
  }

  range->start = kEmpty;
  range->end = kEmpty;

  int i, k;
  for (i = 0, k = nr_of_vmem_entries - 1; k < ctx->vmem_available; i++, k++) {
    if(ctx->vmem[i] != kAllocated && ctx->vmem[k] != kAllocated){
      range->start = i;
      range->end = k;
      for(; i <= k; i++) ctx->vmem[i] = kAllocated;
      break;
    }
  }

  if(range->start == kEmpty || range->end == kEmpty) {
    printf("LOCALIZER ERROR (localizer_alloc): "
      "failed to allocate virtual memory, not enough space.\n");
    exit(EXIT_FAILURE);
  }
}

void
localizer_free(LocalizerContext *ctx, LocalizerVMemAddr vaddr)
{
  // size_t vmem_idx = get_vmem_idx(addr, offset);
  // if(ctx->vmem[vmem_idx] == kEmpty) {
  //   printf("LOCALIZER ERROR (localizer_free): "
  //     "vmem index already freed. (idx=%zu, addr=%p, offset=%zu)\n", 
  //     vmem_idx, addr, offset);
  //   exit(EXIT_FAILURE);
  // }
  
  // size_t pmem_idx = ctx->vmem[vmem_idx];
  
  // // add freed memory to free list
  // if(ctx->pmem_free_list_head == kEmpty) {
  //   ctx->pmem_free_list_head = pmem_idx;
  //   ctx->pmem[pmem_idx] = kEmpty;
  // } else {
  //   ctx->pmem[pmem_idx] = ctx->pmem_free_list_head;
  //   ctx->pmem_free_list_head = pmem_idx;

  // }

  // ctx->vmem[vmem_idx] = kEmpty;
  // ctx->pmem_allocated -= kLocalizerGranularity;
}

int
localizer_allocate_pmem(LocalizerContext *ctx, LocalizerVMemAddr vaddr)
{
  if(ctx->vmem[vaddr] != kAllocated) {
    printf("LOCALIZER ERROR (localizer_allocate_pmem): "
      "vmem index already used. (vaddr=%d)\n", vaddr);
    exit(EXIT_FAILURE);
  }

  int paddr = kEmpty;
  if(ctx->pmem_free_list_head == kEmpty && ctx->pmem_bump_ptr < ctx->pmem_available){
    paddr = ctx->pmem_bump_ptr;
    ctx->pmem_bump_ptr += 1;
  } else if(ctx->pmem_free_list_head != kEmpty) {
    paddr = ctx->pmem_free_list_head;
    ctx->pmem_free_list_head = ctx->pmem[ctx->pmem_free_list_head];
    ctx->pmem[ctx->pmem_free_list_head] = kEmpty;  // not required.
  } else {
    printf("LOCALIZER ERROR (localizer_allocate_pmem): "
      "failed to allocate physical memory, not enough space.\n");
    exit(EXIT_FAILURE);
  }
  
  ctx->vmem[vaddr] = paddr;
  ctx->pmem[paddr] = kEmpty;
  ctx->pmem_allocated += kLocalizerGranularity;

  return paddr;
}

int 
localizer_prepare_access(LocalizerContext *ctx, LocalizerVMemAddr vaddr) 
{
  if(vaddr < 0 || vaddr > ctx->vmem_available)
  {
    printf("LOCALIZER ERROR (localizer_prepare_access): "
      "vaddr out of range. (0 <= vaddr < %zu)\n", ctx->vmem_available);
    exit(EXIT_FAILURE);
  }
  
  if(ctx->vmem[vaddr] == kAllocated) {
    printf("asdfasdfsdf\n")
    ctx->vmem[vaddr] = localizer_allocate_pmem(ctx, vaddr);
  }

  if(ctx->vmem[vaddr] < 0 || ctx->vmem[vaddr] > ctx->pmem_available)
  {
    printf("LOCALIZER ERROR (localizer_prepare_access): "
      "paddr out of range. (0 <= paddr(%d) < %zu, 0 <= vaddr(%d) < %zu)\n", 
        ctx->vmem[vaddr], ctx->pmem_available, vaddr, ctx->vmem_available);

    printf("localizer_prepare_access\n");
    printMem(ctx);


    exit(EXIT_FAILURE);
  }

  return ctx->vmem[vaddr];
}

void
localizer_write(LocalizerContext *ctx, LocalizerVMemAddr vaddr, LocalizerPMemType value) 
{
  int paddr = localizer_prepare_access(ctx, vaddr);
  ctx->pmem[paddr] = value;
}

LocalizerPMemType
localizer_read(LocalizerContext *ctx, LocalizerVMemAddr vaddr) {
  int paddr = localizer_prepare_access(ctx, vaddr);
  return ctx->pmem[paddr];
}
