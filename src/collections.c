#include <stdlib.h>
#include <stdio.h>

#include "acdc.h"
#include "caches.h"

OCollection *new_collection(MContext *mc, collection_t t, 
		size_t sz, unsigned long nelem) {
	OCollection *c = malloc(sizeof(OCollection));
	c->id = 0;
	c->object_size = sz;
	c->num_objects = nelem;
	c->type = t;
}

void traverse_list(MContext *mc, OCollection *oc) {
	//remember that the first word in payload is the next pointer
	//do not alter! cast Objects to LObjects
	//
	//access object
	
	LObject *list = (LObject*)oc->start;
	
	while (list != NULL) {
		//printf("access object\n");
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
		unsigned long nelem) {


	OCollection *list = new_collection(mc, OPTIMAL_LIST, sz, nelem);

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

OCollection *allocate_optimal_list(MContext *mc, size_t sz, 
		unsigned long nelem) {

	if (sz < sizeof(LObject)) {
		printf("Unable to allocate list. Config error. Min. object size too small.\n");
		exit(EXIT_FAILURE);
	}

	OCollection *list = new_collection(mc, OPTIMAL_LIST, sz, nelem);

	size_t aligned_size = get_optimal_list_sz(sz, nelem, L1_LINE_SZ);

	//allocate whole memory at once
	//reserve one extra cache line for alignment
	list->start = allocate_aligned(mc, aligned_size, L1_LINE_SZ);


	LObject *tmp = (LObject*)list->start;


	int i;
	for (i = 1; i < nelem; ++i) {
		LObject *next = (LObject*)((long)tmp + sz);
		long bytes_available = L1_LINE_SZ - ((L1_LINE_SZ - 1) & (long)next);
		if (bytes_available >= sz) {
			tmp->next = next;
		} else {
			//start new cache line
			tmp->next = (LObject*)((long)next + bytes_available);
		}
		tmp = tmp->next;
	}
	tmp->next = 0;


	return list;
}

static OCollection *allocate_list(MContext *mc, size_t sz, unsigned long nelem) {

	//check of size is sufficient for building a list
	//i.e., to contain an Object and an pointer to the next Object
	if (sz < sizeof(LObject)) {
		printf("Unable to allocate list. Config error. Min. object size too small.\n");
		exit(EXIT_FAILURE);
	}
	
	OCollection *list = new_collection(mc, LIST, sz, nelem);

	//TODO: check for shared setting
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

static void deallocate_optimal_list(MContext *mc, OCollection *oc) {
	int objects_per_cache_line = L1_LINE_SZ / oc->object_size;
	deallocate_aligned(mc, oc->start, objects_per_cache_line * L1_LINE_SZ,
			L1_LINE_SZ);
	free(oc);
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

OCollection *allocate_optimal_btree(MContext *mc, size_t sz, unsigned long nelem) {
	if (sz < sizeof(BTObject)) {
		printf("Unable to allocate btree. Config error. "
				"Min.object size too small.\n");
		exit(EXIT_FAILURE);
	}

	OCollection *oc = new_collection(mc, OPTIMAL_BTREE, sz, nelem);
	oc->start = allocate(mc, sz * nelem);

	char *arr = (char*)(oc->start);

	int i;
	int j;
	for (i = 0; i < nelem; ++i) {
		BTObject *n = (BTObject*)(arr + i * sz);
		int left_child = 2 * i + 1;
		int right_child = 2 * i + 2;
		if (left_child > nelem - 1) {
			n->left = NULL;
			j = i;
		} else {
			n->left = (BTObject*)(arr + left_child * sz);
			j = i;
		}
		if (right_child > nelem - 1) {
			n->right = NULL;
			j = i;
		} else {
			n->right = (BTObject*)(arr + right_child * sz);
			j = i;
		}
	}
	
	return oc;
}

BTObject *build_tree_recursion(MContext *mc, size_t sz, unsigned long nelem) {
	
	if (nelem == 0) return NULL;
	
	BTObject *t = (BTObject*)allocate(mc, sz);
	
	if (nelem == 1) {
		// no more children
		t->left = NULL;
		t->right = NULL;
	} else {
		int half = nelem / 2;
		t->left = build_tree_recursion(mc, sz, half);
		t->right = build_tree_recursion(mc, sz, nelem - half);
	}
	
	return t;
}

OCollection *allocate_btree(MContext *mc, size_t sz, unsigned long nelem) {
	if (sz < sizeof(BTObject)) {
		printf("Unable to allocate btree. Config error. "
				"Min.object size too small.\n");
		exit(EXIT_FAILURE);
	}

	OCollection *btree = new_collection(mc, BTREE, sz, nelem);

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
	access_object((Object*)t, sz, sizeof(BTObject));
	btree_preorder_recursion(mc, t->left, sz);
	btree_preorder_recursion(mc, t->right, sz);
}

void traverse_btree_preorder(MContext *mc, OCollection *oc) {
	btree_preorder_recursion(mc, (BTObject*)oc->start, oc->object_size);
}



OCollection *allocate_collection(MContext *mc, collection_t ctype, size_t sz, 
		unsigned long nelem) {

	switch (ctype) {
		case LIST:
			return allocate_list(mc, sz, nelem);
		case OPTIMAL_LIST:
			return allocate_optimal_list_unaligned(mc, sz, nelem);
		case BTREE:
			return allocate_btree(mc, sz, nelem);
		case OPTIMAL_BTREE:
			return allocate_optimal_btree(mc, sz, nelem);
		default:
			printf("Collection Type not supported\n");
			exit(EXIT_FAILURE);
	}
}

void deallocate_collection(MContext *mc, OCollection *oc) {

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
		default:
			printf("Collection Type not supported\n");
			exit(EXIT_FAILURE);
	}
}
void traverse_collection(MContext *mc, OCollection *oc) {

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
		default:
			printf("Collection Type not supported\n");
			exit(EXIT_FAILURE);
	}
}

