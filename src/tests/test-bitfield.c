#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "../acdc.h"

GOptions *gopts;
MContext *mc;

void set_default_params(GOptions *gopts) {
	gopts->mode = ACDC;
	gopts->num_threads = 1;
	gopts->time_quantum = 1<<18;
	gopts->benchmark_duration = 100;
	gopts->seed = 1;
	gopts->min_liveness = 1;
	gopts->max_liveness = 10;
	gopts->min_object_sc = 4;
	gopts->max_object_sc = 8;
	gopts->shared_objects = 0;
	gopts->shared_objects_ratio = 0;
	gopts->receiving_threads_ratio = 0;
}

void setup() {

	gopts = (GOptions*)malloc(sizeof(GOptions));
	set_default_params(gopts);
//      mc = (MContext*)create_mutator_context();

}


int main (int argc, char **argv) {


	int sz, nelem, alignment;

//	setup();


	//assert(get_optimal_list_sz(17, 4, 64) == 128);


        ReferenceMap *reference_map = malloc(sizeof(ReferenceMap));

        int i;
        for (i = 0; i < 20; i+=2) {
                set_bit(reference_map->thread_map, i);
        }
        for (i = 0; i < 20; i++) {
                int r = get_bit(reference_map->thread_map, i);
                printf("%d ", r);
        }
        printf("\n");
        
        for (i = 0; i < 20; i++) {
                int r = get_bit(reference_map->thread_map, i);
                if (r) {
                        clear_bit(reference_map->thread_map, i);
                } else {
                        set_bit(reference_map->thread_map, i);
                }
        }
        for (i = 0; i < 20; i++) {
                int r = get_bit(reference_map->thread_map, i);
                printf("%d ", r);
        }
        printf("\n");
        for (i = 0; i < 20; i++) {
                addReference(reference_map, i);
        }
        for (i = 0; i < 20; i++) {
                int r = get_bit(reference_map->thread_map, i);
                printf("%d ", r);
        }

        printf("\n");



	return EXIT_SUCCESS;
}

