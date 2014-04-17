 /*
 * Copyright (c) 2012, the ACDC Project Authors.
 * All rights reserved. Please see the AUTHORS file for details.
 * Use of this source code is governed by a BSD license that
 * can be found in the LICENSE file.
 */

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>

//mmap osx hack
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif // MAP_ANONYMOUS

static void *metadata_heap_start;
static void *metadata_heap_end;
static void *metadata_heap_bump_pointer;

//returns a rounded up to multiple of r
static inline int round_up_to(int a, int r) {
        if (a < r) return r;
        return ((a / r) * r) + r;
}

static void *align_address(void *ptr, size_t alignment) {
	long addr = ((long)ptr)+ (alignment-1);
	addr = addr & ~(alignment-1);
	return (void*)addr;
}

void init_metadata_heap(size_t heapsize, int do_warmup) {
	
        printf("fetching %lu MB of metadata space\n", heapsize/1024);

        void *r = mmap(NULL, heapsize * (1UL<<20), PROT_READ | PROT_WRITE, 
                       MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
        if (r == MAP_FAILED) {
                perror("mmap");
                exit(errno);
        }

        metadata_heap_start = r;
        metadata_heap_end = metadata_heap_start + (heapsize * (1UL<<20));
        metadata_heap_bump_pointer = metadata_heap_start;

        if (do_warmup) {
                printf("warming up %lu MB of metadata space\n", heapsize);
                //make heap hot
                unsigned long i;
                volatile char *c = (char*)metadata_heap_start;
                for (i = 0; i < heapsize * (1UL<<20); i = i + 4096) {
                        c[i] = c[i-1];
                }
        }
}

static void *get_chunk(size_t size) {
	void *ptr = metadata_heap_bump_pointer;
	metadata_heap_bump_pointer += size;
	if (metadata_heap_bump_pointer >= metadata_heap_end) {
		printf("out of metadata space. Increase with -H option\n");
		exit(EXIT_FAILURE);
	}
	return ptr;
}

void *malloc_meta(size_t size) {
	//default alignment: 16 bytes
	return align_address(get_chunk(size + 15), 16);
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

