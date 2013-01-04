#include <stdlib.h>
#include <stdio.h>

#include "acdc.h"


//TODO: tree

void traverse_list(void) {
	//remember that the first word in payload is the next pointer
	//do not alter!
	//
	//access object
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
		case TREE:
			return;
		default:
			printf("Collection Type not supported\n");
			exit(EXIT_FAILURE);
	}
}

