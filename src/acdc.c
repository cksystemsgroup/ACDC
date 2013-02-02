/*
 * Copyright (c) 2012, the ACDC Project Authors.
 * All rights reserved. Please see the AUTHORS file for details.
 * Use of this source code is governed by a BSD license that
 * can be found in the LICENSE file.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <pthread.h>
#include <glib.h>
#include <assert.h>

#include "acdc.h"
#include "arch.h"
#include "memory.h"
#include "barrier.h"
#include "proc_status.h"

#define QUOTE(name) #name
#define STR(macro) QUOTE(macro)
#define ALLOCATOR_NAME STR(ALLOCATOR)

#define debug(...) _debug(__FILE__, __LINE__, __VA_ARGS__)


LClass ***shared_expiration_classes; //one expiration class per thread
pthread_mutex_t *shared_expiration_classes_locks; //one lock per expiration class
volatile spin_barrier_t false_sharing_barrier;
volatile spin_barrier_t acdc_barrier;
static __thread MContext *my_mc; //TODO: refactor in _debug call
pthread_mutex_t debug_lock = PTHREAD_MUTEX_INITIALIZER;


static void unreference_and_deallocate_LSClass(MContext *mc, LSClass *c);


void _debug(char *filename, int linenum, const char *format, ...) {
	if (my_mc->gopts->verbosity < 2) return;
	va_list args;
	pthread_mutex_lock(&debug_lock);
	fprintf(stdout, "[Debug] %s %4d T%2d @ %4d ", 
			filename,
			linenum,
			my_mc->thread_id, 
			my_mc->time);
	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);
	fprintf(stdout, "\n");
	pthread_mutex_unlock(&debug_lock);
}

inline void set_bit(u_int64_t *word, int bitpos) {
	*word |= (1 << bitpos);
}
inline void unset_bit(u_int64_t *word, int bitpos) {
	*word &= ~(1 << bitpos);
}


/*
 * an expiration class is an array of LClass pointer
 * one LClass for each lifetime
 */
static LClass **allocate_expiration_class(unsigned int max_lifetime) {

	LClass **ec = calloc(max_lifetime, sizeof(LClass*));
	//calloc creates zeroed memory, i.e., the first and last
	//pointers of each LClass are NULL
}

//Lifetime class API (doubly linked list handling, basically)
static void lclass_insert_after(LClass *list, LSClass *after, LSClass *c) {
	c->prev = after;
	c->next = after->next;
	if (after->next == NULL)
		list->last = c;
	else
		after->next->prev = c;
	after->next = c;
}

static void lclass_insert_before(LClass *list, LSClass *before, LSClass *c) {
	c->prev = before->prev;
	c->next = before;
	if (before->prev == NULL)
		list->first = c;
	else
		before->prev->next = c;
	before->prev = c;
}

static void lclass_insert_beginning(LClass *list, LSClass *c) {
	if (list->first == NULL) {
		list->first = c;
		list->last = c;
		c->prev = NULL;
		c->next = NULL;
	} else {
		lclass_insert_before(list, list->first, c);
	}
}

static void lclass_insert_end(LClass *list, LSClass *c) {
	if (list->last == NULL)
		lclass_insert_beginning(list, c);
	else
		lclass_insert_after(list, list->last, c);
}

static void lclass_remove(LClass *list, LSClass *c) {
	if (c->prev == NULL)
		list->first = c->next;
	else
		c->prev->next = c->next;
	if (c->next == NULL)
		list->last = c->prev;
	else
		c->next->prev = c->prev;
}

//Expiration class API
static void expiration_class_insert(MContext *mc, LClass **expiration_class, 
		LSClass *c) {

	unsigned int insert_index = (mc->time + c->lifetime) % 
			(mc->gopts->max_lifetime + 
			 mc->gopts->deallocation_delay);


	LClass *target_lifetime_class = expiration_class[insert_index];

	lclass_insert_end(target_lifetime_class, c);
}

static LClass *expiration_class_get_LClass(MContext *mc, LClass **expiration_class,
		unsigned int remaining_lifetime) {

	assert(remaining_lifetime < mc->gopts->max_lifetime);

	int index = (mc->time + remaining_lifetime) % 
		(mc->gopts->max_lifetime + mc->gopts->deallocation_delay);
	return expiration_class[index];
}

