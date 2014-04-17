/*
 * Copyright (c) 2012, the ACDC Project Authors.
 * All rights reserved. Please see the AUTHORS file for details.
 * Use of this source code is governed by a BSD license that
 * can be found in the LICENSE file.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <assert.h>
#include <sys/time.h>

#include "acdc.h"
#include "arch.h"
#include "memory.h"
#include "metadata-allocator.h"
#include "barrier.h"
#include "proc-status.h"
#include "caches.h"

//distribution pool consisting of one heap class per thread
//and a lock for each heap class
static LClass **shared_heap_classes;
static pthread_mutex_t *shared_heap_classes_locks;
static volatile spin_barrier_t false_sharing_barrier;
static pthread_barrier_t acdc_barrier;
static pthread_mutex_t debug_lock = PTHREAD_MUTEX_INITIALIZER;

static void unreference_and_deallocate_LSClass(MContext *mc, LSClass *c);

void _debug(MContext *mc, char *filename, int linenum, const char *format, ...) {
	if (mc->gopts->verbosity < 2) return;
	va_list args;
	pthread_mutex_lock(&debug_lock);
        fprintf(stdout, "[Debug] %s %4d T%2d @ %4d ", 
			filename,
			linenum,
			mc->thread_id, 
			mc->time);
	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);
	fprintf(stdout, "\n");
	pthread_mutex_unlock(&debug_lock);
}

/*
static inline void set_bit(reference_map_t *word, int bitpos) {
	*word |= (BIT_ZERO << bitpos);
}

static inline void unset_bit(reference_map_t *word, int bitpos) {
	*word &= ~(BIT_ZERO << bitpos);
}
*/

/*
 * returns an unused LSCNode either from a thread-local node cache
 * of from the node_buffer_memory until it's out of space
 */
static LSCNode *get_LSCNode(MContext *mc) {

	LSCNode *node = mc->node_cache.first;
	if (node != NULL) {
		lclass_remove(&mc->node_cache, node);
		debug(mc, "reuse node %p\n", node);
		node->next = NULL;
		node->prev = NULL;
		return node;
	}

	if (mc->node_buffer_counter >= mc->gopts->node_buffer_size) {
		printf("node buffer overflow. increase -N option: %lu\n", 
				mc->gopts->node_buffer_size);
		exit(EXIT_FAILURE);
	}
	node = (LSCNode*)(((uint64_t)mc->node_buffer_memory) + 
		(mc->node_buffer_counter * L1_LINE_SZ));
	mc->node_buffer_counter++;
	debug(mc, "node %p\n", node);
	node->next = NULL;
	node->prev = NULL;
        node->ls_class = NULL;
	return node;
}

/*
 * put a LSCNode back to the thread-local node cache
 */
static void recycle_LSCNode(MContext *mc, LSCNode *node) {
	debug(mc, "give back node %p\n", node);
	lclass_insert_beginning(&mc->node_cache, node);
}

/* 
 * heap class API
 * inserts a lifetime-size-class 'c' in the proper lifetime-class
 * of the heap-class 'heap_class'. The index for insertion is
 * determined from the lifetime of 'c' and the thread clock in 'mc'
 */
static void heap_class_insert(MContext *mc, LClass *heap_class, 
		LSClass *c) {

	unsigned long liveness = c->lifetime - mc->gopts->deallocation_delay;

	unsigned int insert_index = (mc->time + liveness) % 
		(mc->gopts->max_liveness + 
                mc->gopts->deallocation_delay);

	LClass *target_lifetime_class = &heap_class[insert_index];

	//wrap LSClass in a LSCNode object
	LSCNode *node = get_LSCNode(mc);
	node->ls_class = c;

	lclass_insert_end(target_lifetime_class, node);
}

/*
 * returns a pointer to the lifetime-class that corresponds to
 * 'remaining_liveness' in 'heap_class'
 */
