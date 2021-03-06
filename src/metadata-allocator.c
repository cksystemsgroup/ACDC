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

#include "acdc.h"

//mmap osx hack
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#define MAP_HUGETLB 0;
#endif // MAP_ANONYMOUS

static void *metadata_heap_start;
static void *metadata_heap_end;
static void *metadata_heap_bump_pointer;

//returns a rounded up to multiple of r
__attribute__((unused))
static inline int round_up_to(int a, int r) {
        if (a < r) return r;
        return ((a / r) * r) + r;
}

static void *align_address(void *ptr, size_t alignment) {
	long addr = ((long)ptr)+ (alignment-1);
	addr = addr & ~(alignment-1);
	return (void*)addr;
}

void init_metadata_heap(GOptions *gopts) {
	
        int mmap_flags = MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB;
        size_t heapsize = gopts->metadata_heap_sz;

        printf("fetching %lu MB of metadata space\n", heapsize);
        printf("trying to map %lu 2MB pages... ", heapsize/2);

        void *r = mmap(NULL, heapsize * (1UL<<20), PROT_READ | PROT_WRITE, 
                       mmap_flags, 0, 0);
        if (r == MAP_FAILED) {
                printf(" failed.\nRetrying 4k pages... ");

                //retry using 4k pages
                mmap_flags = MAP_ANONYMOUS | MAP_PRIVATE;
                
                r = mmap(NULL, heapsize * (1UL<<20), PROT_READ | PROT_WRITE, 
                         mmap_flags, 0, 0);
                if (r == MAP_FAILED) {
                        perror("mmap");
                        exit(errno);
                }
        } else {
                gopts->use_hugepages = 1;
        }
        printf("OK\n");

        metadata_heap_start = r;
        metadata_heap_end = metadata_heap_start + (heapsize * (1UL<<20));
        metadata_heap_bump_pointer = metadata_heap_start;

        printf("warming up %lu MB of metadata space... ", heapsize);
        //make heap hot
        unsigned long i;
        unsigned long inc = 4096;
        if (gopts->use_hugepages) inc = HUGEPAGE_KB * 1024;

        volatile char *c = (char*)metadata_heap_start;
        for (i = 1; i < heapsize * (1UL<<20); i = i + inc) {
                c[i] = c[i-1];
        }
        printf("Done!\n");
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

