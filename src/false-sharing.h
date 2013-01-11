#ifndef FALSE_SHARING_H
#define FALSE_SHARING_H

#include "acdc.h"

OCollection *allocate_fs_pool(MContext *mc, size_t sz, unsigned long nelem);
void deallocate_fs_pool(MContext *mc, OCollection *oc);
void traverse_fs_pool(MContext *mc, OCollection *oc);

#endif