static void expiration_class_remove(MContext *mc, LClass **expiration_class) {

	int delete_index = (mc->time) % 
		(mc->gopts->max_lifetime + mc->gopts->deallocation_delay);
	delete_index -= mc->gopts->deallocation_delay;
	if (delete_index < 0)
		delete_index += (mc->gopts->max_lifetime +
				mc->gopts->deallocation_delay);

	LClass *expired_lifetime_class = expiration_class[delete_index];

	//forall LSClasses in this LClass: 
	//         decrement reference map and maybe deallocate LSClass
	LSClass *iterator = expired_lifetime_class->first;
	while (iterator != NULL) {
		unreference_and_deallocate_LSClass(mc, iterator);
		iterator = iterator->next;
	}
	//reset list
	expired_lifetime_class->first = NULL;
	expired_lifetime_class->last = NULL;
}


//Mutator specific data
static MContext *create_mutator_context(GOptions *gopts, unsigned int thread_id) {
	
	unsigned int seed = gopts->seed + thread_id;

	MContext *mc = malloc(sizeof(MContext));
	mc->gopts = gopts;
	mc->time = 0;
	
	mc->opt.rand = init_rand(seed); //TODO: remove glib dependency
	mc->thread_id = thread_id;

	mc->stat = malloc(sizeof(MStat));
	mc->stat->running_time = 0;
	mc->stat->allocation_time = 0;
	mc->stat->deallocation_time = 0;
	mc->stat->access_time = 0;
	mc->stat->bytes_allocated = 0;
	mc->stat->bytes_deallocated = 0;
	mc->stat->objects_allocated = 0;
	mc->stat->objects_deallocated = 0;
	mc->stat->lt_histogram = calloc(gopts->max_lifetime + 1, 
			sizeof(unsigned long));
	mc->stat->sz_histogram = calloc(gopts->max_object_sc + 1, 
			sizeof(unsigned long));

	mc->expiration_class = allocate_expiration_class(
			gopts->max_lifetime + gopts->deallocation_delay);

	shared_expiration_classes[thread_id] = allocate_expiration_class(
			gopts->max_lifetime); 
	//no deallocation delay necessary here, we only distribute LSClasses 
	//with lifetimes ranging from 1 to max_lifetime
	
	int r = pthread_mutex_init(&shared_expiration_classes_locks[thread_id], NULL);
	if (r != 0) {
		printf("unable to init mutex: %d\n", r);
		exit(EXIT_FAILURE);
	}

	return mc;
}

static void destroy_mutator_context(MContext *mc) {
	free_rand(mc->opt.rand);
	free(mc->stat->lt_histogram);
	free(mc->stat->sz_histogram);
	free(mc->stat);
	free(mc->expiration_class);
	free(shared_expiration_classes[mc->thread_id]);
	pthread_mutex_destroy(&shared_expiration_classes_locks[mc->thread_id]);
	free(mc);
}

static void get_and_print_memstats(MContext *mc) {

	//warmup phase: start memory measurements after max-lifetime
	//units of time
	if (mc->time < 2 * mc->gopts->max_lifetime) return;

	update_proc_status(mc->gopts->pid);
	mc->stat->current_rss = get_resident_set_size();
	mc->stat->resident_set_size_counter +=
		mc->stat->current_rss;
	mc->stat->vm_peak = get_vm_peak();
	mc->stat->rss_hwm = get_high_water_mark();

	if (mc->gopts->verbosity == 0) return;

	printf("MEMSTATS\t%s\t%3u\t%4u\t%12lu\n",
			ALLOCATOR_NAME,
			mc->thread_id,
			mc->time,
			mc->stat->current_rss
	      );

}

static void print_runtime_stats(MContext *mc) {

	if (mc->gopts->verbosity == 0) return;

	printf("STATS\t%s\t%3u\t%4u\t%12lu\t%12lu\t%12lu\t%12lu\n",
			ALLOCATOR_NAME,
			mc->thread_id,
			mc->time,
			mc->stat->bytes_allocated,
			mc->stat->bytes_deallocated,
			mc->stat->objects_allocated,
			mc->stat->objects_deallocated
		);
}


