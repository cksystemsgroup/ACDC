

#include "acdc.h"
#include "false-sharing.h"




OCollection *allocate_fs_pool(MContext *mc, size_t sz, unsigned long nelem) {
	OCollection *oc = new_collection(mc, FALSE_SHARING, sz, nelem);





	return oc;
}

void deallocate_fs_pool(MContext *mc, OCollection *oc){

}

void traverse_fs_pool(MContext *mc, OCollection *oc){

}

