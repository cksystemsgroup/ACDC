 /*
 * Copyright (c) 2012, the ACDC Project Authors.
 * All rights reserved. Please see the AUTHORS file for details.
 * Use of this source code is governed by a BSD license that
 * can be found in the LICENSE file.
 */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <alloca.h>

#include "acdc.h"
#include "caches.h"


static LSClass *get_LSClass(MContext *mc) {
	//treat Classes as Nodes. They are big enough. Just cast
	LSCNode *node = mc->class_cache.first;
	if (node != NULL) {
		lclass_remove(&mc->class_cache, node);
		debug("reuse class %p\n", node);
		node->next = NULL;
		node->prev = NULL;
		return (LSClass*)node;
	}

	if (mc->class_buffer_counter >= mc->gopts->class_buffer_size) {
		printf("class buffer overflow. increase -C option\n");
		exit(EXIT_FAILURE);
	}
	node = (LSCNode*)(((char*)mc->class_buffer_memory) + 
		(mc->class_buffer_counter * L1_LINE_SZ));

	mc->class_buffer_counter++;
	debug("class %p\n", node);
	node->next = NULL;
	node->prev = NULL;
	return (LSClass*)node;
}
static void recycle_LSClass(MContext *mc, LSClass *class) {
	debug("give back class %p\n", class);
	LSCNode *n = (LSCNode*)class;
	lclass_insert_beginning(&mc->class_cache, n);
}

static void get_thread_ids(int *thread_ids, u_int64_t sharing_map) {
	//int *thread_ids = calloc(num_threads, sizeof(int)); //TODO remove calloc
	int i, j;
	for (i = 0, j = 0; i < sizeof(u_int64_t); ++i) {
		if ( (1UL << i) & sharing_map ) {
			thread_ids[j++] = i;
		}
	}
}
/**
 * write_ratio determines how many objects are written. e.g., 
 * write_ratio of 20 means that every 5th element is written
 */
static int write_ith_element(MContext *mc, int i) {
	if (mc->gopts->write_ratio == 0) return 0;
	if (mc->gopts->write_ratio >= 100) return 1;
	int ith = 100 / mc->gopts->write_ratio;
	if ( i % ith == 0 ) return 1;
	return 0;
}

/**
 * allocates memory for a new LSCLass
 */
static LSClass *new_LSClass(MContext *mc, collection_type t, 
		size_t sz, unsigned long nelem, u_int64_t sharing_map) {

	LSClass *c = get_LSClass(mc);
	c->object_size = sz;
	c->num_objects = nelem;
	c->type = t;
	c->sharing_map = sharing_map;
	c->reference_map = 0;
	return c;
}

// false sharing ---------------------
static void assign_optimal_fs_pool_objects(MContext *mc, LSClass *c, 
		u_int64_t sharing_map) {

	//check which threads should participate
	int num_threads = __builtin_popcountl(sharing_map);
	int *thread_ids = alloca(num_threads * sizeof(int));
	get_thread_ids(thread_ids, sharing_map);

	int cache_lines_per_element = (c->object_size / L1_LINE_SZ) + 1;
	assert(cache_lines_per_element == 1);

	int i;
	for (i = 0; i < c->num_objects; ++i) {
		char *next = (char*)c->start + cache_lines_per_element * L1_LINE_SZ * i;
		SharedObject *o = (SharedObject*)next;
		o->sharing_map = 1UL << ( thread_ids[i % num_threads] );
	}

	assert(i % num_threads == 0);
}


static void assign_fs_pool_objects(MContext *mc, LSClass *c, u_int64_t sharing_map) {

	//check which threads should participate
	int num_threads = __builtin_popcountl(sharing_map);
	int *thread_ids = alloca(num_threads * sizeof(int));
	get_thread_ids(thread_ids, sharing_map);

	int i;
	for (i = 0; i < c->num_objects; ++i) {
		SharedObject *o = ((SharedObject**)c->start)[i];
		o->sharing_map = 1UL << ( thread_ids[i % num_threads]  );	
	}
}

static LSClass *allocate_fs_pool(MContext *mc, size_t sz, unsigned long nelem, 
		u_int64_t sharing_map) {
	
	int num_threads = __builtin_popcountl(sharing_map);
	//make sure that nelem is a multiple of num_threads
	if (nelem % num_threads != 0)
		nelem += num_threads - (nelem % num_threads);

	LSClass *c = new_LSClass(mc, FALSE_SHARING, sz, nelem, sharing_map);

	//we store all objects on an array. one after the other
	c->start = allocate(mc, nelem * sizeof(SharedObject*));

	int i;
	for (i = 0; i < nelem; ++i) {
		((SharedObject**)c->start)[i] = allocate(mc, sz);
	}
		
	assign_fs_pool_objects(mc, c, sharing_map);
	return c;
}