static void unreference_and_deallocate_LSClass(MContext *mc, LSClass *c) {

	assert((c->sharing_map & (1 << mc->thread_id)) != 0);
	//I'm allowed to be here
	
	//unset reference bit and check if we can deallocate the class
	while (1) {
		//I got a reference
		assert((c->reference_map & (1 << mc->thread_id )) != 0 );

		//atomically unset my bit
		u_int64_t old_rm = c->reference_map;
		u_int64_t new_rm = old_rm;
		unset_bit(&new_rm, mc->thread_id);
		assert(__builtin_popcountl(new_rm) == 
						__builtin_popcountl(old_rm) - 1);

		if (__sync_bool_compare_and_swap(
					&c->reference_map, old_rm, new_rm)) {
			//worked
			//if (c->reference_map == 0 && c->sharing_map == 0) {
			if (c->reference_map) {
				deallocate_collection(mc, (LSClass*)c);
				debug("deleted %p", c);
			} else {
				//someone else will deallocate
				assert((c->reference_map & (1 << mc->thread_id)) == 0);
				if (c->reference_map == 0) {
					debug("%p still in distribution for %d others", 
							c,
							__builtin_popcountl(c->sharing_map)
					     );
				} else {
					debug("%p can be deleted by %d others", c,
						__builtin_popcountl(c->reference_map)
						);
				}
			}
			break;
		} else {
			//some other thread changed the reference mask. retry
		}
	}
}

//runs in O(number of live objects)
static void access_live_LClasses(MContext *mc) {

	if (mc->gopts->skip_traversal == 1) return;

	int i, idx;
	for (i = mc->time; i < mc->time + mc->gopts->max_lifetime; ++i) {
		
		LClass *lc = expiration_class_get_LClass(mc, mc->expiration_class, i);

		//traverse all LSClasses in lc
		LSClass *iterator = lc->first;
		while (iterator != NULL) {
			traverse_LSClass(mc, iterator);
			iterator = iterator->next;
		}
	}
}

static void lock_shared_expiration_class(int thread_id) {
	pthread_mutex_lock(&shared_expiration_classes_locks[thread_id]);
}
static void unlock_shared_expiration_class(int thread_id) {
	pthread_mutex_unlock(&shared_expiration_classes_locks[thread_id]);
}

// runs in O(number of threads)
static void share_LSClass(MContext *mc, LSClass *c) {

	assert(c->reference_map == 0);
	assert(__builtin_popcountl(c->sharing_map) <= mc->gopts->num_threads);

	//the threads specified in sharing_map will receive a 
	//reference to this LSClass. We have to prepare the reference map
	//accordingy. Later, when a thread wants to deallocate the LSClass,
	//it atomically removes its bit from the reference map.
	//The last thread to unset its bit can actually deallocate the LSCLass
	c->reference_map = c->sharing_map;
	//reference_map is volatile. We can start to distribute the reference to c
	//TODO: this should be doable with only sharing_map.

	int i;
	for (i = 0; i < mc->gopts->num_threads; ++i) {
		if (c->sharing_map & (1 << i)) {
			//thread i will share this LSCLass
			//it needs to get a reference
			LClass **expiration_class = shared_expiration_classes[i];
			//lock the shared expiration class of this thread
			lock_shared_expiration_class(i);
			//insert LSClass
			expiration_class_insert(mc, expiration_class, c);
			unlock_shared_expiration_class(i);
		}
	}
}



// returns the number of bytes of all LSClasss that we got from the distr. pool
// this method runs in O(max_lifetime)
int get_shared_LClasses(MContext *mc) {

	int tid = mc->thread_id;

	//lock my shared expiration class
	lock_shared_expiration_class(tid);

	//for each possible lifetime
	int i;
	for (i = 0; i < mc->gopts->max_lifetime; ++i) {
		//append shared LClass to local LClass
		LClass *remote = shared_expiration_classes[tid][i];
		LClass *local = expiration_class_get_LClass(mc, mc->expiration_class, i);
		if (remote->first == NULL) continue; //no shared LSCLasses here
		if (local->first == NULL) {
			local->first = remote->first;
			local->last = remote->last;
		} else {
			local->last->next = remote->first;
			local->last = remote->last;
		}
	}
	unlock_shared_expiration_class(mc->thread_id);
}

