#include <stdlib.h>
#include <stdio.h>

#include "acdc.h"
#include "caches.h"

//TODO: tree

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

OCollection *allocate_optimal_list(MContext *mc, size_t sz, 
		unsigned long nelem) {


	//TODO: align objects


	if (sz < sizeof(LObject)) {
		printf("Unable to allocate list. Config error. Min. object size too small.\n");
		exit(EXIT_FAILURE);
	}

	OCollection *list = malloc(sizeof(OCollection));
	list->id = 0;
	list->object_size = sz;
	list->num_objects = nelem;
	list->type = OPTIMAL_LIST;

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


	OCollection *list = malloc(sizeof(OCollection));
	list->id = 0;
	list->object_size = sz;
	list->num_objects = nelem;
	list->type = LIST;

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

OCollection *allocate_collection(MContext *mc, collection_t ctype, size_t sz, 
		unsigned long nelem) {

	switch (ctype) {
		case LIST:
			return allocate_list(mc, sz, nelem);
			break;
		case OPTIMAL_LIST:
			return allocate_optimal_list(mc, sz, nelem);
			break;
		case TREE:
			return NULL;
			break;
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
			deallocate_optimal_list(mc, oc);
			return;
		case TREE:
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
		case TREE:
			return;
		default:
			printf("Collection Type not supported\n");
			exit(EXIT_FAILURE);
	}
}