static LSClass *allocate_optimal_fs_pool(MContext *mc, size_t sz, unsigned long nelem,
		u_int64_t sharing_map) {
	
	int num_threads = __builtin_popcountl(sharing_map);
	//make sure that nelem is a multiple of num_threads
	if (nelem % num_threads != 0)
		nelem += num_threads - (nelem % num_threads);

	assert(nelem % num_threads == 0);
	
	LSClass *c = new_LSClass(mc, OPTIMAL_FALSE_SHARING, sz, nelem, sharing_map);

	int cache_lines_per_element = (sz / L1_LINE_SZ) + 1;

	c->start = allocate_aligned(mc, 
			nelem * cache_lines_per_element * L1_LINE_SZ, L1_LINE_SZ);
	
	assign_optimal_fs_pool_objects(mc, c, sharing_map);

	return c;
}

static void deallocate_fs_pool(MContext *mc, LSClass *c) {

	assert(c->reference_map == 0);
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

	//free(c);
	recycle_LSClass(mc, c);
}

static void traverse_fs_pool(MContext *mc, LSClass *c) {
	//check if thread bit is set in sharing_map
	u_int64_t my_bit = 1UL << mc->thread_id;

	assert(c->reference_map != 0);
	assert(c->start != NULL);

	int i;
	for (i = 0; i < c->num_objects; ++i) {
		//check out what are my objects
		SharedObject *so = ((SharedObject**)c->start)[i];
		if (so->sharing_map & my_bit) {
			//printf("ACCESS\n");
			int j;
			assert(c->reference_map != 0);
			//long long access_start = rdtsc();
			for (j = 0; j < mc->gopts->write_iterations; ++j)
				write_object(so, c->object_size, sizeof(SharedObject));
			//long long access_end = rdtsc();
			//mc->stat->access_time += access_end - access_start;
		}
	}
}

static void traverse_optimal_fs_pool(MContext *mc, LSClass *c) {

	//check if thread bit is set in sharing_map
	u_int64_t my_bit = 1UL << mc->thread_id;

	assert(c->reference_map != 0);
	assert(c->start != NULL);

	int cache_lines_per_element = (c->object_size / L1_LINE_SZ) + 1;
	
	int i;
	for (i = 0; i < c->num_objects; ++i) {
		char *next = (char*)c->start + 
			cache_lines_per_element * L1_LINE_SZ * i;
		SharedObject *so = (SharedObject*)next;
		
		assert(c->reference_map != 0);
		
		if (so->sharing_map & my_bit) {
			//printf("ACCESS\n");
			int j;
			assert(c->reference_map != 0);
			//long long access_start = rdtsc();
			for (j = 0; j < mc->gopts->write_iterations; ++j)
				write_object(so, c->object_size, sizeof(SharedObject));
			//long long access_end = rdtsc();
			//mc->stat->access_time += access_end - access_start;
		}
	}
}

// linked list ----------------------------
static void traverse_list(MContext *mc, LSClass *c) {
	
	int access_counter = 0;

	//remember that the first word in payload is the next pointer
	//do not alter! cast Objects to LObjects
	LObject *list = (LObject*)c->start;
	
	while (list != NULL) {
		//printf("access object\n");
		int i;
		if (write_ith_element(mc, access_counter++))
			for (i = 0; i < mc->gopts->write_iterations; ++i)
				write_object((Object*)list, 
						c->object_size, sizeof(LObject));
		list = list->next;
	}
}

/*
static size_t get_aligned_optimal_list_sz(size_t sz, unsigned long nelem, size_t alignment) {
	int objects_per_line = L1_LINE_SZ / sz;
	int lines_required = 0;
	if (objects_per_line > 0) {
		//allocate one extra cache line in case nelem % objects_per_line != 0
		lines_required = (nelem / objects_per_line);
		if (nelem % objects_per_line != 0) lines_required++;
	} else {
		//object is larger than a cache line
		int lines_per_object = sz / L1_LINE_SZ;
		if (sz % L1_LINE_SZ != 0) lines_per_object++;
		lines_required = nelem * lines_per_object;
	}
	return lines_required * L1_LINE_SZ;
}
*/

