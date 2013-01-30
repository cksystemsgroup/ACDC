 /*
 * Copyright (c) 2012, the ACDC Project Authors.
 * All rights reserved. Please see the AUTHORS file for details.
 * Use of this source code is governed by a BSD license that
 * can be found in the LICENSE file.
 */


#ifndef FALSE_SHARING_H
#define FALSE_SHARING_H

#include "acdc.h"

OCollection *allocate_fs_pool(MContext *mc, size_t sz, unsigned long nelem,
                              u_int64_t sharing_map);
OCollection *allocate_optimal_fs_pool(MContext *mc, size_t sz, unsigned long nelem,
                                      u_int64_t sharing_map);
void deallocate_fs_pool(MContext *mc, OCollection *oc);
void deallocate_optimal_fs_pool(MContext *mc, OCollection *oc);
void traverse_fs_pool(MContext *mc, OCollection *oc);
void traverse_optimal_fs_pool(MContext *mc, OCollection *oc);
void assign_fs_pool_objects(MContext *mc, OCollection *oc, u_int64_t sharing_map);
void assign_optimal_fs_pool_objects(MContext *mc, OCollection *oc, u_int64_t sharing_map);
/////////////////
//small objects
//

OCollection *allocate_small_fs_pool(MContext *mc, size_t sz, unsigned long nelem,
                              u_int64_t sharing_map);
OCollection *allocate_small_optimal_fs_pool(MContext *mc, size_t sz, unsigned long nelem,
                                      u_int64_t sharing_map);
void deallocate_small_fs_pool(MContext *mc, OCollection *oc);
void deallocate_small_optimal_fs_pool(MContext *mc, OCollection *oc);
void traverse_small_fs_pool(MContext *mc, OCollection *oc, int readonly);
void traverse_small_optimal_fs_pool(MContext *mc, OCollection *oc, int readonly);
void assign_small_fs_pool_objects(MContext *mc, OCollection *oc, u_int64_t sharing_map);
void assign_small_optimal_fs_pool_objects(MContext *mc, OCollection *oc, u_int64_t sharing_map);
#endif
