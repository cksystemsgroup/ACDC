#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <glib.h>

#include "acdc.h"
#include "distribution.h"
#include "memory.h"


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

MContext *create_mutator_context(GOptions *gopts, unsigned int thread_id) {
	
	unsigned int seed = gopts->seed + thread_id;

	MContext *mc = malloc(sizeof(MContext));
	mc->gopts = gopts;
	mc->time = 0;
	
	mc->opt.rand = init_rand(seed);
	mc->opt.thread_id = thread_id;

	mc->stat = malloc(sizeof(MStat));
	mc->stat->bytes_allocated = 0;
	mc->stat->bytes_deallocated = 0;
	mc->stat->objects_allocated = 0;
	mc->stat->objects_deallocated = 0;
	mc->stat->lt_histogram = calloc(gopts->max_lifetime, sizeof(unsigned long));
	mc->stat->sz_histogram = calloc(gopts->max_object_sc, sizeof(unsigned long));

	int num_pools = gopts->max_lifetime;
	mc->collection_pools = malloc(sizeof(CollectionPool) * num_pools);
		
	//setup CollectionPools
	int i;
	for (i = 0; i < num_pools; ++i) {
		//use int keys for the hash maps
		mc->collection_pools[i].collections = 
			g_hash_table_new(object_collection_hash, 
					object_collection_equal);

		mc->collection_pools[i].remaining_lifetime = 
			gopts->min_lifetime + i;
	}

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
	

	printf("STATS\t%u\t%u\t%lu\t%lu\t%lu\t%lu\n",
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
	
	//printf("%p %p \n", key, value);
	//printf("deallocate %u elements of size %u\n", oc->num_objects, 
	//		oc->object_size);

	//deallocate collection
	deallocate_collection(mc, oc);
}

void delete_expired_objects(MContext *mc) {

	int delete_index = (mc->time) % mc->gopts->max_lifetime;

	//printf("DEBUG: will remove from pool %d\n", delete_index);

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


void *acdc_thread(void *ptr) {
	MContext *mc = (MContext*)ptr;

	unsigned long time_counter = 0;
	int runs = 0;

	printf("running thread %d\n", mc->opt.thread_id);

	while (runs < mc->gopts->benchmark_duration) {
		size_t sz = 0;
		unsigned int lt;
		unsigned int num_objects;
		get_random_object_props(mc, &sz, &lt, &num_objects);

		mc->stat->lt_histogram[lt] += num_objects;
		mc->stat->sz_histogram[get_sizeclass(sz)] += num_objects;

		//allocate objects
		//create data structures
		
		//create trees and lists
		//printf("allocate list of %u elements of sz %lu lt %u\n", 
		//		num_objects, sz, lt);

		collection_t tp = LIST;
#ifdef OPTIMAL_MODE
		if (tp == LIST) tp = OPTIMAL_LIST;
#endif
		OCollection *c = allocate_collection(mc, tp, sz, num_objects);

		
		//get CollectionPool for lt
		unsigned int insert_index = (mc->time + lt) % 
			mc->gopts->max_lifetime;

		CollectionPool *cp = &(mc->collection_pools[insert_index]);
		cp->remaining_lifetime = lt; //FIXME: is this necessary?

		while (g_hash_table_contains(cp->collections,
					(gconstpointer)c)) {

			//printf("a collection for size %lu objects with "
			//		"id %u already exists\n",
			//		c->object_size,
			//		c->id);
			c->id++;
			
		}
		//add collection with proper id to hash map
		//g_hash_table_insert(cp->collections, (gpointer)c, (gpointer)c);
		g_hash_table_add(cp->collections, (gpointer)c);
		



		//TODO: access (all) objects
		//access objects that were allocated together
		access_live_objects(mc);


		time_counter += num_objects * sz;
		if (time_counter >= mc->gopts->time_threshold) {
			
			print_mutator_stats(mc);
		
			//access_live_objects(mc);

			//proceed in time
			mc->time++;
			time_counter = 0;
			runs++;

			delete_expired_objects(mc);
		}
	}


	return (void*)mc;
}


void run_acdc(GOptions *gopts) {

	int i, r;
	pthread_t *threads = malloc(sizeof(pthread_t) * gopts->num_threads);
	MContext **thread_results = malloc(sizeof(MContext*) * gopts->num_threads);
	for (i = 0; i < gopts->num_threads; ++i) {
		MContext *mc = create_mutator_context(gopts, i);
		r = pthread_create(&threads[i], NULL, acdc_thread, (void*)mc);
		if (r) {
			printf("Unable to create acdc_thread: %d\n", r);
			exit(EXIT_FAILURE);
		}
	}

	for (i = 0; i < gopts->num_threads; ++i) {
		r = pthread_join(threads[i], (void*)(&thread_results[i]));
		if (r) {
			printf("Unable to join acdc_thread: %d\n", r);
			exit(EXIT_FAILURE);
		}
	}



	//aggreagate info in thread 0's MContext
	int j, k;
	for (i = 1; i < gopts->num_threads; ++i) {
		MContext *res = thread_results[i];
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


	for (j = 0; j < gopts->max_lifetime; ++j) {
		printf("LT_HISTO:\t%d\t%lu\n", 
				j, 
				thread_results[0]->stat->lt_histogram[j]
				);
	}
	for (j = 0; j < gopts->max_object_sc; ++j) {
		printf("SZ_HISTO:\t%d\t%lu\n", 
				j, 
				thread_results[0]->stat->sz_histogram[j]
				);
	}
	
	
	//free mutator context
}

