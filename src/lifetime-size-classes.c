 /*
 * Copyright (c) 2012, the ACDC Project Authors.
 * All rights reserved. Please see the AUTHORS file for details.
 * Use of this source code is governed by a BSD license that
 * can be found in the LICENSE file.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <alloca.h>

#include "acdc.h"
#include "arch.h"
#include "caches.h"

static inline void write_object(Object *o, size_t size, size_t offset) {
	int i;
	size_t pl_sz = size - offset;
	//payload starts after header
	volatile char *payload = (char*)o + offset;
	
	for (i = 1; i < pl_sz; ++i) {
		payload[i] = payload[i-1] + 1;
	}
}

static LSClass *get_LSClass(MContext *mc) {
	LSCNode *node = mc->class_cache.first;
	if (node != NULL) {
		lclass_remove(&mc->class_cache, node);
		node->next = NULL;
		node->prev = NULL;
		return (LSClass*)node;
	}

	if (mc->class_buffer_counter >= mc->gopts->class_buffer_size) {
		printf("class buffer overflow. increase -C option\n");
		exit(EXIT_FAILURE);
	}
	
        uint32_t size = ((sizeof(LSClass) / L1_LINE_SZ) * (L1_LINE_SZ)) + L1_LINE_SZ;
        //printf("sizeof: %lu\n", sizeof(LSClass));
        //printf("size: %d\n", size);
        node = (LSCNode*)(((uint64_t)mc->class_buffer_memory) + 
		(mc->class_buffer_counter * size));
        
	mc->class_buffer_counter++;
        
        //init node
	node->next = NULL;
	node->prev = NULL;
        node->ls_class = NULL;

        LSClass* c = (LSClass*)node;
        return c;
}

static void recycle_LSClass(MContext *mc, LSClass *class) {
	assert(class->reference_counter == 0);

	//treat Classes as Nodes. They are big enough. Just cast
	LSCNode *n = (LSCNode*)class;
	lclass_insert_beginning(&mc->class_cache, n);
}

/*
static void get_thread_ids(int *thread_ids, ReferenceMap *reference_map) {
	int i, j;
        pthread_spin_lock(&reference_map->lock);
        //TODO(martin): iterate only to gopts->maxthreads
	for (i = 0, j = 0; i < MAX_NUM_THREADS; ++i) {
		if (get_bit(reference_map->thread_map, i) == 1) {
			thread_ids[j++] = i;
		}
	}
        pthread_spin_unlock(&reference_map->lock);
}
*/

/**
 * write_access_ratio determines how many objects are written. e.g., 
 * write_access_ratio of 20 means that every 5th element is written
 */
static int write_ith_element(MContext *mc, int i) {
	if (mc->gopts->write_access_ratio == 0) return 0;
	if (mc->gopts->write_access_ratio >= 100) return 1;
	int ith = 100 / mc->gopts->write_access_ratio;
	if ( i % ith == 0 ) return 1;
	return 0;
}

/**
 * allocates memory for a new LSCLass
 */