static LClass *heap_class_get_LClass(MContext *mc, LClass *heap_class,
		unsigned int remaining_liveness) {

	assert(remaining_liveness < mc->gopts->max_liveness);

	int index = (mc->time + remaining_liveness) % 
		(mc->gopts->max_liveness + mc->gopts->deallocation_delay);
	return &heap_class[index];
}

/*
 * removes the expired lifetime-class from 'heap_class' and
 * deallocates (or just unreferences) the objects in each lifetime-size-class
 * that belong to the expired lifetime-class
 */
static void heap_class_remove(MContext *mc, LClass *heap_class) {

	int delete_index = (mc->time) % 
		(mc->gopts->max_liveness + mc->gopts->deallocation_delay);
	delete_index -= mc->gopts->deallocation_delay;
	if (delete_index < 0)
		delete_index += (mc->gopts->max_liveness +
				mc->gopts->deallocation_delay);

	LClass *expired_lifetime_class = &heap_class[delete_index];

	debug(mc, "delete from index %d", delete_index);

	//forall LSClasses in this LClass: 
	//         decrement reference map and maybe deallocate LSClass
	LSCNode *iterator = expired_lifetime_class->first;
	while (iterator != NULL) {
		unreference_and_deallocate_LSClass(mc, iterator->ls_class);
		LSCNode *tmp = iterator;
		iterator = iterator->next;
		recycle_LSCNode(mc, tmp);
	}
	//reset list
	expired_lifetime_class->first = NULL;
	expired_lifetime_class->last = NULL;
}

/*
 * allocates a heap-class, i.e., an array of lifetime-classes
 * where we have one lifetime-class for each lifetime in
 * [min. liveness, max. liveness]
 */
static LClass *allocate_heap_class(unsigned int max_lifetime) {

	LClass *ec = calloc_meta(max_lifetime, sizeof(LClass));
	//calloc creates zeroed memory, i.e., the first and last
	//pointers of each LClass are NULL
	return ec;
}

/*
 * setup the space and initial state of the thread-local meta data
 */
static MContext *create_mutator_context(GOptions *gopts, unsigned int thread_id) {
	MContext *mc = calloc_meta_aligned(1, sizeof(MContext), L1_LINE_SZ);
	mc->gopts = gopts;
	mc->time = 0;
	
	mc->thread_id = thread_id;

	mc->rand = gopts->seed + thread_id; // different seeds for different threads

	mc->stat = calloc_meta(1, sizeof(MStat));
	mc->stat->lt_histogram = calloc_meta(gopts->max_liveness + 1, 
			sizeof(unsigned long));
	mc->stat->sz_histogram = calloc_meta(gopts->max_object_sc + 1, 
			sizeof(unsigned long));

	mc->heap_class = allocate_heap_class(
			gopts->max_liveness + gopts->deallocation_delay);

	shared_heap_classes[thread_id] = allocate_heap_class(
			gopts->max_liveness); 
	//no deallocation delay necessary here, we only distribute LSClasses 
	//with lifetimes ranging from 1 to max_liveness
	
	int r = pthread_mutex_init(&shared_heap_classes_locks[thread_id], NULL);
	if (r != 0) {
		printf("unable to init mutex: %d\n", r);
		exit(EXIT_FAILURE);
	}

	mc->node_buffer_memory = malloc_meta_aligned(
			L1_LINE_SZ * gopts->node_buffer_size,
			L1_LINE_SZ);
	mc->node_buffer_counter = 0;
	mc->node_cache.first = NULL;
	mc->node_cache.last = NULL;
       	
	assert(sizeof(LSClass) <= L1_LINE_SZ);
//        uint32_t lsc_size = ((sizeof(LSClass) / L1_LINE_SZ) * (L1_LINE_SZ)) + L1_LINE_SZ;
	mc->class_buffer_memory = malloc_meta_aligned(
			L1_LINE_SZ * gopts->class_buffer_size,
                                L1_LINE_SZ);
	mc->class_buffer_counter = 0;
	mc->class_cache.first = NULL;
	mc->class_cache.last = NULL;

        mc->thread_id_buffer = calloc_meta(gopts->num_threads, sizeof(int));

	return mc;
}

