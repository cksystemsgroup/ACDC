#include <stdlib.h>

#include "acdc.h"
#include "distribution.h"
#include "memory.h"


MContext *create_mutator_context(GOptions *gopts, unsigned int thread_id) {
	
	unsigned int seed = gopts->seed + thread_id;

	MContext *mc = malloc(sizeof(MContext));
	mc->gopts = gopts;
	
	mc->opt.rand = init_rand(seed);
	mc->opt.thread_id = thread_id;

	mc->stat = malloc(sizeof(MStat));
	mc->stat->bytes_allocated = 0;
	mc->stat->bytes_deallocated = 0;
	mc->stat->objects_allocated = 0;
	mc->stat->objects_deallocated = 0;

}

void destroy_mutator_context(MContext *mc) {

	free_rand(mc->opt.rand);
	free(mc->stat);
	free(mc);
}


