#include <glib.h>
#include <stdio.h>

#include "acdc.h"
#include "memory.h"


GRand *init_rand(unsigned int seed) {
	return g_rand_new_with_seed(seed);
}
void free_rand(GRand *rand) {
	g_rand_free(rand);
}


static unsigned int get_random_lifetime(MContext *mc) {

	return g_rand_int_range(mc->opt.rand,
			mc->gopts->min_lifetime,
			mc->gopts->max_lifetime + 1);
}
static unsigned int get_random_size(MContext *mc) {
	unsigned int sc = g_rand_int_range(mc->opt.rand,
			mc->gopts->min_object_sc,
			mc->gopts->max_object_sc + 1);

	return g_rand_int_range(mc->opt.rand,
			1 << sc,
			1 << (sc + 1));
}

unsigned int get_random_thread(MContext *mc) {

	return g_rand_int_range(mc->opt.rand,
			0,
			mc->gopts->num_threads);
}

static collection_t get_random_collection_type(MContext *mc) {

	unsigned int r = g_rand_int_range(mc->opt.rand, 0, 100);

	if (r < mc->gopts->list_ratio) {
		return LIST;
	}
	if (r < (mc->gopts->list_ratio + mc->gopts->btree_ratio)) {
		return BTREE;
	}
	if (r < (mc->gopts->list_ratio + 
				mc->gopts->btree_ratio +
				mc->gopts->false_sharing_ratio)) {
		return FALSE_SHARING;
	}
	
	//default. never reached
	return LIST;
}

static u_int64_t get_random_thread_selection(MContext *mc) {

	u_int64_t my_thread_bit = 1 << mc->opt.thread_id;

	if (mc->gopts->share_objects == 0 || 
			mc->gopts->share_thread_ratio == 0) {
		//only this thread is interested
		return my_thread_bit;
	}

	//threads except me, times share ratio
	int number_of_other_threads = 
		(mc->gopts->num_threads - 1) / (100 / mc->gopts->share_thread_ratio);

	//printf("%d will share with %d threads\n", mc->opt.thread_id, 
	//		number_of_other_threads);

	//get number_of_other_threads random thread id's (except mine)
	//and set their bits in tm
	int i = 0;
	u_int64_t tm = my_thread_bit;
	while (i < number_of_other_threads) {
		int tid = get_random_thread(mc);
		//check if we already haven't added this thread
		if (!(tm & (1 << tid))) {
	//		printf("adding thread %d\n", tid);
			++i;
			tm |= 1 << tid;
		}
	}

	return tm;
}

void get_random_object_props(MContext *mc, 
		size_t *size, 
		unsigned int *lifetime, 
		unsigned int *num_objects,
		collection_t *type,
		u_int64_t *rctm) {

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
	//*num_objects = 10;
	*type = get_random_collection_type(mc);
	*rctm = get_random_thread_selection(mc);
}