/*
 * record memory usage 
 */
static void get_and_print_memstats(MContext *mc) {

	update_proc_status(mc->gopts->pid);
	mc->stat->current_rss = get_resident_set_size();
	
	//warmup phase: start memory measurements after max-liveness
	//units of time
	if (mc->time >= 2 * mc->gopts->max_liveness) {
		mc->stat->resident_set_size_counter +=
			mc->stat->current_rss;
		mc->stat->vm_peak = get_vm_peak();
		mc->stat->rss_hwm = get_high_water_mark();
	}

	if (mc->gopts->do_metadata_warmup) {
                if (mc->stat->current_rss < mc->gopts->metadata_heap_sz) {
                        printf("FAULTY RSS SAMPLE: %ld\n", mc->stat->current_rss);
                        return;
                }
        }
	
	if (mc->gopts->verbosity == 0) return;
        
        if (mc->gopts->do_metadata_warmup) {
                printf("MEMSTATS\t%s\t%3u\t%4u\t%12lu\n",
                                mc->gopts->allocator_name,
                                mc->thread_id,
                                mc->time,
                                mc->stat->current_rss - mc->gopts->metadata_heap_sz
                      );
        } else {
	        printf("MEMSTATS\t%s\t%3u\t%4u\t%12lu\n",
			mc->gopts->allocator_name,
			mc->thread_id,
			mc->time,
			mc->stat->current_rss
	      );
        }
}

/*
 * print runtime statistics
 */
static void print_runtime_stats(MContext *mc) {

	if (mc->gopts->verbosity == 0) return;

	printf("STATS\t%s\t%3u\t%4u\t%12lu\t%12lu\t%12lu\t%12lu\n",
			mc->gopts->allocator_name,
			mc->thread_id,
			mc->time,
			mc->stat->bytes_allocated,
			mc->stat->bytes_deallocated,
			mc->stat->objects_allocated,
			mc->stat->objects_deallocated
		);
}

/*
 * process the objects of a lifetime-size-class of an expired lifetime-class.
 * Shared objects have their bitmap decremented and
 * unshared objects are deallocated
 */
static void unreference_and_deallocate_LSClass(MContext *mc, LSClass *c) {

        if (atomic_uint64_decrement(&(c->reference_counter)) == 0) {
                deallocate_LSClass(mc, c);
	        debug(mc, "deleted %p", c);
        } else {
		//someone else will deallocate
		debug(mc, "%p can be deleted by %d others", c, c->reference_counter);
        }
}

/*
 * traverses all lifetime-size-classes in all live lifetime-classes
 * and access the objects.
 * runs in O(number of live objects)
 */
static void access_live_LClasses(MContext *mc) {

	if (mc->gopts->access_live_objects == 0) return;

	int i;
	for (i = 0; i < mc->gopts->max_liveness; ++i) {
		
		LClass *lc = heap_class_get_LClass(mc, mc->heap_class, i);

		//traverse all LSClasses in lc
		LSCNode *iterator = lc->first;
		while (iterator != NULL) {
			traverse_LSClass(mc, iterator->ls_class);
			iterator = iterator->next;
		}
	}
}

static void lock_shared_heap_class(int thread_id) {
	pthread_mutex_lock(&shared_heap_classes_locks[thread_id]);
}
static void unlock_shared_heap_class(int thread_id) {
	pthread_mutex_unlock(&shared_heap_classes_locks[thread_id]);
}

/*
 * puts a shared lifetime-size-class 'c' in the shared heap class of each
 * receiving thread.
 * runs in O(number of threads)
 */
