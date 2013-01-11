#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "acdc.h"
#include "memory.h"

static void print_usage() {
	printf("ACDC Benchmark usage:\n"
			"\n"
			"-m mode\n"
			"-n number of threads\n"
			"-d benchmark duration\n"
			"-t time threshold\n"
			"-r seed value\n"
			"-l min. object lifetime\n"
			"-L max. object lifetime\n"
			"-s min. sizeclass (1<<x)\n"
			"-S max. sizeclass (1<<x)\n"
			"-O share objects\n"
			"-R share ratio\n"
			"-T share thread ratio\n"
			"-b %% ratio of btrees\n"
			"-q %% ratio of linked lists (queues)\n"
			);
	exit(EXIT_FAILURE);
}

static void set_default_params(GOptions *gopts) {
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
	gopts->list_ratio = 0;
	gopts->btree_ratio = 0;
	gopts->false_sharing_ratio = 100;
}

static void check_params(GOptions *gopts) {
	//TODO; exit on wrong parameter settings
	
	if (gopts->list_ratio + 
			gopts->btree_ratio +
			gopts->false_sharing_ratio != 100) {
		printf("If using -b, -f and -q, their arguments must add"
				" up to 100%%\n");
		exit(EXIT_FAILURE);
	}
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
	printf("gopts->list_ratio = %d\n", gopts->list_ratio);
	printf("gopts->btree_ratio = %d\n", gopts->btree_ratio);
	printf("gopts->false_sharing_ratio = %d\n", gopts->false_sharing_ratio);
	printf("gopts->share_objects = %d\n", gopts->share_objects);
	printf("gopts->share_ratio = %d\n", gopts->share_ratio);
	printf("gopts->share_thread_ratio = %d\n", gopts->share_thread_ratio);
}

int main(int argc, char **argv) {

	//initial_break = sbrk(0);


	GOptions *gopts = malloc(sizeof(GOptions));
	set_default_params(gopts);
	const char *optString = "m:n:t:d:r:l:L:s:S:OR:T:b:q:f:vh";

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
			case 'b':
				gopts->btree_ratio = atoi(optarg);
				break;
			case 'q':
				gopts->list_ratio = atoi(optarg);
				break;
			case 'f':
				gopts->false_sharing_ratio = atoi(optarg);
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

