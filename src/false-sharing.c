#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "acdc.h"
#include "arch.h"
#include "caches.h"
#include "false-sharing.h"


OCollection *allocate_fs_pool(MContext *mc, size_t sz, unsigned long nelem, 
		u_int64_t rctm) {
	

	int num_threads = __builtin_popcountl(rctm);
	//make sure that nelem is a multiple of num_threads
	if (nelem % num_threads != 0)
		nelem += num_threads - (nelem % num_threads);

	OCollection *oc = new_collection(mc, FALSE_SHARING, sz, nelem, rctm);

	//we store all objects on an array. one after the other
	oc->start = calloc(nelem, sizeof(SharedObject*));

	int i;
	for (i = 0; i < nelem; ++i) {
		((SharedObject**)oc->start)[i] = allocate(mc, sz);
	}

	return oc;
}

OCollection *allocate_optimal_fs_pool(MContext *mc, size_t sz, unsigned long nelem,
		u_int64_t rctm) {
	
	int num_threads = __builtin_popcountl(rctm);
	//make sure that nelem is a multiple of num_threads
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
		 deallocate(mc, ((SharedObject**)oc->start)[i], oc->object_size);
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
void assign_optimal_fs_pool_objects(MContext *mc, OCollection *oc, u_int64_t rctm) {


	//check which threads should participate
	int num_threads = __builtin_popcountl(rctm);
	int *thread_ids = calloc(num_threads, sizeof(int));

	int i, j;
	j = 0;
	for (i = 0; i < sizeof(u_int64_t); ++i) {
		if ( (1 << i) & rctm ) {
			//printf("Bit %d is set\n", i);
			thread_ids[j++] = i;
		}
	}

	//DEBUG
	//printf("Sharing threads: ");
	for (i = 0; i < num_threads; ++i) {
	//	printf("%d ", thread_ids[i]);
	}
	//printf("\n");

	int cache_lines_per_element = (oc->object_size / L1_LINE_SZ) + 1;

	for (i = 0; i < oc->num_objects; ++i) {
		char *next = (char*)oc->start + cache_lines_per_element * L1_LINE_SZ * i;
		SharedObject *o = (SharedObject*)next;

		//first object belongs to first thread, second to second...
		// nth object to n%num_threads
		//printf("object %d goes to thread: %d\n", i, thread_ids[i % num_threads]);
		o->rctm = 1 << ( thread_ids[i % num_threads] );
	}

	assert(i % num_threads == 0);
	
	free(thread_ids);
}

void assign_fs_pool_objects(MContext *mc, OCollection *oc, u_int64_t rctm) {

	//check which threads should participate
	int num_threads = __builtin_popcountl(rctm);
	int *thread_ids = calloc(num_threads, sizeof(int));

	int i, j;
	for (i = 0, j = 0; i < sizeof(u_int64_t); ++i) {
		if ( (1 << i) & rctm ) {
			//printf("Bit %d is set\n", i);
			thread_ids[j++] = i;
		}
	}

	for (i = 0; i < oc->num_objects; ++i) {
		SharedObject *o = ((SharedObject**)oc->start)[i];
		
		//first object belongs to first thread, second to second...
		// nth object to n%num_threads
		//printf("this object goes to thread: %d\n", thread_ids[i % num_threads]);
		o->rctm = 1 << ( thread_ids[i % num_threads]  );	
	}

	free(thread_ids);

}


/////////////////////////////////////////////
//false sharing with small objects
/////////////////////////////////////////////
OCollection *allocate_small_fs_pool(MContext *mc, size_t sz, unsigned long nelem,
                              u_int64_t rctm) {

	//idea: only create num_threads objects and every thread gets one
	int num_threads = __builtin_popcountl(rctm);
	assert (num_threads == nelem);

	OCollection *oc = allocate_fs_pool(mc, sz, nelem, rctm);
	assign_fs_pool_objects(mc, oc, rctm);
	return oc;
}

OCollection *allocate_small_optimal_fs_pool(MContext *mc, size_t sz, unsigned long nelem,
                                      u_int64_t rctm) {

	return NULL;
}
void deallocate_small_fs_pool(MContext *mc, OCollection *oc) {
	//same thing, different interface
	deallocate_fs_pool(mc, oc);
}

void deallocate_small_optimal_fs_pool(MContext *mc, OCollection *oc) {}

void traverse_small_fs_pool(MContext *mc, OCollection *oc) {
	//check if thread bit is set in rctm
	u_int64_t my_bit = 1 << mc->opt.thread_id;

	assert(oc->reference_map != 0);
	assert(oc->start != NULL);

	if (oc->sharing_map & my_bit) {
		int i;
		for (i = 0; i < oc->num_objects; ++i) {
			//check out what are my objects
			SharedObject *so = ((SharedObject**)oc->start)[i];
			if (so->rctm & my_bit) {
				int j;
				assert(oc->reference_map != 0);
				//long long access_start = rdtsc();
				for (j = 0; j < 10000000; ++j)
					access_object(so, oc->object_size, sizeof(SharedObject));
				//long long access_end = rdtsc();
				//mc->stat->access_time += access_end - access_start;
			}
		}
	} //else I don't have access to this collection

}
void traverse_small_optimal_fs_pool(MContext *mc, OCollection *oc) {}
void assign_small_fs_pool_objects(MContext *mc, OCollection *oc, u_int64_t rctm) {}
void assign_small_optimal_fs_pool_objects(MContext *mc, OCollection *oc, u_int64_t rctm) {}

