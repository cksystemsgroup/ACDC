/*
 * Copyright (c) 2012, the ACDC Project Authors.
 * All rights reserved. Please see the AUTHORS file for details.
 * Use of this source code is governed by a BSD license that
 * can be found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <glib.h>
#include <assert.h>

#include "acdc.h"
#include "arch.h"
#include "memory.h"
#include "barrier.h"

#define QUOTE(name) #name
#define STR(macro) QUOTE(macro)
#define ALLOCATOR_NAME STR(ALLOCATOR)

CollectionPool *distribution_pools; // one CP per lifetime
pthread_mutex_t distribution_pools_lock = PTHREAD_MUTEX_INITIALIZER;

volatile spin_barrier_t barrier;


inline void set_bit(u_int64_t *word, int bitpos) {
	*word |= (1 << bitpos);
}
inline void unset_bit(u_int64_t *word, int bitpos) {
	*word &= ~(1 << bitpos);
}

guint object_collection_hash(gconstpointer key) {

	OCollection *oc = (OCollection*)key;
	unsigned long combined_key = oc->object_size << 32;
	combined_key |= oc->id;
	return g_int64_hash((gconstpointer)&combined_key);
}

gboolean object_collection_equal(gconstpointer a, gconstpointer b) {

	OCollection *ac = (OCollection*)a;
	OCollection *bc = (OCollection*)b;

	return (ac->object_size == bc->object_size &&
			ac->id == bc->id);
}

CollectionPool *create_collection_pools(int num_pools) {

	CollectionPool *collection_pools = calloc(num_pools, sizeof(CollectionPool));
		
	//setup CollectionPools
	int i;
	for (i = 0; i < num_pools; ++i) {
		//use int keys for the hash maps
		collection_pools[i].collections = 
			g_hash_table_new(object_collection_hash, 
					object_collection_equal);


		//collection_pools[i].remaining_lifetime = 
		//	gopts->min_lifetime + i;
	}
	return collection_pools;
}

MContext *create_mutator_context(GOptions *gopts, unsigned int thread_id) {
	
	unsigned int seed = gopts->seed + thread_id;

	MContext *mc = malloc(sizeof(MContext));
	mc->gopts = gopts;
	mc->time = 0;
	
	mc->opt.rand = init_rand(seed);
	mc->opt.thread_id = thread_id;

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

	//int num_pools = gopts->max_lifetime;
	//
	mc->collection_pools = create_collection_pools(gopts->max_lifetime);

	return mc;
}

void destroy_mutator_context(MContext *mc) {

	free_rand(mc->opt.rand);
	free(mc->stat->lt_histogram);
	free(mc->stat->sz_histogram);
	free(mc->stat);
	free(mc);
}


void print_mutator_stats(MContext *mc) {

	printf("STATS\t%s\t%3u\t%4u\t%12lu\t%12lu\t%12lu\t%12lu\n",
			ALLOCATOR_NAME,
			mc->opt.thread_id,
			mc->time,
			mc->stat->bytes_allocated,
			mc->stat->bytes_deallocated,
			mc->stat->objects_allocated,
			mc->stat->objects_deallocated
			);
}

void delete_collection(gpointer key, gpointer value, gpointer user_data) {

	OCollection *oc = (OCollection*)value;
	MContext *mc = (MContext*)user_data;
	
	//shared Collections
	//unset reference bit and check if we can deallocate the collection
	while (1) {
		assert(oc->reference_map != 0);
		assert((oc->reference_map & (1 << mc->opt.thread_id )) != 0 );

		u_int64_t old_rm = oc->reference_map;
		//unset my bit
		u_int64_t new_rm = old_rm;
		unset_bit(&new_rm, mc->opt.thread_id);
		assert(__builtin_popcountl(new_rm) == 
						__builtin_popcountl(old_rm) - 1);

		if (__sync_bool_compare_and_swap(
					&oc->reference_map, old_rm, new_rm)) {
			//worked
			if (oc->reference_map == 0 && oc->sharing_map == 0) {
				deallocate_collection(mc, oc);
			} else {
				//someone else will deallocate
				assert((oc->reference_map & (1<<mc->opt.thread_id))
					       	== 0);
			}
			break;
		} else {
			//some other thread changed the reference mask
		}
	}
}

void delete_expired_objects(MContext *mc) {

	int delete_index = (mc->time) % mc->gopts->max_lifetime;
	
	CollectionPool *cp = &(mc->collection_pools[delete_index]);
	g_hash_table_foreach(cp->collections, delete_collection, mc);
	//delete items in hashmap
	g_hash_table_remove_all(cp->collections);
}

static void access_collection(gpointer key, gpointer value, gpointer user_data) {

	OCollection *oc = (OCollection*)value;
	MContext *mc = (MContext*)user_data;

	traverse_collection(mc, oc);
}

void access_live_objects(MContext *mc) {

	int i, idx;
	for (i = mc->time; i < mc->time + mc->gopts->max_lifetime; ++i) {
		idx = i % mc->gopts->max_lifetime;
		//printf("access objects from pool %d\n", idx);
		
		CollectionPool *cp = &(mc->collection_pools[idx]);
		g_hash_table_foreach(cp->collections, access_collection, mc);

	}
}

void add_collection_to_pool(OCollection *oc, CollectionPool *cp) {

	//make sure we have a unique key for OCollection objects
	while (g_hash_table_contains(cp->collections,
				(gconstpointer)oc)) {
		oc->id++;
	}
	//add collection with proper id to hash map (key == value)
	g_hash_table_add(cp->collections, (gpointer)oc);
}

void add_to_distribution_pool(MContext *mc, OCollection *oc, int lt) {
	
	CollectionPool *cp = &distribution_pools[lt];

	pthread_mutex_lock(&distribution_pools_lock);

	int before = g_hash_table_size(cp->collections);
	add_collection_to_pool(oc, cp);
	int after = g_hash_table_size(cp->collections);
	assert(after == before + 1);
	
	pthread_mutex_unlock(&distribution_pools_lock);
}

struct gfdc_args {
	MContext *mc;
	int lt;
	int count;
};
gboolean get_from_distribution_collection(gpointer key, gpointer value, gpointer user_data) {

	OCollection *oc = (OCollection*)value;
	struct gfdc_args *args = (struct gfdc_args*)user_data;

	u_int64_t my_bit = 1 << args->mc->opt.thread_id;

	//check if I should get this OCollection
	if (!(oc->sharing_map & my_bit)) return FALSE; //my bit is not set. i'm not involved

	if (oc->reference_map & my_bit) return FALSE; //i already have a reference
	
	//set my bit in reference mask
	while (1) {
		u_int64_t old_rm = oc->reference_map;
		u_int64_t new_rm = old_rm;
		set_bit(&new_rm, args->mc->opt.thread_id);
	
		if (__sync_bool_compare_and_swap(
					&oc->reference_map, old_rm, new_rm)) {
			//worked
			assert((oc->reference_map & (1<<args->mc->opt.thread_id)) != 0);
			break;
		} else {
			//some other thread changed the reference mask
		}
	}
	//unset my bit in the sharing map
	while (1) {
		u_int64_t old_sm = oc->sharing_map;
		u_int64_t new_sm = old_sm;
		unset_bit(&new_sm, args->mc->opt.thread_id);
	
		if (__sync_bool_compare_and_swap(
					&oc->sharing_map, old_sm, new_sm)) {
			//worked
			assert(__builtin_popcountl(oc->sharing_map) <
						__builtin_popcountl(old_sm));
			break;
		} else {
			//some other thread changed the reference mask, retry
		}
	}
	
	//insert in thread local CollectionPool for this lifetime
	unsigned int insert_index = (args->mc->time + args->lt) % 
			args->mc->gopts->max_lifetime;

	CollectionPool *target_pool = &args->mc->collection_pools[insert_index];
	add_collection_to_pool(oc, target_pool);
	args->count += (oc->num_objects * oc->object_size);

	// can we remove this OCollection from hash map?
	// check if all bits are cleared. not shared anymore
	if (oc->sharing_map == 0) return TRUE;
       	
	//others have to get a reference to oc first.
	//do not delete yet
	return FALSE;
}

// returns the number of bytes of all OCollections that we got from the distr. pool
int get_from_distribution_pool(MContext *mc) {

	struct gfdc_args args;
	args.mc = mc;
	args.count = 0;
	
	pthread_mutex_lock(&distribution_pools_lock);

	int lt;
	for (lt = 0; lt <= mc->gopts->max_lifetime; ++lt) {
		CollectionPool *cp = &distribution_pools[lt];
		args.lt = lt;
		g_hash_table_foreach_remove(cp->collections, 
				get_from_distribution_collection, (gpointer)&args);
	}
	
	pthread_mutex_unlock(&distribution_pools_lock);
	return args.count;
}

volatile OCollection *fs_collection = NULL;
volatile int fs_collection_bytes;
volatile int fs_allocation_thread;
volatile int fs_deallocation_thread;
void *false_sharing_thread(void *ptr) {

	MContext *mc = (MContext*)ptr;
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
		u_int64_t rctm;
	
		//one thread allocates and tells the others how much it allocated
		//sho allocates?
		if (mc->opt.thread_id == 0) {
			fs_allocation_thread = get_random_thread(mc);
		}
		spin_barrier_wait(&barrier);

		if (mc->opt.thread_id == fs_allocation_thread) {
		
			get_random_object_props(mc, &sz, &lt, &num_objects, &tp, &rctm);
			// for false sharing we only use num_threads objects for one time slot
			tp = FALSE_SHARING;
			num_objects = mc->gopts->num_threads;
			lt = 1;
#ifdef OPTIMAL_MODE
			tp = OPTIMAL_FALSE_SHARING;
#endif		
			if (sz < sizeof(Object))
				sz = sizeof(Object) + 4;
		
			mc->stat->lt_histogram[lt] += num_objects;
			mc->stat->sz_histogram[get_sizeclass(sz)] += num_objects;
		
			allocation_start = rdtsc();
			fs_collection = 
				allocate_collection(mc, tp, sz, num_objects, rctm);

			fs_collection->reference_map = rctm;

			allocation_end = rdtsc();
			mc->stat->allocation_time += allocation_end - allocation_start;

			fs_collection_bytes = num_objects * sz;
		
		}
		spin_barrier_wait(&barrier);

		//all theads access the fs collection
		assert(fs_collection != NULL);
		access_start = rdtsc();
		traverse_collection(mc, (OCollection*)fs_collection);
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
			OCollection *old_c = (OCollection*)fs_collection;
			old_c->reference_map = 0;
			deallocation_start = rdtsc();
			deallocate_collection(mc, (OCollection*)fs_collection);
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

	unsigned long time_counter = 0;
	int runs = 0;
	unsigned long long allocation_start, allocation_end;
	unsigned long long deallocation_start, deallocation_end;
	unsigned long long access_start, access_end;

	printf("running thread %d\n", mc->opt.thread_id);

	mc->stat->running_time = rdtsc();

	while (runs < mc->gopts->benchmark_duration) {

		size_t sz = 0;
		unsigned int lt;
		unsigned int num_objects;
		collection_t tp;
		u_int64_t rctm;

		get_random_object_props(mc, &sz, &lt, &num_objects, &tp, &rctm);

		//TODO: move to get_random_object...
		//check if collections can be built with sz
		if (tp == BTREE && sz < sizeof(BTObject))
			sz = sizeof(BTObject) + 4;
		if (tp == LIST && sz < sizeof(LObject))
			sz = sizeof(LObject) + 4;
		if (tp == FALSE_SHARING && sz < sizeof(SharedObject))
			sz = sizeof(SharedObject) + 4;

		// for small false sharing we only use num_threads objects
		if (tp == FALSE_SHARING) 
			num_objects = __builtin_popcountl(rctm);
		
		mc->stat->lt_histogram[lt] += num_objects;
		mc->stat->sz_histogram[get_sizeclass(sz)] += num_objects;


#ifdef OPTIMAL_MODE
		if (tp == LIST) tp = OPTIMAL_LIST;
		if (tp == BTREE) tp = OPTIMAL_BTREE;
#endif
					
		allocation_start = rdtsc();
		OCollection *oc = 
			allocate_collection(mc, tp, sz, num_objects, rctm);

		allocation_end = rdtsc();
		mc->stat->allocation_time += allocation_end - allocation_start;

		assert(oc->sharing_map != 0);
		assert(oc->reference_map == 0);
		add_to_distribution_pool(mc, oc, lt);
		
		unsigned long bytes_from_dist_pool = 0;
		//while (bytes_from_dist_pool == 0 && allocators_alive == 1) {
		while (bytes_from_dist_pool == 0) {
			bytes_from_dist_pool += get_from_distribution_pool(mc);
		}

		//access (all) objects
		//access objects that were allocated together
		access_start = rdtsc();
		access_live_objects(mc);
		access_end = rdtsc();

		mc->stat->access_time += access_end - access_start;
		
		time_counter += bytes_from_dist_pool;

		if (time_counter >= mc->gopts->time_threshold) {
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
		}
	}

	mc->stat->running_time = rdtsc() - mc->stat->running_time;

	return (void*)mc;
}

void run_acdc(GOptions *gopts) {

	int i, r;
	pthread_t *threads = malloc(sizeof(pthread_t) * gopts->num_threads);
	MContext **thread_results = malloc(sizeof(MContext*) * gopts->num_threads);

	distribution_pools = create_collection_pools(gopts->max_lifetime + 1);

	r = spin_barrier_init(&barrier, gopts->num_threads);

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
	}

	//aggreagate info in thread 0's MContext
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
	//TODO: free mutator context
}

