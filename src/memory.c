 /*
 * Copyright (c) 2012, the ACDC Project Authors.
 * All rights reserved. Please see the AUTHORS file for details.
 * Use of this source code is governed by a BSD license that
 * can be found in the LICENSE file.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "acdc.h"
#include "alloc/nulloc.h"

Object *allocate(MContext *mc, size_t size) {
	void *ptr;
		
	if (size < sizeof(Object)) {
		printf("Error: min object size is %lu. Requested: %lu\n",
				sizeof(Object),
				size);
		exit(EXIT_FAILURE);
	}

	ptr = acdc_alloc(size);
	
	//set header information
	Object *o = (Object*)ptr;

	return o;
}

/*
Object *allocate_aligned(MContext *mc, size_t size, size_t alignment) {

	Object *o = allocate(mc, size + alignment + sizeof(Object*));
	Object *aligned_o = (Object*)(((long)o + alignment + sizeof(Object*)) &
		~(alignment-1));

	((void**)aligned_o)[-1] = o;

	return aligned_o;
}

void deallocate_aligned(MContext *mc, Object *o, size_t size, size_t alignment) {

	deallocate(mc, ((void**)o)[-1], size + alignment + sizeof(Object*));
}
*/

void deallocate(MContext *mc, Object *o, size_t size) {
	acdc_free(o);
}


unsigned int get_sizeclass(size_t size) {
	return (unsigned int)log2((double)size);
}

