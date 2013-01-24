 /*
 * Copyright (c) 2012, the ACDC Project Authors.
 * All rights reserved. Please see the AUTHORS file for details.
 * Use of this source code is governed by a BSD license that
 * can be found in the LICENSE file.
 */

#include <stdio.h>
#include "resources.h"

#ifdef __linux__

#include <sys/time.h>
#include <sys/resource.h>


long get_resident_set_size() {
	return 0;
}
long get_high_water_mark() {
	return 0;
}
long get_data_segment_size() {
	return 0;
}


#else // everyting but linux not supported yet

long get_resident_set_size() {
	printf("WARNING: resource information not supported for this platform\n");
	return 0;
}
long get_high_water_mark() {
	printf("WARNING: resource information not supported for this platform\n");
	return 0;
}
long get_data_segment_size() {
	printf("WARNING: resource information not supported for this platform\n");
	return 0;
}

#endif // __linux__
