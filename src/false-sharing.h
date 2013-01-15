#ifndef FALSE_SHARING_H
#define FALSE_SHARING_H

#include "acdc.h"

OCollection *allocate_fs_pool(MContext *mc, size_t sz, unsigned long nelem);
OCollection *allocate_optimal_fs_pool(MContext *mc, size_t sz, unsigned long nelem);
void deallocate_fs_pool(MContext *mc, OCollection *oc);
void deallocate_optimal_fs_pool(MContext *mc, OCollection *oc);
void traverse_fs_pool(MContext *mc, OCollection *oc);
void traverse_optimal_fs_pool(MContext *mc, OCollection *oc);
void assign_fs_pool_objects(MContext *mc, OCollection *oc, u_int64_t rctm);
void assign_optimal_fs_pool_objects(MContext *mc, OCollection *oc, u_int64_t rctm);

#endif