LSClass *allocate_optimal_list_unaligned(MContext *mc, size_t sz, 
		unsigned long nelem, u_int64_t sharing_map) {


	LSClass *list = new_LSClass(mc, OPTIMAL_LIST, sz, nelem, sharing_map);

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
	//free(c);
	recycle_LSClass(mc, c);
}


static LSClass *allocate_list(MContext *mc, size_t sz, unsigned long nelem, u_int64_t sharing_map) {

	//check of size is sufficient for building a list
	//i.e., to contain an Object and an pointer to the next Object
	if (sz < sizeof(LObject)) {
		printf("Unable to allocate list. Config error. Min. object size too small.\n");
		exit(EXIT_FAILURE);
	}
	
	LSClass *list = new_LSClass(mc, LIST, sz, nelem, sharing_map);

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
		//printf("delete\n");
	}
	//free(c);
	recycle_LSClass(mc, c);
}

//binary tree -----------------------
static void deallocate_optimal_btree(MContext *mc, LSClass *c) {
	deallocate(mc, (Object*)(c->start), c->object_size * c->num_objects);
	//free(c);
	
	recycle_LSClass(mc, c);
}

static LSClass *allocate_optimal_btree(MContext *mc, size_t sz, 
		unsigned long nelem, u_int64_t sharing_map) {

	if (sz < sizeof(BTObject)) {
		printf("Unable to allocate btree. Config error. "
				"Min.object size too small.\n");
		exit(EXIT_FAILURE);
	}

	LSClass *c = new_LSClass(mc, OPTIMAL_BTREE, sz, nelem, sharing_map);
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
	t->right = build_tree_recursion(mc, sz, nelem - half);
	t->left = build_tree_recursion(mc, sz, half);
	
	return t;
}

static LSClass *allocate_btree(MContext *mc, size_t sz, unsigned long nelem, 
		u_int64_t sharing_map) {
	
	if (sz < sizeof(BTObject)) {
		printf("Unable to allocate btree. Config error. "
				"Min.object size too small.\n");
		exit(EXIT_FAILURE);
	}

	LSClass *btree = new_LSClass(mc, BTREE, sz, nelem, sharing_map);

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
	//free(c);
	recycle_LSClass(mc, c);
}


static void btree_preorder_recursion(MContext *mc, BTObject *t, size_t sz) {
	if (t == NULL) return;
	int access_counter = 0;
	int i;
	if (write_ith_element(mc, access_counter++))
		for (i = 0; i < mc->gopts->write_iterations; ++i)
			write_object((Object*)t, sz, sizeof(BTObject));

	btree_preorder_recursion(mc, t->left, sz);
	btree_preorder_recursion(mc, t->right, sz);
}

static void traverse_btree_preorder(MContext *mc, LSClass *c) {
	btree_preorder_recursion(mc, (BTObject*)c->start, c->object_size);
}


// public methods
LSClass *allocate_LSClass(MContext *mc, collection_type ctype, size_t sz, 
		unsigned long nelem, u_int64_t sharing_map) {

	LSClass *c;

	switch (ctype) {
		case LIST:
			return allocate_list(mc, sz, nelem, sharing_map);
		case OPTIMAL_LIST:
			return allocate_optimal_list_unaligned(mc, sz, nelem, sharing_map);
		case BTREE:
			return allocate_btree(mc, sz, nelem, sharing_map);
		case OPTIMAL_BTREE:
			return allocate_optimal_btree(mc, sz, nelem, sharing_map);
		case FALSE_SHARING:
			c = allocate_fs_pool(mc, sz, nelem, sharing_map);
			//assign_fs_pool_objects(mc, c, sharing_map);
			return c;
		case OPTIMAL_FALSE_SHARING:
			c = allocate_optimal_fs_pool(mc, sz, nelem, sharing_map);
			//assign_optimal_fs_pool_objects(mc, c, sharing_map);
			return c;
		default:
			printf("Allocate: Collection Type not supported\n");
			exit(EXIT_FAILURE);
	}
}

void deallocate_LSClass(MContext *mc, LSClass *c) {

	assert(c->reference_map == 0);

	switch (c->type) {
		case LIST:
			deallocate_list(mc, c);
			return;
		case OPTIMAL_LIST:
			deallocate_optimal_list_unaligned(mc, c);
			return;
		case BTREE:
			deallocate_btree(mc, c);
			return;
		case OPTIMAL_BTREE:
			deallocate_optimal_btree(mc, c);
			return;
		case FALSE_SHARING:
			deallocate_fs_pool(mc, c);
			return;
		case OPTIMAL_FALSE_SHARING:
			deallocate_optimal_fs_pool(mc, c);
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
			traverse_btree_preorder(mc, c);
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

