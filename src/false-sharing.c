#include <stdlib.h>

#include "acdc.h"
#include "false-sharing.h"


OCollection *allocate_fs_pool(MContext *mc, size_t sz, unsigned long nelem) {
	
	OCollection *oc = new_collection(mc, FALSE_SHARING, sz, nelem);

	//we store all objects on an array. one after the other
	oc->start = calloc(nelem, sizeof(Object*));

	int i;
	for (i = 0; i < nelem; ++i) {
		((Object**)oc->start)[i] = allocate(mc, sz);
	}

	return oc;
}

void deallocate_fs_pool(MContext *mc, OCollection *oc){
	int i;
	for (i = 0; i < oc->num_objects; ++i) {
		 deallocate(mc, ((Object**)oc->start)[i], oc->object_size);
	}
	free(oc->start);
	free(oc);
}

void traverse_fs_pool(MContext *mc, OCollection *oc){

}