static void share_LSClass(MContext *mc, LSClass *c) {

        int i, number_of_receiving_threads;

        get_random_thread_selection(mc, mc->thread_id_buffer, &number_of_receiving_threads);

        //set reference counter
        c->reference_counter = number_of_receiving_threads;

        for (i = 0; i < number_of_receiving_threads; ++i) {
                int receiving_thread_id = mc->thread_id_buffer[i];

                //printf("sharing with %d\n", receiving_thread_id);

                LClass *heap_class = shared_heap_classes[receiving_thread_id];
                //lock the shared heap class of this thread
                lock_shared_heap_class(receiving_thread_id);
                //insert LSClass
                heap_class_insert(mc, heap_class, c);
                unlock_shared_heap_class(receiving_thread_id);
        }
        
}



/* a thread can call this function to fetch all lifetime-classes from its
 * shared heap-class to its local heap-class.
 *  this method runs in O(max_liveness)
 */
static void get_shared_LClasses(MContext *mc) {

	int tid = mc->thread_id;

	//lock my shared expiration class
	lock_shared_heap_class(tid);

	//for each possible lifetime
	int i;
	for (i = 0; i < mc->gopts->max_liveness; ++i) {
		//append shared LClass to local LClass
		LClass *remote = &shared_heap_classes[tid][i];
		LClass *local = heap_class_get_LClass(mc, mc->heap_class, i);
		if (remote->first == NULL) continue; //no shared LSCLasses here
		if (local->first == NULL) {
			local->first = remote->first;
			local->last = remote->last;
		} else {
			local->last->next = remote->first;
			local->last = remote->last;
		}
		remote->first = NULL;
		remote->last = NULL;
	}
	unlock_shared_heap_class(mc->thread_id);
}

/*
 * special case of ACDC to run false-sharing mode
 */
/*
static volatile LSClass *fs_collection = NULL;
static volatile int fs_collection_bytes;
static volatile int fs_allocation_thread;
static volatile int fs_deallocation_thread;
static void *false_sharing_thread(void *ptr) {

	MContext *mc = (MContext*)ptr;
	unsigned long time_counter = 0;
	int runs = 0;
	unsigned long long allocation_start, allocation_end;
	unsigned long long deallocation_start, deallocation_end;
	unsigned long long access_start, access_end;

	int local_fs_collection_bytes;

	printf("running thread %d\n", mc->thread_id);

	mc->stat->running_time = rdtsc();

	while (runs < mc->gopts->benchmark_duration) {

		size_t sz = 0;
		unsigned int lt;
		unsigned int num_objects;
		lifetime_size_class_type tp;
		ReferenceMap reference_map;
	
		//one thread allocates and tells the others how much it allocated
		//who allocates?
		if (mc->thread_id == 0) {
			fs_allocation_thread = get_random_thread(mc);
		}
		spin_barrier_wait(&false_sharing_barrier);

		if (mc->thread_id == fs_allocation_thread) {
		
			get_random_object_props(mc, &sz, &lt, &num_objects, &tp, &reference_map);
			// for false sharing we only use num_threads objects for one time slot
			tp = FALSE_SHARING;
			num_objects = mc->gopts->num_threads;
			lt = 1;
#ifdef OPTIMAL_MODE
			tp = OPTIMAL_FALSE_SHARING;
#endif		
			sz = sizeof(SharedObject) + 2;
		
			//mc->stat->lt_histogram[lt] += num_objects;
			//mc->stat->sz_histogram[get_sizeclass(sz)] += num_objects;
		
			allocation_start = rdtsc();
			fs_collection = 
				allocate_LSClass(mc, tp, sz, num_objects, &reference_map);

			//fs_collection->reference_map = reference_map;

			allocation_end = rdtsc();
			mc->stat->allocation_time += allocation_end - allocation_start;

			fs_collection_bytes = num_objects * sz;
		
		}
		spin_barrier_wait(&false_sharing_barrier);

		//all theads access the fs collection
		access_start = rdtsc();
		traverse_LSClass(mc, (LSClass*)fs_collection);
		access_end = rdtsc();
		mc->stat->access_time += access_end - access_start;
		
		local_fs_collection_bytes = fs_collection_bytes;
	
		time_counter += local_fs_collection_bytes;

		//fs collections only last for one time period
		if (mc->thread_id == 0) {
			fs_deallocation_thread = get_random_thread(mc);
		}
		spin_barrier_wait(&false_sharing_barrier);

		if (mc->thread_id == fs_deallocation_thread) {
                        //TODO(martin): check if next 2 lines are necessary
			//LSClass *old_c = (LSClass*)fs_collection;
			//old_c->reference_map = 0;
			deallocation_start = rdtsc();
			deallocate_LSClass(mc, (LSClass*)fs_collection);
			deallocation_end = rdtsc();
			mc->stat->deallocation_time += 
				deallocation_end - deallocation_start;
		}
		spin_barrier_wait(&false_sharing_barrier);

		mc->time++;
		runs++;
		print_runtime_stats(mc);
	}

	mc->stat->running_time = rdtsc() - mc->stat->running_time;

	return (void*)mc;
}*/

