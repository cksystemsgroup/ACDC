 /*
 * Copyright (c) 2012, the ACDC Project Authors.
 * All rights reserved. Please see the AUTHORS file for details.
 * Use of this source code is governed by a BSD license that
 * can be found in the LICENSE file.
 */


#include <stdio.h>
#include <assert.h>

#include "acdc.h"


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
static unsigned int get_sharing_dist(MContext *mc) {
	if (!mc->gopts->share_objects) return 0;
	int r = get_rand_int_range(mc, 0, 100);
	if (r < mc->gopts->share_ratio) return 1;
	return 0;
}

static unsigned int get_random_lifetime(MContext *mc) {
	return get_rand_int_range(mc,
			mc->gopts->min_lifetime,
			mc->gopts->max_lifetime);
}

static unsigned int get_random_size(MContext *mc) {
	int sc = get_rand_int_range(mc,
			mc->gopts->min_object_sc,
			mc->gopts->max_object_sc);

	return get_rand_int_range(mc,
			1UL << sc,
			(1UL << (sc + 1)) -1);
}

unsigned int get_random_thread(MContext *mc) {
	return get_rand_int_range(mc,
			0,
			mc->gopts->num_threads - 1);
}

static collection_type get_random_collection_type(MContext *mc) {

	unsigned int r = get_rand_int_range(mc, 0, 100);

	if (r < mc->gopts->list_based_ratio) {
		return LIST;
	}
	if (r < (mc->gopts->list_based_ratio + mc->gopts->btree_based_ratio)) {
		return BTREE;
	}
	return LIST; //default
}

//returns a sharing bitmap based on the "receiving threads ratio" 
static u_int64_t get_random_thread_selection(MContext *mc) {

	u_int64_t my_thread_bit = 1UL << mc->thread_id;

	if (mc->gopts->share_objects == 0 || 
			mc->gopts->share_thread_ratio == 0) {
		//only this thread is interested
		return my_thread_bit;
	}

	//threads except me, times share ratio
	int number_of_other_threads = 
		(mc->gopts->num_threads - 1) / (100 / mc->gopts->share_thread_ratio);


	//get number_of_other_threads random thread id's (except mine)
	//and set their bits in tm
	int i = 0;
	u_int64_t tm = my_thread_bit;
	while (i < number_of_other_threads) {
		int tid = get_random_thread(mc);
		//check if we already haven't added this thread
		if (!(tm & (1UL << tid))) {
			++i;
			tm |= 1UL << tid;
		}
	}
	return tm;
}

//creates random object properties and stores them in call-by-reference arguments
void get_random_object_props(MContext *mc, 
		size_t *size, 
		unsigned int *lifetime, 
		unsigned int *num_objects,
		collection_type *type,
		u_int64_t *sharing_map) {

	unsigned int lt = get_random_lifetime(mc);
	unsigned int sz = get_random_size(mc);
	unsigned int sc = get_sizeclass(sz);

	unsigned int effect_of_sizeclass = (mc->gopts->max_object_sc - sc) + 1;
	effect_of_sizeclass *= effect_of_sizeclass; //quadratic impact
	//effect_of_sizeclass *= effect_of_sizeclass; //cubic impact

	unsigned int effect_of_lifetime = (mc->gopts->max_lifetime - lt) + 1;
	effect_of_lifetime *= effect_of_lifetime; //quadratic impact
	//effect_of_lifetime *= effect_of_lifetime; //cubic impact

	//output parameters
	*size = sz;
	*lifetime = lt;
	*num_objects = effect_of_sizeclass * effect_of_lifetime;

	assert(*num_objects > 0);

	*type = get_random_collection_type(mc);
	if (get_sharing_dist(mc)) {
		*sharing_map = get_random_thread_selection(mc); //shared objects
	} else {
		*sharing_map = 1UL << mc->thread_id; //unshared
	}
	return;
}

