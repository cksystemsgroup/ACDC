#include <stdlib.h>
#include <stdio.h>

#include "acdc.h"


//TODO: tree

void traverse_list(MContext *mc, OCollection *oc) {
	//remember that the first word in payload is the next pointer
	//do not alter!
	//
	//access object
	
	LObject *list = (LObject*)oc->start;
	
	while (list != NULL) {
		//printf("access object\n");
		access_object((Object*)list, oc->object_size, sizeof(LObject));
		list = list->next;	
	}
}


static OCollection *allocate_optimal_list(MContext *mc, size_t sz, 
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
	deallocate(mc, oc->start, oc->object_size * oc->num_objects);
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

