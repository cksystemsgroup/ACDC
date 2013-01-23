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

#include "acdc.h"
#include "caches.h"
#include "false-sharing.h"

OCollection *new_collection(MContext *mc, collection_t t, 
		size_t sz, unsigned long nelem, u_int64_t rctm) {

	OCollection *c = malloc(sizeof(OCollection));
	c->id = 0;
	c->object_size = sz;
	c->num_objects = nelem;
	c->type = t;
	c->sharing_map = rctm;
	c->reference_map = 0;//1 << mc->opt.thread_id;
	
	return c;
}

void traverse_list(MContext *mc, OCollection *oc) {
	//remember that the first word in payload is the next pointer
	//do not alter! cast Objects to LObjects
	//
	//access object
	
	LObject *list = (LObject*)oc->start;
	
	while (list != NULL) {
		//printf("access object\n");
		int i;
		for (i = 0; i < mc->gopts->access_iterations; ++i)
			access_object((Object*)list, oc->object_size, sizeof(LObject));
		list = list->next;
	}
}


size_t get_optimal_list_sz(size_t sz, unsigned long nelem, size_t alignment) {
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

OCollection *allocate_optimal_list_unaligned(MContext *mc, size_t sz, 
		unsigned long nelem, u_int64_t rctm) {


	OCollection *list = new_collection(mc, OPTIMAL_LIST, sz, nelem, rctm);

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


static void deallocate_optimal_list_unaligned(MContext *mc, OCollection *oc) {
	deallocate(mc, oc->start, oc->object_size * oc->num_objects);
	free(oc);
}


static OCollection *allocate_list(MContext *mc, size_t sz, unsigned long nelem, u_int64_t rctm) {

	//check of size is sufficient for building a list
	//i.e., to contain an Object and an pointer to the next Object
	if (sz < sizeof(LObject)) {
		printf("Unable to allocate list. Config error. Min. object size too small.\n");
		exit(EXIT_FAILURE);
	}
	
	OCollection *list = new_collection(mc, LIST, sz, nelem, rctm);

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


static void deallocate_list(MContext *mc, OCollection *oc) {
	LObject *l = (LObject*)(oc->start);
	while (l != NULL) {
		LObject *n = l->next;
		deallocate(mc, (Object*)l, oc->object_size);
		l = n;
		//printf("delete\n");
	}
	free(oc);
}


void deallocate_optimal_btree(MContext *mc, OCollection *oc) {
	deallocate(mc, (Object*)(oc->start), oc->object_size * oc->num_objects);
	free(oc);
}

OCollection *allocate_optimal_btree(MContext *mc, size_t sz, unsigned long nelem, u_int64_t rctm) {
	if (sz < sizeof(BTObject)) {
		printf("Unable to allocate btree. Config error. "
				"Min.object size too small.\n");
		exit(EXIT_FAILURE);
	}

	OCollection *oc = new_collection(mc, OPTIMAL_BTREE, sz, nelem, rctm);
	oc->start = allocate(mc, sz * nelem);

	char *arr = (char*)(oc->start);

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
	
	return oc;
}

BTObject *build_tree_recursion(MContext *mc, size_t sz, unsigned long nelem) {
	
	if (nelem == 0) return NULL;
	
	BTObject *t = (BTObject*)allocate(mc, sz);
	
	--nelem;

	int half = nelem / 2;
	t->left = build_tree_recursion(mc, sz, half);
	t->right = build_tree_recursion(mc, sz, nelem - half);
	
	return t;
}

OCollection *allocate_btree(MContext *mc, size_t sz, unsigned long nelem, u_int64_t rctm) {
	if (sz < sizeof(BTObject)) {
		printf("Unable to allocate btree. Config error. "
				"Min.object size too small.\n");
		exit(EXIT_FAILURE);
	}

	OCollection *btree = new_collection(mc, BTREE, sz, nelem, rctm);

	btree->start = (Object*)build_tree_recursion(mc, sz, nelem);

	return btree;
}

void deallocate_subtree_recursion(MContext *mc, BTObject *t, size_t node_sz) {
	if (t == NULL) return;
	deallocate_subtree_recursion(mc, t->left, node_sz);
	deallocate_subtree_recursion(mc, t->right, node_sz);
	deallocate(mc, (Object*)t, node_sz);
}

void deallocate_btree(MContext *mc, OCollection *oc) {
	deallocate_subtree_recursion(mc, (BTObject*)oc->start, oc->object_size);
	free(oc);
}


void btree_preorder_recursion(MContext *mc, BTObject *t, size_t sz) {
	if (t == NULL) return;
	int i;
	for (i = 0; i < mc->gopts->access_iterations; ++i)
		access_object((Object*)t, sz, sizeof(BTObject));
	btree_preorder_recursion(mc, t->left, sz);
	btree_preorder_recursion(mc, t->right, sz);
}

void traverse_btree_preorder(MContext *mc, OCollection *oc) {
	btree_preorder_recursion(mc, (BTObject*)oc->start, oc->object_size);
}



OCollection *allocate_collection(MContext *mc, collection_t ctype, size_t sz, 
		unsigned long nelem, u_int64_t rctm) {

	OCollection *oc;

	switch (ctype) {
		case LIST:
			return allocate_list(mc, sz, nelem, rctm);
		case OPTIMAL_LIST:
			return allocate_optimal_list_unaligned(mc, sz, nelem, rctm);
		case BTREE:
			return allocate_btree(mc, sz, nelem, rctm);
		case OPTIMAL_BTREE:
			return allocate_optimal_btree(mc, sz, nelem, rctm);
		case FALSE_SHARING:
			oc = allocate_small_fs_pool(mc, sz, nelem, rctm);
			//assign_fs_pool_objects(mc, oc, rctm);
			return oc;
		case OPTIMAL_FALSE_SHARING:
			oc = allocate_small_optimal_fs_pool(mc, sz, nelem, rctm);
			//assign_optimal_fs_pool_objects(mc, oc, rctm);
			return oc;
		default:
			printf("Allocate: Collection Type not supported\n");
			exit(EXIT_FAILURE);
	}
}

void deallocate_collection(MContext *mc, OCollection *oc) {

	assert(oc->reference_map == 0);

	switch (oc->type) {
		case LIST:
			deallocate_list(mc, oc);
			return;
		case OPTIMAL_LIST:
			deallocate_optimal_list_unaligned(mc, oc);
			return;
		case BTREE:
			deallocate_btree(mc, oc);
			return;
		case OPTIMAL_BTREE:
			deallocate_optimal_btree(mc, oc);
			return;
		case FALSE_SHARING:
			deallocate_small_fs_pool(mc, oc);
			return;
		case OPTIMAL_FALSE_SHARING:
			deallocate_small_optimal_fs_pool(mc, oc);
			return;
		default:
			printf("Deallocate: Collection Type not supported\n");
			exit(EXIT_FAILURE);
	}
}
void traverse_collection(MContext *mc, OCollection *oc) {

	if (mc->gopts->access_iterations == 0) return;

	switch (oc->type) {
		case LIST:
			traverse_list(mc, oc);
			return;
		case OPTIMAL_LIST:
			traverse_list(mc, oc);
			return;
		case BTREE:
			traverse_btree_preorder(mc, oc);
			return;
		case OPTIMAL_BTREE:
			traverse_btree_preorder(mc, oc);
			return;
		case FALSE_SHARING:
			traverse_small_fs_pool(mc, oc);
			return;
		case OPTIMAL_FALSE_SHARING:
			traverse_small_optimal_fs_pool(mc, oc);
			return;
		default:
			printf("Traverse: Collection Type not supported\n");
			exit(EXIT_FAILURE);
	}
}

