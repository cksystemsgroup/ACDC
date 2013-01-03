#include <stdlib.h>

#include "acdc.h"


static OCollection *allocate_list(size_t sz, unsigned long nelem) {

	OCollection *list = malloc(sizeof(OCollection));

	list->object_size = sz;

	list->start = allocate(sz)



}


OCollection *allocate_collection(MContext *mc, collection_t, size_t sz, 
		unsigned long nelem) {


}

