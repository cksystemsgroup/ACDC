#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "acdc.h"

GOptions *gopts;
MContext *mc;

void set_default_params(GOptions *gopts) {
	gopts->mode = ACDC;
	gopts->num_threads = 1;
	gopts->time_threshold = 1<<18;
	gopts->benchmark_duration = 100;
	gopts->seed = 1;
	gopts->min_lifetime = 1;
	gopts->max_lifetime = 10;
	gopts->min_object_sc = 4;
	gopts->max_object_sc = 8;
	gopts->share_objects = 0;
	gopts->share_ratio = 0;
	gopts->share_thread_ratio = 0;
}

void setup() {

	gopts = malloc(sizeof(GOptions));
	set_default_params(gopts);
	mc = create_mutator_context(gopts);

}


void test_allocate_btree(void) {


	OCollection *oc = allocate_optimal_btree(mc, 34, 10);
	deallocate_optimal_btree(mc, oc);

}

void test_allocate_aligned(void) {

	
	Object *o = allocate_aligned(mc, 34, 64);

	assert( (63 & (long)o) == 0);

	deallocate_aligned(mc, o, 34, 64);

	allocate_optimal_list(mc, 34, 4);



}


int main (int argc, char **argv) {


	int sz, nelem, alignment;

	setup();


	assert(get_optimal_list_sz(16, 4, 64) == 64);
	assert(get_optimal_list_sz(16, 5, 64) == 128);
	assert(get_optimal_list_sz(33, 2, 64) == 128);
	assert(get_optimal_list_sz(65, 2, 64) == 256);
	assert(get_optimal_list_sz(128, 1, 64) == 128);
	assert(get_optimal_list_sz(129, 1, 64) == 192);
	assert(get_optimal_list_sz(127, 2, 64) == 256);
	assert(get_optimal_list_sz(5, 13, 64) == 128);
	assert(get_optimal_list_sz(17, 4, 64) == 128);



	test_allocate_aligned();

	test_allocate_btree();

	return EXIT_SUCCESS;
}