static LSClass *new_LSClass(MContext *mc, lifetime_size_class_type t, 
		size_t sz, unsigned long nelem) {

	LSClass *c = get_LSClass(mc);
	c->object_size = sz;
	c->num_objects = nelem;
	c->type = t;

	return c;
}
/*
// lifetime-size-class handling for false sharing ---------------------
static void assign_optimal_fs_pool_objects(MContext *mc, LSClass *c) {

	//check which threads should participate
	//int num_threads = map_bits(reference_map);
	int num_threads = reference_map->reference_count;
	int *thread_ids = alloca(num_threads * sizeof(int));
//	get_thread_ids(thread_ids, reference_map);

	int cache_lines_per_element = (c->object_size / L1_LINE_SZ) + 1;
	assert(cache_lines_per_element == 1);

	int i;
	for (i = 0; i < c->num_objects; ++i) {
		//TODO(martin): fix false sharing mode after reference_map upgrade
		//char *next = (char*)c->start + cache_lines_per_element * L1_LINE_SZ * i;
		//SharedObject *o = (SharedObject*)next;
                //o->reference_map = BIT_ZERO << ( thread_ids[i % num_threads] );
	}

	assert(i % num_threads == 0);
}
*/
/*
static void assign_fs_pool_objects(MContext *mc, LSClass *c) {

	//check which threads should participate
	//int num_threads = map_bits(reference_map);
	int num_threads = reference_map->reference_count;
	int *thread_ids = alloca(num_threads * sizeof(int));
//	get_thread_ids(thread_ids, reference_map);

	int i;
	for (i = 0; i < c->num_objects; ++i) {
		//TODO(martin): fix false sharing mode after reference_map upgrade
		//SharedObject *o = ((SharedObject**)c->start)[i];
		//o->reference_map = BIT_ZERO << ( thread_ids[i % num_threads]  );	
	}
}
*/
/*
static LSClass *allocate_fs_pool(MContext *mc, size_t sz, unsigned long nelem) {
	
	//int num_threads = map_bits(reference_map);
	int num_threads = reference_map->reference_count;
	//make sure that nelem is a multiple of num_threads
	if (nelem % num_threads != 0)
		nelem += num_threads - (nelem % num_threads);

	LSClass *c = new_LSClass(mc, FALSE_SHARING, sz, nelem);

	//we store all objects on an array. one after the other
	c->start = allocate(mc, nelem * sizeof(SharedObject*));

	int i;
	for (i = 0; i < nelem; ++i) {
		((SharedObject**)c->start)[i] = allocate(mc, sz);
	}
		
	assign_fs_pool_objects(mc, c);
	return c;
}
*/
/*
static LSClass *allocate_optimal_fs_pool(MContext *mc, size_t sz, unsigned long nelem) {
	
        //int num_threads = map_bits(reference_map);
//	int num_threads = reference_map->reference_count;
	//make sure that nelem is a multiple of num_threads
	if (nelem % num_threads != 0)
		nelem += num_threads - (nelem % num_threads);

	assert(nelem % num_threads == 0);
	
	LSClass *c = new_LSClass(mc, OPTIMAL_FALSE_SHARING, sz, nelem);

	int cache_lines_per_element = (sz / L1_LINE_SZ) + 1;

	c->start = allocate_aligned(mc, 
			nelem * cache_lines_per_element * L1_LINE_SZ, L1_LINE_SZ);
	
	assign_optimal_fs_pool_objects(mc, c);

	return c;
}
*/

static void deallocate_fs_pool(MContext *mc, LSClass *c) {

	//assert(c->reference_map == 0);
//        assert(c->reference_map.reference_count == 0);
	assert(c->start != NULL);

	int i;
	for (i = 0; i < c->num_objects; ++i) {
		 deallocate(mc, ((SharedObject**)c->start)[i], c->object_size);
	}
	deallocate(mc, c->start, c->num_objects * sizeof(SharedObject*));
	c->start = NULL;
	recycle_LSClass(mc, c);
}

static void deallocate_optimal_fs_pool(MContext *mc, LSClass *c) {
	
	int cache_lines_per_element = (c->object_size / L1_LINE_SZ) + 1;

	deallocate_aligned(mc, c->start, 
			c->num_objects * cache_lines_per_element * L1_LINE_SZ,
			L1_LINE_SZ);

	recycle_LSClass(mc, c);
}

static void traverse_fs_pool(MContext *mc, LSClass *c) {
	//check if thread bit is set in reference_map
	//reference_map_t my_bit = BIT_ZERO << mc->thread_id;

	//assert(c->reference_map != 0);
	assert(c->start != NULL);

	int i;
	for (i = 0; i < c->num_objects; ++i) {
		//check authorization on object
		//SharedObject *so = ((SharedObject**)c->start)[i];
		/*
                if (so->reference_map & my_bit) {
			int j;
			assert(c->reference_map != 0);
			for (j = 0; j < mc->gopts->write_iterations; ++j)
				write_object(so, c->object_size, sizeof(SharedObject));
		}
TODO(martin): fix per-object reference mapping
                */
	}
}

static void traverse_optimal_fs_pool(MContext *mc, LSClass *c) {

	//reference_map_t my_bit = BIT_ZERO << mc->thread_id;

	//assert(c->reference_map != 0);
	assert(c->start != NULL);

	//int cache_lines_per_element = (c->object_size / L1_LINE_SZ) + 1;
	
	int i;
	for (i = 0; i < c->num_objects; ++i) {
		//char *next = (char*)c->start + 
			//cache_lines_per_element * L1_LINE_SZ * i;
		//SharedObject *so = (SharedObject*)next;
	
                /*
		assert(c->reference_map != 0);
		
		if (so->reference_map & my_bit) {
			int j;
			assert(c->reference_map != 0);
			for (j = 0; j < mc->gopts->write_iterations; ++j)
				write_object(so, c->object_size, sizeof(SharedObject));
		}
TODO(martin): fix per-object reference mapping
                */

	}
}

// list-based implementation of lifetime-size-classes  ------------
static void traverse_list(MContext *mc, LSClass *c) {
	
	int access_counter = 0;

	//remember that the first word in payload is the next pointer
	//do not alter! cast Objects to LObjects
	LObject *list = (LObject*)c->start;
	
	while (list != NULL) {
		int i;
		if (write_ith_element(mc, access_counter++))
			for (i = 0; i < mc->gopts->write_iterations; ++i)
				write_object((Object*)list, 
						c->object_size, sizeof(LObject));
		list = list->next;
	}
}

