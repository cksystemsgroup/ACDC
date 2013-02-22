 /*
 * Copyright (c) 2012, the ACDC Project Authors.
 * All rights reserved. Please see the AUTHORS file for details.
 * Use of this source code is governed by a BSD license that
 * can be found in the LICENSE file.
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "acdc.h"
#include "metadata-allocator.h"
#include "proc_status.h"

static void print_usage() {
	printf("ACDC Benchmark usage:\n"
			"\n"
			"-a (run in ACDC mode)\n"
			"-f (run in false-sharing mode)\n"
			"-n: number of threads (default 1)\n"
			"-F: (fixed) number of objects (default 0: ACDC decides)\n"
			"-s: min. size (in 2^x bytes)\n"
			"-S: max. size (in 2^x bytes)\n"		
			"-l: min. lifetime\n"
			"-L: max. lifetime\n"
			"-D: deallocation delay\n"
			"-t: time quantum\n"
			"-d: benchmark duration\n"
			"-g: max. time drift\n"
			"-q: list-based ratio in %%\n"
			"-A access live objects\n"
			"-w: write access ratio in %%\n"
			"-O shared objects\n"
			"-R: shared objects ratio\n"
			"-T: receiving threads ratio\n"
			"-r: seed value\n"
			"-i: write iterations.\n"
			"-H: meta data heap size in kB\n"
			"-N: node buffer size\n"
			"-C: class buffer size\n"
			);
	exit(EXIT_FAILURE);
}

static void set_default_params(GOptions *gopts) {
	gopts->mode = ACDC;
	gopts->num_threads = 1;
	gopts->time_quantum = 1<<18;
	gopts->benchmark_duration = 100;
	gopts->seed = 1;
	gopts->metadata_heap_sz = 4096; //4MB
	gopts->min_lifetime = 1;
	gopts->max_lifetime = 10;
	gopts->max_time_gap = -1;
	gopts->deallocation_delay = 0;
	gopts->min_object_sc = 4;
	gopts->max_object_sc = 8;
	gopts->fixed_number_of_objects = 0;
	gopts->node_buffer_size = 1000; //TODO estimate from other parameters
	gopts->class_buffer_size = 1000; //TODO estimate from other parameters
	gopts->shared_objects = 0;
	gopts->shared_objects_ratio = 0;
	gopts->receiving_threads_ratio = 100;
	gopts->list_based_ratio = 100;
	gopts->btree_based_ratio = 0;
	gopts->write_iterations = 1;
	gopts->write_access_ratio = 10; // 10 percent of all traversed objects are accessed too
	gopts->access_live_objects = 0;
	gopts->verbosity = 0;
}

static void check_params(GOptions *gopts) {
	//TODO:check missing parameters
	if (gopts->list_based_ratio < 0 || 
			gopts->list_based_ratio  > 100) {
		printf("Parameter error: -q value must be between 0 and 100\n");
		exit(EXIT_FAILURE);
	}
	gopts->btree_based_ratio = 100 - gopts->list_based_ratio;

	if (gopts->mode == FS) {
		gopts->receiving_threads_ratio = 100;
		gopts->shared_objects = 1;
		gopts->shared_objects_ratio = 100;
		gopts->access_live_objects = 0;
		gopts->write_access_ratio = 100;
	}
	if (gopts->max_time_gap < 0) gopts->max_time_gap = gopts->max_lifetime;


#ifdef __GCC_HAVE_SYNC_COMPARE_AND_SWAP_16
	int max_threads = 128;
#else
	int max_threads = 64;
#endif
	if (gopts->num_threads > max_threads) {
		printf("Parameter error: -n value must be between 0 and %d\n",
				max_threads);
		exit(EXIT_FAILURE);
	}
}


static void print_params(GOptions *gopts) {
	printf("gopts->mode = %d\n", gopts->mode);
	printf("gopts->num_threads = %d\n", gopts->num_threads);
	printf("gopts->time_quantum = %d\n", gopts->time_quantum);
	printf("gopts->benchmark_duration = %d\n", gopts->benchmark_duration);
	printf("gopts->seed = %d\n", gopts->seed);
	printf("gopts->metadata_heap_sz = %lu\n", gopts->metadata_heap_sz);
	printf("gopts->min_lifetime = %d\n", gopts->min_lifetime);
	printf("gopts->max_lifetime = %d\n", gopts->max_lifetime);
	printf("gopts->max_time_gap = %d\n", gopts->max_time_gap);
	printf("gopts->deallocation_delay = %d\n", gopts->deallocation_delay);
	printf("gopts->min_object_sc = %d\n", gopts->min_object_sc);
	printf("gopts->max_object_sc = %d\n", gopts->max_object_sc);
	printf("gopts->fixed_number_of_objects = %d\n", gopts->fixed_number_of_objects);
	printf("gopts->node_buffer_size = %d\n", gopts->node_buffer_size);
	printf("gopts->class_buffer_size = %d\n", gopts->class_buffer_size);
	printf("gopts->list_based_ratio = %d\n", gopts->list_based_ratio);
	printf("gopts->write_iterations = %d\n", gopts->write_iterations);
	printf("gopts->write_access_ratio = %d\n", gopts->write_access_ratio);
	printf("gopts->access_live_objects = %d\n", gopts->access_live_objects);
	printf("gopts->shared_objects = %d\n", gopts->shared_objects);
	printf("gopts->shared_objects_ratio = %d\n", gopts->shared_objects_ratio);
	printf("gopts->receiving_threads_ratio = %d\n", gopts->receiving_threads_ratio);
	printf("gopts->verbosity = %d\n", gopts->verbosity);
}

int main(int argc, char **argv) {

	GOptions *gopts = sbrk(sizeof(GOptions));	
	//GOptions *gopts = malloc(sizeof(GOptions));	
	if (gopts == (void*)-1) {
		printf("unable to allocate global options\n");
		exit(EXIT_FAILURE);
	}

	gopts->pid = getpid();

	set_default_params(gopts);
	const char *optString = "afn:t:d:r:H:l:L:D:g:s:S:F:N:C:OR:T:q:i:w:Avh";

	int opt = getopt(argc, argv, optString);
	while (opt != -1) {
		switch (opt) {
			case 'a':
				gopts->mode = ACDC;
				break;
			case 'f':
				gopts->mode = FS;
				break;
			case 'n':
				gopts->num_threads = atoi(optarg);
				break;
			case 't':
				gopts->time_quantum = atoi(optarg);
				break;
			case 'd':
				gopts->benchmark_duration = atoi(optarg);
				break;
			case 'r':
				gopts->seed = atoi(optarg);
				break;
			case 'H':
				gopts->metadata_heap_sz = atoi(optarg);
				break;
			case 'l':
				gopts->min_lifetime = atoi(optarg);
				break;
			case 'L':
				gopts->max_lifetime = atoi(optarg);
				break;
			case 'D':
				gopts->deallocation_delay = atoi(optarg);
				break;
			case 'g':
				gopts->max_time_gap = atoi(optarg);
				break;
			case 's':
				gopts->min_object_sc = atoi(optarg);
				break;
			case 'S':
				gopts->max_object_sc = atoi(optarg);
				break;
			case 'F':
				gopts->fixed_number_of_objects = atoi(optarg);
				break;
			case 'N':
				gopts->node_buffer_size = atoi(optarg);
				break;
			case 'C':
				gopts->class_buffer_size = atoi(optarg);
				break;
			case 'O':
				gopts->shared_objects = 1;
				break;
			case 'R':
				gopts->shared_objects_ratio = atoi(optarg);
				break;
			case 'T':
				gopts->receiving_threads_ratio = atoi(optarg);
				break;
			case 'q':
				gopts->list_based_ratio = atoi(optarg);
				break;
			case 'i':
				gopts->write_iterations = atoi(optarg);
				break;
			case 'w':
				gopts->write_access_ratio = atoi(optarg);
				break;
			case 'A':
				gopts->access_live_objects = 1;
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

	init_metadata_heap(gopts->metadata_heap_sz);

	run_acdc(gopts);

	return (EXIT_SUCCESS);
}