/*
 * ACDC main loop
 */
static void *acdc_thread(void *ptr) {
	MContext *mc = (MContext*)ptr;

	unsigned long time_counter = 0;
	int runs = 0;
	unsigned long long allocation_start, allocation_end;
	unsigned long long access_start, access_end;

	//printf("running thread %d\n", mc->thread_id);

	//start benchmark together
        int r = pthread_barrier_wait(&acdc_barrier);
        if (!( r == 0 || r == PTHREAD_BARRIER_SERIAL_THREAD )) {
                printf("pthread_barrier_wait: %d\n", r);
                exit(r);
        }

	mc->stat->running_time = rdtsc();

	while (runs < mc->gopts->benchmark_duration) {

		size_t sz = 0;
		unsigned int liveness;
		unsigned int num_objects;
		lifetime_size_class_type tp;

		get_random_object_props(mc, &sz, &liveness, &num_objects, &tp);

		if (mc->gopts->fixed_number_of_objects > 0) {
			//override random properties
			num_objects = mc->gopts->fixed_number_of_objects;
			sz = 1UL << mc->gopts->min_object_sc;
			liveness = mc->gopts->min_liveness;
		}

		//check if collections can be built with sz
		
		if (sz < (sizeof(BTObject)))
			sz = sizeof(BTObject);
		//if (tp == LIST && sz < (sizeof(LObject)))
		//	sz = sizeof(LObject);

		mc->stat->lt_histogram[liveness] += num_objects;
		mc->stat->sz_histogram[get_sizeclass(sz)] += num_objects;


#ifdef OPTIMAL_MODE
		if (tp == LIST) tp = OPTIMAL_LIST;
		if (tp == BTREE) tp = OPTIMAL_BTREE;
#endif
					
		allocation_start = rdtsc();
		LSClass *c = 
			allocate_LSClass(mc, tp, sz, num_objects);

		c->lifetime = liveness + mc->gopts->deallocation_delay;

		allocation_end = rdtsc();
		mc->stat->allocation_time += allocation_end - allocation_start;

		debug(mc, "created collection %p with liveness %d", c, liveness);

		if (mc->gopts->shared_objects && get_sharing_dist(mc)) {
                        //printf("This i will share\n");
			share_LSClass(mc, c);
			get_shared_LClasses(mc);
		} else {
			//bypass sharing pool
                        //printf("This i will NOT share\n");
                        c->reference_counter = 1;
			heap_class_insert(mc, mc->heap_class, c);
		}

		access_start = rdtsc();
		access_live_LClasses(mc);
		access_end = rdtsc();
		mc->stat->access_time += access_end - access_start;
		
		time_counter += sz * num_objects;

		if (time_counter >= mc->gopts->time_quantum) {

			if (mc->thread_id == 0) {
				get_and_print_memstats(mc);
			}

			//proceed in time
			mc->time++;
			time_counter = 0;
			runs++;

			//remove the just-expired LClass
			heap_class_remove(mc, mc->heap_class);	

			print_runtime_stats(mc);

                        //take care of time gap
			if ((mc->time % mc->gopts->max_time_gap) == 0) {
                                r = pthread_barrier_wait(&acdc_barrier);
                                if (!( r == 0 || r == PTHREAD_BARRIER_SERIAL_THREAD )) {
                                        printf("pthread_barrier_wait: %d\n", r);
                                        exit(r);
                                }
                        }
		}
	}

	mc->stat->running_time = rdtsc() - mc->stat->running_time;

	return (void*)mc;
}