volatile LSClass *fs_collection = NULL;
volatile int fs_collection_bytes;
volatile int fs_allocation_thread;
volatile int fs_deallocation_thread;
void *false_sharing_thread(void *ptr) {

	MContext *mc = (MContext*)ptr;
	my_mc = mc;
	unsigned long time_counter = 0;
	int runs = 0;
	unsigned long long allocation_start, allocation_end;
	unsigned long long deallocation_start, deallocation_end;
	unsigned long long access_start, access_end;

	int local_fs_collection_bytes;

	printf("running thread %d\n", mc->opt.thread_id);

	mc->stat->running_time = rdtsc();

	//while (runs < mc->gopts->benchmark_duration && allocators_alive == 1) {
	while (runs < mc->gopts->benchmark_duration) {

		size_t sz = 0;
		unsigned int lt;
		unsigned int num_objects;
		collection_t tp;
		u_int64_t sharing_map;
	
		//one thread allocates and tells the others how much it allocated
		//sho allocates?
		if (mc->opt.thread_id == 0) {
			fs_allocation_thread = get_random_thread(mc);
		}
		spin_barrier_wait(&barrier);

		if (mc->opt.thread_id == fs_allocation_thread) {
		
			get_random_object_props(mc, &sz, &lt, &num_objects, &tp, &sharing_map);
			// for false sharing we only use num_threads objects for one time slot
			tp = FALSE_SHARING;
			num_objects = mc->gopts->num_threads;
			lt = 1;
#ifdef OPTIMAL_MODE
			tp = OPTIMAL_FALSE_SHARING;
#endif		
			if (sz < sizeof(SharedObject))
				sz = sizeof(SharedObject) + 2;
		
			mc->stat->lt_histogram[lt] += num_objects;
			mc->stat->sz_histogram[get_sizeclass(sz)] += num_objects;
		
			allocation_start = rdtsc();
			fs_collection = 
				allocate_collection(mc, tp, sz, num_objects, sharing_map);

			fs_collection->reference_map = sharing_map;

			allocation_end = rdtsc();
			mc->stat->allocation_time += allocation_end - allocation_start;

			fs_collection_bytes = num_objects * sz;
		
		}
		spin_barrier_wait(&barrier);

		//all theads access the fs collection
		assert(fs_collection != NULL);
		access_start = rdtsc();
		traverse_collection(mc, (LSClass*)fs_collection);
		access_end = rdtsc();
		mc->stat->access_time += access_end - access_start;
		
		local_fs_collection_bytes = fs_collection_bytes;
	
		time_counter += local_fs_collection_bytes;

		//fs collections only last for one time period
		if (mc->opt.thread_id == 0) {
			fs_deallocation_thread = get_random_thread(mc);
		}
		spin_barrier_wait(&barrier);

		if (mc->opt.thread_id == fs_deallocation_thread) {
			LSClass *old_c = (LSClass*)fs_collection;
			old_c->reference_map = 0;
			deallocation_start = rdtsc();
			deallocate_collection(mc, (LSClass*)fs_collection);
			deallocation_end = rdtsc();
			mc->stat->deallocation_time += 
				deallocation_end - deallocation_start;
		}
		spin_barrier_wait(&barrier);

		mc->time++;
		runs++;
		print_mutator_stats(mc);
	}

	mc->stat->running_time = rdtsc() - mc->stat->running_time;

	return (void*)mc;
}

//int allocators_alive = 1;
void *acdc_thread(void *ptr) {
	MContext *mc = (MContext*)ptr;
	my_mc = mc;

	unsigned long time_counter = 0;
	int runs = 0;
	unsigned long long allocation_start, allocation_end;
	unsigned long long deallocation_start, deallocation_end;
	unsigned long long access_start, access_end;

	printf("running thread %d\n", mc->opt.thread_id);

	//start benchmark together
	spin_barrier_wait(&acdc_barrier);

	mc->stat->running_time = rdtsc();

	while (runs < mc->gopts->benchmark_duration) {

		size_t sz = 0;
		unsigned int lt;
		unsigned int num_objects;
		collection_t tp;
		u_int64_t sharing_map;

		get_random_object_props(mc, &sz, &lt, &num_objects, &tp, &sharing_map);

		//TODO: move to get_random_object...
		//check if collections can be built with sz
		if (tp == BTREE && sz < sizeof(BTObject))
			sz = sizeof(BTObject) + 4;
		if (tp == LIST && sz < sizeof(LObject))
			sz = sizeof(LObject) + 4;
		if (tp == FALSE_SHARING && sz < sizeof(SharedObject))
			sz = sizeof(SharedObject) + 4;

		mc->stat->lt_histogram[lt] += num_objects;
		mc->stat->sz_histogram[get_sizeclass(sz)] += num_objects;

#ifdef OPTIMAL_MODE
		if (tp == LIST) tp = OPTIMAL_LIST;
		if (tp == BTREE) tp = OPTIMAL_BTREE;
#endif
					
		allocation_start = rdtsc();
		LSClass *oc = 
			allocate_collection(mc, tp, sz, num_objects, sharing_map);

		allocation_end = rdtsc();
		mc->stat->allocation_time += allocation_end - allocation_start;

		assert(oc->sharing_map != 0);
		assert(oc->sharing_map == sharing_map);
		assert(oc->reference_map == 0);
		assert(__builtin_popcountl(oc->sharing_map) <= mc->gopts->num_threads);

		share_LSClass(mc, c);

		debug("created collection %p with lt %d", oc, lt);
		
		
		unsigned long bytes_from_dist_pool = 0;
		//while (bytes_from_dist_pool == 0 && allocators_alive == 1) {
		while (bytes_from_dist_pool == 0) {
			bytes_from_dist_pool += get_from_distribution_pool(mc);
		}

		access_start = rdtsc();
		access_live_LClasses(mc);
		access_end = rdtsc();
		mc->stat->access_time += access_end - access_start;
		
		//time_counter += bytes_from_dist_pool;
		time_counter += sz * num_objects;

		if (time_counter >= mc->gopts->time_threshold) {

			if (mc->opt.thread_id == 0) {
				get_and_print_memstats(mc);
			}

			//proceed in time
			mc->time++;
			time_counter = 0;
			runs++;

			deallocation_start = rdtsc();
			delete_expired_objects(mc);
			deallocation_end = rdtsc();

			mc->stat->deallocation_time += 
				deallocation_end - deallocation_start;

			print_mutator_stats(mc);

			if ((mc->time % mc->gopts->max_time_gap) == 0)
				spin_barrier_wait(&acdc_barrier);
		}
	}

	mc->stat->running_time = rdtsc() - mc->stat->running_time;

	return (void*)mc;
}

