#ifndef DISTRIBUTION_H
#define DISTRIBUTION_H

#include <glib.h>
#include "acdc.h"



//TODO: method description
GRand *init_rand(unsigned int seed);
void free_rand(GRand *rand);
void get_random_object_props(MContext *mc, 
		size_t *size, 
		unsigned int *lifetime, 
		unsigned int *num_objects);


#endif
