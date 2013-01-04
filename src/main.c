#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "acdc.h"
#include "memory.h"

/*
struct global_options {
  //benchmark options
  int mode; //-m: acdc, false-sharing, ...
  int num_threads;  //-n: number of mutator threads
  int benchmark_duration; //-d: How long acdc will run
  int seed; //-r:
  
  //options for object creation
  int min_lifetime; //-l: must be >= 1 and <= max_lifetime
  int max_lifetime; //-L:
  int min_object_sc; //-s: minimal sizeclass
  int max_object_sc; //-S: max sizeclass

  //sharing options
  int share_objects; //-O
  int share_ratio; //-R: share_ratio% of all objects will be shared
  int share_thread_ratio; //-T: share_thread_ratio% of all threads will be involved

};
*/

static void print_usage() {
	printf("TODO: help message\n");
	exit(EXIT_FAILURE);
}

static void set_default_params(GOptions *gopts) {
	gopts->mode = ACDC;
	gopts->num_threads = 1;
	gopts->time_threshold = 1<<18;
	gopts->benchmark_duration = 100;
	gopts->seed = 1;
	gopts->min_lifetime = 1;
	gopts->max_lifetime = 20;
	gopts->min_object_sc = 4;
	gopts->max_object_sc = 10;
	gopts->share_objects = 0;
	gopts->share_ratio = 0;
	gopts->share_thread_ratio = 0;
}

static void check_params(GOptions *gopts) {
	//TODO; exit on wrong parameter settings
}


static void print_params(GOptions *gopts) {
	printf("gopts->mode = %d\n", gopts->mode);
	printf("gopts->num_threads = %d\n", gopts->num_threads);
	printf("gopts->time_threshold = %d\n", gopts->time_threshold);
	printf("gopts->benchmark_duration = %d\n", gopts->benchmark_duration);
	printf("gopts->seed = %d\n", gopts->seed);
	printf("gopts->min_lifetime = %d\n", gopts->min_lifetime);
	printf("gopts->max_lifetime = %d\n", gopts->max_lifetime);
	printf("gopts->min_object_sc = %d\n", gopts->min_object_sc);
	printf("gopts->max_object_sc = %d\n", gopts->max_object_sc);
	printf("gopts->share_objects = %d\n", gopts->share_objects);
	printf("gopts->share_ratio = %d\n", gopts->share_ratio);
	printf("gopts->share_thread_ratio = %d\n", gopts->share_thread_ratio);
}

int main(int argc, char **argv) {


	GOptions *gopts = malloc(sizeof(GOptions));
	set_default_params(gopts);
	const char *optString = "m:n:t:d:r:l:L:s:S:OR:T:vh";

	int opt = getopt(argc, argv, optString);
	while (opt != -1) {
		switch (opt) {
			case 'm':
				//TODO: set mode. Don't know if necessary
				break;
			case 'n':
				gopts->num_threads = atoi(optarg);
				break;
			case 't':
				gopts->time_threshold = atoi(optarg);
				break;
			case 'd':
				gopts->benchmark_duration = atoi(optarg);
				break;
			case 'r':
				gopts->seed = atoi(optarg);
				break;
			case 'l':
				gopts->min_lifetime = atoi(optarg);
				break;
			case 'L':
				gopts->max_lifetime = atoi(optarg);
				break;
			case 's':
				gopts->min_object_sc = atoi(optarg);
				break;
			case 'S':
				gopts->max_object_sc = atoi(optarg);
				break;
			case 'O':
				gopts->share_objects = 1;
				break;
			case 'R':
				gopts->share_ratio = atoi(optarg);
				break;
			case 'T':
				gopts->share_thread_ratio = atoi(optarg);
				break;
			case 'v':
				gopts->verbosity++;
				break;
			case 'h':
				print_usage(); //and exit
				break;
			default:
				break;

		}
		opt = getopt(argc, argv, optString);
	}

	check_params(gopts); //and exit on error

	print_params(gopts);

	run_acdc(gopts);


	free(gopts);
	return (EXIT_SUCCESS);
}