/*
 * setup benchmark system and spawn ACDC threads
 */
void run_acdc(GOptions *gopts) {

	int i, r;
	struct timeval start, end, elapsed;
	pthread_t *threads = malloc_meta(sizeof(pthread_t) * gopts->num_threads);
	MContext **thread_data = malloc_meta(sizeof(MContext*) * gopts->num_threads);
	MContext **thread_results = malloc_meta(sizeof(MContext*) * gopts->num_threads);
	int thread_0_index = 0;

	//distribution_pools = create_collection_pools(gopts->max_liveness + 1);
	
	//allocate shared data here. 
	//init everything per-thread in create_mutator_context
	shared_heap_classes = calloc_meta(gopts->num_threads, sizeof(LClass*));
	shared_heap_classes_locks = calloc_meta(gopts->num_threads, 
			sizeof(pthread_mutex_t));

	r = spin_barrier_init(&false_sharing_barrier, gopts->num_threads);
        r = pthread_barrier_init(&acdc_barrier, NULL, gopts->num_threads);
        if (r != 0) {
                printf("pthread_barrier_init: %d\n", r);
                exit(r);
        }

	void *(*thread_function)(void*);

	if (gopts->mode == ACDC) {
		thread_function = acdc_thread;
	} else if (gopts->mode == FS) {
		//thread_function = false_sharing_thread;
		printf("Mode not supported right now... we are working on it\n");
		exit(EXIT_FAILURE);
	} else {
		printf("Mode not supported\n");
		exit(EXIT_FAILURE);
	}	
	
	for (i = 0; i < gopts->num_threads; ++i) {
                //printf("Creating metadata heap for thread %d\n", i);
		thread_data[i] = create_mutator_context(gopts, i);
	}
        //printf(" DONE\n");

	gettimeofday(&start, NULL);

	for (i = 0; i < gopts->num_threads; ++i) {
		r = pthread_create(&threads[i], NULL, thread_function, (void*)thread_data[i]);
		if (r) {
			printf("Unable to create thread_function: %d\n", r);
			exit(EXIT_FAILURE);
		}
	}

	for (i = 0; i < gopts->num_threads; ++i) {
		r = pthread_join(threads[i], (void*)(&thread_results[i]));
		if (r) {
			printf("Unable to join thread_function: %d\n", r);
			exit(EXIT_FAILURE);
		}
		if (thread_results[i]->thread_id == 0)
			thread_0_index = i;
	}

	gettimeofday(&end, NULL);

	//aggreagate info in first thread's MContext
	int j;
	for (i = 1; i < gopts->num_threads; ++i) {
		MContext *res = thread_results[i];

		thread_results[0]->stat->running_time +=
			res->stat->running_time;
		thread_results[0]->stat->allocation_time +=
			res->stat->allocation_time;
		thread_results[0]->stat->deallocation_time +=
			res->stat->deallocation_time;
		thread_results[0]->stat->access_time +=
			res->stat->access_time;
		thread_results[0]->stat->bytes_allocated += 
			res->stat->bytes_allocated;
		thread_results[0]->stat->bytes_deallocated += 
			res->stat->bytes_deallocated;
		thread_results[0]->stat->objects_allocated += 
			res->stat->objects_allocated;
		thread_results[0]->stat->objects_deallocated += 
			res->stat->objects_deallocated;
		for (j = 0; j < gopts->max_liveness; ++j) {
			thread_results[0]->stat->lt_histogram[j] +=
				res->stat->lt_histogram[j];
		}
		for (j = 0; j < gopts->max_object_sc; ++j) {
			thread_results[0]->stat->sz_histogram[j] +=
				res->stat->sz_histogram[j];
		}
	}

	printf("Lifetime Histogram:\tlifetime\tnum_objects\n");
	for (i = 0; i <= gopts->max_liveness; ++i) {
		printf("LT_HISTO:\t%d\t%lu\n", 
				i, 
				thread_results[0]->stat->lt_histogram[i]
				);
	}
	printf("Size-class Histogram:\tsize-class(2^n)\tnum_objects\n");
	for (i = 0; i <= gopts->max_object_sc; ++i) {
		printf("SZ_HISTO:\t%d\t%lu\n", 
				i, 
				thread_results[0]->stat->sz_histogram[i]
				);
	}

	printf("\nTime is given in CPU cycles\n\n");
	printf("RESULTS\tallocator\tnum_threads\trunning_time\trunning_time_in_percent\tallocation_time\tallocation_time_in_percent\tdeallocation_time\tdeallocation_time_in_percent\taccess_time\taccess_time_in_percent\n");
	printf("RUNTIME\t%s\t%d\t%llu\t%3.1f%% \t%llu \t%3.1f%% \t%llu \t%3.1f%% \t%llu \t%3.1f%%\n\n", 
			gopts->allocator_name,
			gopts->num_threads,
			thread_results[0]->stat->running_time, 
			100.0,
			thread_results[0]->stat->allocation_time,
			((double)thread_results[0]->stat->allocation_time /
			 (double)thread_results[0]->stat->running_time)*100.0,
			thread_results[0]->stat->deallocation_time,
			((double)thread_results[0]->stat->deallocation_time /
			 (double)thread_results[0]->stat->running_time)*100.0,
			thread_results[0]->stat->access_time,
			((double)thread_results[0]->stat->access_time /
			 (double)thread_results[0]->stat->running_time)*100.0
			);
	elapsed.tv_sec = end.tv_sec - start.tv_sec;
	elapsed.tv_usec = end.tv_usec - start.tv_usec;
	unsigned long time_in_ms = (elapsed.tv_sec * 1000000 + elapsed.tv_usec)/1000;

	printf("WALLCLOCK [ms]\t%lu\n\n", time_in_ms);

	//update_proc_status(gopts->pid);
        size_t avg_rss = 0;
        if (gopts->do_metadata_warmup) {
	        avg_rss = (thread_results[thread_0_index]->stat->resident_set_size_counter / 
			(gopts->benchmark_duration - 2 * gopts->max_liveness))
		       	- gopts->metadata_heap_sz;	/* warmup*/
        } else {
	        avg_rss = (thread_results[thread_0_index]->stat->resident_set_size_counter / 
			(gopts->benchmark_duration - 2 * gopts->max_liveness));
        }
	printf("MEM-RESULTS\tallocator\tnum_threads\tVM_PEAK\tRSS_HWM\tRSS_AVG (after warmup)\n");
	printf("MEMORY\t%s\t%d\t%ld\t%ld\t%ld\n\n",
			gopts->allocator_name,
			gopts->num_threads,
			thread_results[thread_0_index]->stat->vm_peak, 
			thread_results[thread_0_index]->stat->rss_hwm,
                        avg_rss
			);


	//TODO: free mutator context. Benchmark terminates anyways
	for (i = 0; i < gopts->num_threads; ++i) {
		//destroy_mutator_context(thread_results[i]);
		pthread_mutex_destroy(&shared_heap_classes_locks[i]);
	}
}