void run_acdc(GOptions *gopts) {

	int i, r;
	pthread_t *threads = malloc(sizeof(pthread_t) * gopts->num_threads);
	MContext **thread_results = malloc(sizeof(MContext*) * gopts->num_threads);
	int thread_0_index = 0;

	//distribution_pools = create_collection_pools(gopts->max_lifetime + 1);
	
	//allocate shared data here. 
	//init everything per-thread in create_mutator_context
	shared_expiration_classes = calloc(gopts->num_threads, sizeof(LClass**));
	shared_expiration_classes_locks = calloc(gopts->num_threads, 
			sizeof(pthread_mutex_t));

	r = spin_barrier_init(&barrier, gopts->num_threads);
	r = spin_barrier_init(&acdc_barrier, gopts->num_threads);

	void *(*thread_function)(void*);

	if (gopts->mode == ACDC) {
		thread_function = acdc_thread;
	} else if (gopts->mode == FS) {
		thread_function = false_sharing_thread;
	} else {
		printf("Mode not supported\n");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < gopts->num_threads; ++i) {
		MContext *mc = create_mutator_context(gopts, i);
		r = pthread_create(&threads[i], NULL, thread_function, (void*)mc);
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
		if (thread_results[i]->opt.thread_id == 0)
			thread_0_index = i;
	}

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
		for (j = 0; j < gopts->max_lifetime; ++j) {
			thread_results[0]->stat->lt_histogram[j] +=
				res->stat->lt_histogram[j];
		}
		for (j = 0; j < gopts->max_object_sc; ++j) {
			thread_results[0]->stat->sz_histogram[j] +=
				res->stat->sz_histogram[j];
		}
	}

	for (j = 0; j <= gopts->max_lifetime; ++j) {
		printf("LT_HISTO:\t%d\t%lu\n", 
				j, 
				thread_results[0]->stat->lt_histogram[j]
				);
	}
	for (j = 0; j <= gopts->max_object_sc; ++j) {
		printf("SZ_HISTO:\t%d\t%lu\n", 
				j, 
				thread_results[0]->stat->sz_histogram[j]
				);
	}

	printf("RUNTIME\t%s\t%d\t%llu\t%3.1f%% \t%llu \t%3.1f%% \t%llu \t%3.1f%% \t%llu \t%3.1f%%\n", 
			ALLOCATOR_NAME,
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

	//update_proc_status(gopts->pid);
	printf("MEMORY\t%s\t%d\t%ld\t%ld\t%ld\n",
			ALLOCATOR_NAME,
			gopts->num_threads,
			thread_results[thread_0_index]->stat->vm_peak, 
			thread_results[thread_0_index]->stat->rss_hwm, 
			thread_results[thread_0_index]->stat->resident_set_size_counter / 
			gopts->benchmark_duration
			);

	//TODO: free mutator context
}

