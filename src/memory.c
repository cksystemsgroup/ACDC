 /*
 * Copyright (c) 2012, the ACDC Project Authors.
 * All rights reserved. Please see the AUTHORS file for details.
 * Use of this source code is governed by a BSD license that
 * can be found in the LICENSE file.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "acdc.h"

static void *metadata_heap_start;
static void *metadata_heap_end;
static void *metadata_heap_bump_pointer;

static void *align_address(void *ptr, size_t alignment) {
	long addr = ((long)ptr)+ (alignment-1);
	addr = addr & ~(alignment-1);
	return (void*)addr;
}
void init_metadata_heap(size_t heapsize) {
	metadata_heap_start = sbrk(heapsize * 1024); //parameter is in kB
	if (metadata_heap_start == (void*)-1) {
		printf("unable to allocate metadata heap\n");
		exit(EXIT_FAILURE);
	}
	metadata_heap_end = sbrk(0);
	metadata_heap_bump_pointer = metadata_heap_start;
}
static void *get_chunk(size_t size) {
	metadata_heap_bump_pointer += size;
	if (metadata_heap_bump_pointer >= metadata_heap_end) {
		printf("out of metadata space. Increase -H option\n");
		exit(EXIT_FAILURE);
	}
	return metadata_heap_bump_pointer;
}

void *malloc_meta(size_t size) {
	//default alignment: 8 bytes
	return align_address(get_chunk(size + 7), 8);
}

void *calloc_meta(size_t nelem, size_t size) {
	void *ptr = malloc_meta(nelem * size);
	int i;
	for (i = 0; i < nelem*size; ++i) {
		((char*)ptr)[i] = '\0';
	}
	return ptr;
}

void *malloc_meta_aligned(size_t size, size_t alignment) {
	return align_address(get_chunk(size + alignment - 1), alignment);
}
void *calloc_meta_aligned(size_t nelem, size_t size, size_t alignment) {
	void *ptr = malloc_meta_aligned(nelem * size, alignment);
	int i;
	for (i = 0; i < nelem*size; ++i) {
		((char*)ptr)[i] = '\0';
	}
	return ptr;
}

Object *allocate(MContext *mc, size_t size) {
	void *ptr;
		
	if (size < sizeof(Object)) {
		printf("Error: min object size is %lu. Requested: %lu\n",
				sizeof(Object),
				size);
		exit(EXIT_FAILURE);
	}

	ptr = malloc(size);
	
	//set header information
	Object *o = (Object*)ptr;

	//update mutator stats
	mc->stat->bytes_allocated += size;
	mc->stat->objects_allocated++;

	return o;
}

Object *allocate_aligned(MContext *mc, size_t size, size_t alignment) {

	Object *o = allocate(mc, size + alignment + sizeof(Object*));
	Object *aligned_o = (Object*)(((long)o + alignment + sizeof(Object*)) &
		~(alignment-1));

	((void**)aligned_o)[-1] = o;

	return aligned_o;
}

void deallocate_aligned(MContext *mc, Object *o, size_t size, size_t alignment) {

	deallocate(mc, ((void**)o)[-1], size + alignment + sizeof(Object*));
}

void deallocate(MContext *mc, Object *o, size_t size) {

	//update mutator stats
	mc->stat->bytes_deallocated += size;
	mc->stat->objects_deallocated++;

	free(o);
}


void write_object(Object *o, size_t size, size_t offset) {
	int i;
	size_t pl_sz = size - offset;
	//payload starts after header
	volatile char *payload = (char*)o + offset;
	
	for (i = 1; i < pl_sz; ++i) {
		payload[i] = payload[i-1] + 1;
	}
}


unsigned int get_sizeclass(size_t size) {
	return (unsigned int)log2((double)size);
}

