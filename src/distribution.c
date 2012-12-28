#include <glib.h>

#include "distribution.h"


GRand *init_rand(unsigned int seed) {
	return g_rand_new_with_seed(seed);
}
void free_rand(GRand *rand) {
	g_rand_free(rand);
}


unsigned int get_random_lifetime(MContext *mc, 
                                 unsigned int min, 
                                 unsigned int max) {


	return 0;
}
