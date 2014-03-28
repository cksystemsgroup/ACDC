 /*
 * Copyright (c) 2012, the ACDC Project Authors.
 * All rights reserved. Please see the AUTHORS file for details.
 * Use of this source code is governed by a BSD license that
 * can be found in the LICENSE file.
 */


#ifndef PROC_STATUS_H
#define PROC_STATUS_H

#include <sys/types.h>

void update_proc_status(pid_t pid);
long get_vm_peak();
long get_vm_size();
long get_resident_set_size();
long get_high_water_mark();
long get_data_segment_size();

#endif
