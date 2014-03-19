 /*
 * Copyright (c) 2012, the ACDC Project Authors.
 * All rights reserved. Please see the AUTHORS file for details.
 * Use of this source code is governed by a BSD license that
 * can be found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "acdc.h"
#include "arch.h"

//Schrage minimum standard PRNG. Assumes int to be 32 bits
static void next_rand(MContext *mc) {
	int hi = mc->rand / 127773;
	int lo = mc->rand % 127773;
	mc->rand = 16807 * lo - 2836 * hi;
	if (mc->rand < 0)
		mc->rand += 2147483647;
}

//returns a random integer in [from, to]
static int get_rand_int_range(MContext *mc, int from, int to) {
	assert(to >= from);
	next_rand(mc);
	int range = (to - from) + 1;
	int small_rand = mc->rand % range; //ok for small ranges
	return from + small_rand;
}

//determines if a lifetime-size-class should be shared
//based on "shared objects" and "shared objects ratio"
unsigned int get_sharing_dist(MContext *mc) {
	if (!mc->gopts->shared_objects) return 0;
	int r = get_rand_int_range(mc, 0, 100);
	if (r < mc->gopts->shared_objects_ratio) return 1;
	return 0;
}

static unsigned int get_random_liveness(MContext *mc) {
	return get_rand_int_range(mc,
			mc->gopts->min_liveness,
			mc->gopts->max_liveness);
}

static unsigned int get_random_size(MContext *mc) {
	int sc = get_rand_int_range(mc,
			mc->gopts->min_object_sc,
			mc->gopts->max_object_sc - 1);

	assert(sc >= mc->gopts->min_object_sc);
	assert(sc <= mc->gopts->max_object_sc - 1);

	unsigned int sz = get_rand_int_range(mc,
			BIT_ZERO << sc,
			(BIT_ZERO << (sc + 1)) -1);

	assert(sz >= (BIT_ZERO << mc->gopts->min_object_sc));
	assert(sz <= (BIT_ZERO << mc->gopts->max_object_sc));

	return sz;
}


static lifetime_size_class_type get_random_lifetime_size_class_type(MContext *mc) {

	unsigned int r = get_rand_int_range(mc, 0, 100);

	if (r < mc->gopts->list_based_ratio) {
		return LIST;
	}
	if (r < (mc->gopts->list_based_ratio + mc->gopts->btree_based_ratio)) {
		return BTREE;
	}
	return LIST; //default
}

static inline void swap_int(int *a, int *b) {
        if (a != b) {
                *a ^= *b;
                *b ^= *a;
                *a ^= *b;
        }
}

//initializes thread_id_array with random thread id's based on "receiving threads ratio" 
void get_random_thread_selection(MContext *mc, int *thread_id_array, int *thread_id_array_sz) {
        
	int number_of_sharing_threads = mc->gopts->num_threads / (100 / mc->gopts->receiving_threads_ratio);

        //printf("number of sharing threads %d\n", number_of_sharing_threads);

        //get number_of_sharing_threads random thread id's from 0 to gopts->num_threads-1

        //init thread_id_array with all possible thread id's and remove randomly


        *thread_id_array_sz = mc->gopts->num_threads;

        int i;
        for (i = 0; i < *thread_id_array_sz; ++i) {
                thread_id_array[i] = i;
        }

        for (i = 0; i < mc->gopts->num_threads - number_of_sharing_threads; ++i) {
                //randomly select a still existing thread id
                int id = get_rand_int_range(mc, 0, *thread_id_array_sz - 1);

                //swap element at position id with last element
                swap_int(&thread_id_array[id], &thread_id_array[*thread_id_array_sz - 1]);

                //"virtually" remove last element
                (*thread_id_array_sz)--;
        }
}

//creates random object properties and stores them in call-by-reference arguments
void get_random_object_props(MContext *mc, 
		size_t *size, 
		unsigned int *liveness, 
		unsigned int *num_objects,
		lifetime_size_class_type *type) {

	unsigned int lt = get_random_liveness(mc);
	unsigned int sz = get_random_size(mc);
	unsigned int sc = get_sizeclass(sz);

	unsigned int effect_of_sizeclass = (mc->gopts->max_object_sc - sc) + 1;
	effect_of_sizeclass *= effect_of_sizeclass; //quadratic impact

	unsigned int effect_of_liveness = (mc->gopts->max_liveness - lt) + 1;
	effect_of_liveness *= effect_of_liveness; //quadratic impact

	//output parameters
	*size = sz;
	*liveness = lt;
	*num_objects = effect_of_sizeclass * effect_of_liveness;

	assert(*num_objects > 0);
	assert(sz <= (BIT_ZERO << mc->gopts->max_object_sc));

	*type = get_random_lifetime_size_class_type(mc);
}