LSClass *allocate_optimal_list_unaligned(MContext *mc, size_t sz, 
		unsigned long nelem) {

	LSClass *list = new_LSClass(mc, OPTIMAL_LIST, sz, nelem);

	//allocate whole memory at once
	list->start = allocate(mc, sz * nelem);
	LObject *tmp = (LObject*)list->start;
	
	//create pointers in contiguous memory
	int i;
	for (i = 1; i < nelem; ++i) {
		tmp->next = (LObject*)((char*)tmp + sz);
		tmp = tmp->next;
	}
	tmp->next = NULL;
	return list;
}

static void deallocate_optimal_list_unaligned(MContext *mc, LSClass *c) {
	deallocate(mc, c->start, c->object_size * c->num_objects);
	recycle_LSClass(mc, c);
}

static LSClass *allocate_list(MContext *mc, size_t sz, unsigned long nelem) {

	//check if size is sufficient for building a list
	//i.e., to contain an Object and an pointer to the next Object
	if (sz < sizeof(LObject)) {
		printf("Unable to allocate list. Min. object size too small.\n");
		exit(EXIT_FAILURE);
	}
	
	LSClass *list = new_LSClass(mc, LIST, sz, nelem);

	list->start = allocate(mc, sz);
	LObject *tmp = (LObject*)list->start;

	int i;
	for (i = 1; i < nelem; ++i) {
		tmp->next = (LObject*)allocate(mc, sz);
		tmp = tmp->next;
	}
	tmp->next = NULL;
	return list;
}


static void deallocate_list(MContext *mc, LSClass *c) {
	LObject *l = (LObject*)(c->start);
	while (l != NULL) {
		LObject *n = l->next;
		deallocate(mc, (Object*)l, c->object_size);
		l = n;
	}
	recycle_LSClass(mc, c);
}

//binary tree-based implemenation of lifetime-size-classes ---------------
static void deallocate_optimal_btree(MContext *mc, LSClass *c) {
	deallocate(mc, (Object*)(c->start), c->object_size * c->num_objects);
	recycle_LSClass(mc, c);
}

static LSClass *allocate_optimal_btree(MContext *mc, size_t sz, 
		unsigned long nelem) {

	if (sz < sizeof(BTObject)) {
		printf("Unable to allocate btree. Config error. "
				"Min.object size too small.\n");
		exit(EXIT_FAILURE);
	}

	LSClass *c = new_LSClass(mc, OPTIMAL_BTREE, sz, nelem);
	c->start = allocate(mc, sz * nelem);

	char *arr = (char*)(c->start);

	int i;
	for (i = 0; i < nelem; ++i) {
		BTObject *n = (BTObject*)(arr + i * sz);
		int left_child = 2 * i + 1;
		int right_child = 2 * i + 2;
		if (left_child > nelem - 1) {
			n->left = NULL;
		} else {
			n->left = (BTObject*)(arr + left_child * sz);
		}
		if (right_child > nelem - 1) {
			n->right = NULL;
		} else {
			n->right = (BTObject*)(arr + right_child * sz);
		}
	}
	
	return c;
}

static BTObject *build_tree_recursion(MContext *mc, size_t sz, 
		unsigned long nelem) {
	
	if (nelem == 0) return NULL;
	
	BTObject *t = (BTObject*)allocate(mc, sz);
	
	--nelem;

	int half = nelem / 2;
	t->left = build_tree_recursion(mc, sz, half);
	t->right = build_tree_recursion(mc, sz, nelem - half);
	
	return t;
}

static LSClass *allocate_btree(MContext *mc, size_t sz, unsigned long nelem) {
	
	if (sz < sizeof(BTObject)) {
		printf("Unable to allocate btree. Config error. "
				"Min.object size too small.\n");
		exit(EXIT_FAILURE);
	}

	LSClass *btree = new_LSClass(mc, BTREE, sz, nelem);

	btree->start = (Object*)build_tree_recursion(mc, sz, nelem);

	return btree;
}

static void deallocate_subtree_recursion(MContext *mc, BTObject *t, size_t node_sz) {
	if (t == NULL) return;
	deallocate_subtree_recursion(mc, t->left, node_sz);
	deallocate_subtree_recursion(mc, t->right, node_sz);
	deallocate(mc, (Object*)t, node_sz);
}

