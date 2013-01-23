 /*
 * Copyright (c) 2012, the ACDC Project Authors.
 * All rights reserved. Please see the AUTHORS file for details.
 * Use of this source code is governed by a BSD license that
 * can be found in the LICENSE file.
 */


#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "acdc.h"
#include "arch.h"
#include "caches.h"
#include "false-sharing.h"


int *get_thread_ids(u_int64_t rctm) {
	int num_threads = __builtin_popcountl(rctm);
	int *thread_ids = calloc(num_threads, sizeof(int));
	int i, j;
	for (i = 0, j = 0; i < sizeof(u_int64_t); ++i) {
		if ( (1 << i) & rctm ) {
			thread_ids[j++] = i;
		}
	}
	return thread_ids;
}

OCollection *allocate_fs_pool(MContext *mc, size_t sz, unsigned long nelem, 
		u_int64_t rctm) {
	

	int num_threads = __builtin_popcountl(rctm);
	//make sure that nelem is a multiple of num_threads
	if (nelem % num_threads != 0)
		nelem += num_threads - (nelem % num_threads);

	OCollection *oc = new_collection(mc, FALSE_SHARING, sz, nelem, rctm);

	//we store all objects on an array. one after the other
	oc->start = calloc(nelem, sizeof(Object*));

	int i;
	for (i = 0; i < nelem; ++i) {
		((Object**)oc->start)[i] = allocate(mc, sz);
	}

	return oc;
}

OCollection *allocate_optimal_fs_pool(MContext *mc, size_t sz, unsigned long nelem,
		u_int64_t rctm) {
	
	int num_threads = __builtin_popcountl(rctm);
	//make sure that nelem is a multiple of num_threads
	if (nelem % num_threads != 0)
		nelem += num_threads - (nelem % num_threads);

	assert(nelem % num_threads == 0);
	
	OCollection *oc = new_collection(mc, OPTIMAL_FALSE_SHARING, sz, nelem, rctm);

	int cache_lines_per_element = (sz / L1_LINE_SZ) + 1;

	oc->start = allocate_aligned(mc, 
			nelem * cache_lines_per_element * L1_LINE_SZ, L1_LINE_SZ);

	return oc;
}

void deallocate_fs_pool(MContext *mc, OCollection *oc) {

	assert(oc->reference_map == 0);
	assert(oc->start != NULL);

	int i;
	for (i = 0; i < oc->num_objects; ++i) {
		 deallocate(mc, ((Object**)oc->start)[i], oc->object_size);
	}
	free(oc->start);
	oc->start = NULL;
	free(oc);
}

void deallocate_optimal_fs_pool(MContext *mc, OCollection *oc) {
	
	int cache_lines_per_element = (oc->object_size / L1_LINE_SZ) + 1;

	deallocate_aligned(mc, oc->start, 
			oc->num_objects * cache_lines_per_element * L1_LINE_SZ,
			L1_LINE_SZ);

	free(oc);
}

//TODO: refactor. same code twice
/*
void assign_optimal_fs_pool_objects(MContext *mc, OCollection *oc, u_int64_t rctm) {

	//check which threads should participate
	int num_threads = __builtin_popcountl(rctm);
	int *thread_ids = get_thread_ids(rctm);

	int cache_lines_per_element = (oc->object_size / L1_LINE_SZ) + 1;
	assert(cache_lines_per_element == 1);

	int i;
	for (i = 0; i < oc->num_objects; ++i) {
		char *next = (char*)oc->start + cache_lines_per_element * L1_LINE_SZ * i;
		SharedObject *o = (SharedObject*)next;
		o->rctm = 1 << ( thread_ids[i % num_threads] );
	}

	assert(i % num_threads == 0);
	free(thread_ids);
}


void assign_fs_pool_objects(MContext *mc, OCollection *oc, u_int64_t rctm) {

	//check which threads should participate
	int num_threads = __builtin_popcountl(rctm);
	int *thread_ids = get_thread_ids(rctm);

	int i;
	for (i = 0; i < oc->num_objects; ++i) {
		SharedObject *o = ((SharedObject**)oc->start)[i];
		o->rctm = 1 << ( thread_ids[i % num_threads]  );	
	}
	free(thread_ids);
}
*/

/////////////////////////////////////////////
//false sharing with small objects
/////////////////////////////////////////////
OCollection *allocate_small_fs_pool(MContext *mc, size_t sz, unsigned long nelem,
                              u_int64_t rctm) {

	//idea: only create num_threads objects and every thread gets one
	int num_threads = __builtin_popcountl(rctm);
	assert (num_threads == nelem);

	OCollection *oc = allocate_fs_pool(mc, sz, nelem, rctm);
	//assign_fs_pool_objects(mc, oc, rctm);
	return oc;
}

OCollection *allocate_small_optimal_fs_pool(MContext *mc, size_t sz, 
		unsigned long nelem, u_int64_t rctm) {

	OCollection *oc = allocate_optimal_fs_pool(mc, sz, nelem, rctm);
	//assign_optimal_fs_pool_objects(mc, oc, rctm);
	return oc;
}

void deallocate_small_fs_pool(MContext *mc, OCollection *oc) {
	//same thing, different interface
	deallocate_fs_pool(mc, oc);
}

void deallocate_small_optimal_fs_pool(MContext *mc, OCollection *oc) {

	deallocate_optimal_fs_pool(mc, oc);
}

void traverse_small_fs_pool(MContext *mc, OCollection *oc) {
	//check if thread bit is set in rctm
	//u_int64_t my_bit = 1 << mc->opt.thread_id;

	assert(oc->reference_map != 0);
	assert(oc->start != NULL);

	int i;
	for (i = 0; i < oc->num_objects; ++i) {
		//check out what are my objects
		Object *so = ((Object**)oc->start)[i];
		//if (so->rctm & my_bit) {
			//printf("ACCESS\n");
			int j;
			assert(oc->reference_map != 0);
			//long long access_start = rdtsc();
			for (j = 0; j < mc->gopts->access_iterations; ++j)
				access_object(so, oc->object_size, sizeof(Object));
			//long long access_end = rdtsc();
			//mc->stat->access_time += access_end - access_start;
		//}
	}
}
void traverse_small_optimal_fs_pool(MContext *mc, OCollection *oc) {

	//check if thread bit is set in rctm
	//u_int64_t my_bit = 1 << mc->opt.thread_id;

	assert(oc->reference_map != 0);
	assert(oc->start != NULL);

	int cache_lines_per_element = (oc->object_size / L1_LINE_SZ) + 1;
	
	int i;
	for (i = 0; i < oc->num_objects; ++i) {
		char *next = (char*)oc->start + 
			cache_lines_per_element * L1_LINE_SZ * i;
		Object *so = (Object*)next;
		
		assert(oc->reference_map != 0);
		
		//if (so->rctm & my_bit) {
			//printf("ACCESS\n");
			int j;
			assert(oc->reference_map != 0);
			//long long access_start = rdtsc();
			for (j = 0; j < mc->gopts->access_iterations; ++j)
				access_object(so, oc->object_size, sizeof(Object));
			//long long access_end = rdtsc();
			//mc->stat->access_time += access_end - access_start;
		//}
	}
}

