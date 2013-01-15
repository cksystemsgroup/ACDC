#include <stdlib.h>
#include <stdio.h>

#include "acdc.h"
#include "arch.h"
#include "caches.h"
#include "false-sharing.h"


OCollection *allocate_fs_pool(MContext *mc, size_t sz, unsigned long nelem, 
		u_int64_t rctm) {
	

	int num_threads = __builtin_popcountl(TM(rctm));
	//make sure that nelem is a multiple of num_threads
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
	
	int num_threads = __builtin_popcountl(TM(rctm));
	//make sure that nelem is a multiple of num_threads
	nelem += num_threads - (nelem % num_threads);
	
	OCollection *oc = new_collection(mc, OPTIMAL_FALSE_SHARING, sz, nelem, rctm);

	int cache_lines_per_element = (sz / L1_LINE_SZ) + 1;

	oc->start = allocate_aligned(mc, 
			nelem * cache_lines_per_element * L1_LINE_SZ, L1_LINE_SZ);

	return oc;
}

void deallocate_fs_pool(MContext *mc, OCollection *oc) {
	int i;
	for (i = 0; i < oc->num_objects; ++i) {
		 deallocate(mc, ((SharedObject**)oc->start)[i], oc->object_size);
	}
	free(oc->start);
	pthread_barrier_destroy(&oc->barrier);
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
	u_int64_t tm = TM(rctm);
	if (oc->shared_object.rctm != rctm) {
		printf("used assign after share?\n");
		exit(EXIT_FAILURE);
	}
	int num_threads = __builtin_popcountl(tm);
	int *thread_ids = calloc(num_threads, sizeof(int));

	int i, j;
	for (i = 0, j = 0; i < sizeof(u_int64_t); ++i) {
		if ( (1 << i) & tm ) {
			//printf("Bit %d is set\n", i);
			thread_ids[j++] = i;
		}
	}

	int cache_lines_per_element = (oc->object_size / L1_LINE_SZ) + 1;

	for (i = 0; i < oc->num_objects; ++i) {
		char *next = (char*)oc->start + cache_lines_per_element * L1_LINE_SZ * i;
		SharedObject *o = (SharedObject*)next;

		//first object belongs to first thread, second to second...
		// nth object to n%num_threads
		//printf("this object goes to thread: %d\n", thread_ids[i % num_threads]);
		o->rctm = 1 << ( thread_ids[i % num_threads]  );	
	}
	
	
	free(thread_ids);

}

void assign_fs_pool_objects(MContext *mc, OCollection *oc, u_int64_t rctm) {

	//check which threads should participate
	u_int64_t tm = TM(rctm);
	if (oc->shared_object.rctm != rctm) {
		printf("used assign after share?\n");
		exit(EXIT_FAILURE);
	}
	int num_threads = __builtin_popcountl(tm);
	int *thread_ids = calloc(num_threads, sizeof(int));

	int i, j;
	for (i = 0, j = 0; i < sizeof(u_int64_t); ++i) {
		if ( (1 << i) & tm ) {
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

void traverse_optimal_fs_pool(MContext *mc, OCollection *oc) {
	
	//check if thread bit is set in rctm
	u_int64_t tm = TM(oc->shared_object.rctm);
	u_int64_t my_bit = 1 << mc->opt.thread_id;
	
	int cache_lines_per_element = (oc->object_size / L1_LINE_SZ) + 1;

	if (tm & my_bit) {
		//wait at barrier for others
		//TODO: in case we want to use this barrier, we have
		//to re-init it properly
		/*int r = pthread_barrier_wait(&oc->barrier);
		if (!(r == 0 || r == PTHREAD_BARRIER_SERIAL_THREAD)) {
			printf("unable to wait at barrier: %d\n", r);
		}*/

		int i;
		for (i = 0; i < oc->num_objects; ++i) {
			char *next = (char*)oc->start + 
				cache_lines_per_element * L1_LINE_SZ * i;
			SharedObject *so = (SharedObject*)next;
			if (TM(so->rctm) & my_bit) {
				//found my element. wait for neighbours
				pthread_barrier_wait(&oc->barrier);
				access_object(so, oc->object_size, sizeof(SharedObject));
			}
		}
	} //else I don't have access to this collection
}


void traverse_fs_pool(MContext *mc, OCollection *oc) {

	//check if thread bit is set in rctm
	u_int64_t tm = TM(oc->shared_object.rctm);
	u_int64_t my_bit = 1 << mc->opt.thread_id;

	if (tm & my_bit) {
		//wait at barrier for others
		//TODO: in case we want to use this barrier, we have
		//to re-init it properly
		/*int r = pthread_barrier_wait(&oc->barrier);
		if (!(r == 0 || r == PTHREAD_BARRIER_SERIAL_THREAD)) {
			printf("unable to wait at barrier: %d\n", r);
		}*/

		int i;
		for (i = 0; i < oc->num_objects; ++i) {
			//check out what are my objects
			SharedObject *so = ((SharedObject**)oc->start)[i];
			if (TM(so->rctm) & my_bit) {
				access_object(so, oc->object_size, sizeof(SharedObject));
			}
		}
	} //else I don't have access to this collection
}