static void deallocate_btree(MContext *mc, LSClass *c) {
	deallocate_subtree_recursion(mc, (BTObject*)c->start, c->object_size);
	recycle_LSClass(mc, c);
}

static void btree_inverse_preorder_recursion(MContext *mc, BTObject *t, size_t sz) {
	if (t == NULL) return;
	int access_counter = 0;
	int i;
	if (write_ith_element(mc, access_counter++))
		for (i = 0; i < mc->gopts->write_iterations; ++i)
			write_object((Object*)t, sz, sizeof(BTObject));

	btree_inverse_preorder_recursion(mc, t->right, sz);
	btree_inverse_preorder_recursion(mc, t->left, sz);
}

static void traverse_btree_inverse_preorder(MContext *mc, LSClass *c) {
	btree_inverse_preorder_recursion(mc, (BTObject*)c->start, c->object_size);
}

static void btree_preorder_recursion(MContext *mc, BTObject *t, size_t sz) {
	if (t == NULL) return;
	int access_counter = 0;
	int i;
	if (write_ith_element(mc, access_counter++))
		for (i = 0; i < mc->gopts->write_iterations; ++i)
			write_object((Object*)t, sz, sizeof(BTObject));

	btree_preorder_recursion(mc, t->right, sz);
	btree_preorder_recursion(mc, t->left, sz);
}

static void traverse_btree_preorder(MContext *mc, LSClass *c) {
	btree_preorder_recursion(mc, (BTObject*)c->start, c->object_size);
}

// public methods
LSClass *allocate_LSClass(MContext *mc, lifetime_size_class_type type, size_t sz, 
		unsigned long nelem) {

	assert(sz > 0);
	assert(sz < (BIT_ZERO << mc->gopts->max_object_sc));
	assert(nelem > 0);

	//LSClass *c;

	switch (type) {
		case LIST:
			return allocate_list(mc, sz, nelem);
		case OPTIMAL_LIST:
			return allocate_optimal_list_unaligned(
					mc, sz, nelem);
		case BTREE:
			return allocate_btree(mc, sz, nelem);
		case OPTIMAL_BTREE:
			return allocate_optimal_btree(mc, sz, nelem);
		case FALSE_SHARING:
			//c = allocate_fs_pool(mc, sz, nelem);
			return NULL;
		case OPTIMAL_FALSE_SHARING:
			//c = allocate_optimal_fs_pool(mc, sz, nelem);
			return NULL;
		default:
			printf("Allocate: Collection Type not supported\n");
			exit(EXIT_FAILURE);
	}
}

void deallocate_LSClass(MContext *mc, LSClass *c) {

	unsigned long long start, end;

	assert(c->reference_counter == 0);
	assert(c->object_size > 0);
	assert(c->object_size < (BIT_ZERO << mc->gopts->max_object_sc));
	assert(c->num_objects > 0);

	start = rdtsc();

	switch (c->type) {
		case LIST:
			deallocate_list(mc, c);
			end = rdtsc();
			mc->stat->deallocation_time += end - start;
			return;
		case OPTIMAL_LIST:
			deallocate_optimal_list_unaligned(mc, c);
			end = rdtsc();
			mc->stat->deallocation_time += end - start;
			return;
		case BTREE:
			deallocate_btree(mc, c);
			end = rdtsc();
			mc->stat->deallocation_time += end - start;
			return;
		case OPTIMAL_BTREE:
			deallocate_optimal_btree(mc, c);
			end = rdtsc();
			mc->stat->deallocation_time += end - start;
			return;
		case FALSE_SHARING:
			deallocate_fs_pool(mc, c);
			end = rdtsc();
			mc->stat->deallocation_time += end - start;
			return;
		case OPTIMAL_FALSE_SHARING:
			deallocate_optimal_fs_pool(mc, c);
			end = rdtsc();
			mc->stat->deallocation_time += end - start;
			return;
		default:
			printf("Deallocate: Collection Type not supported\n");
			exit(EXIT_FAILURE);
	}

	
}

void traverse_LSClass(MContext *mc, LSClass *c) {

	switch (c->type) {
		case LIST:
			traverse_list(mc, c);
			return;
		case OPTIMAL_LIST:
			traverse_list(mc, c);
			return;
		case BTREE:
			traverse_btree_inverse_preorder(mc, c);
			return;
		case OPTIMAL_BTREE:
			traverse_btree_preorder(mc, c);
			return;
		case FALSE_SHARING:
			traverse_fs_pool(mc, c);
			return;
		case OPTIMAL_FALSE_SHARING:
			traverse_optimal_fs_pool(mc, c);
			return;
		default:
			printf("Traverse: Collection Type not supported\n");
			exit(EXIT_FAILURE);
	}
}


