 /*
 * Copyright (c) 2012, the ACDC Project Authors.
 * All rights reserved. Please see the AUTHORS file for details.
 * Use of this source code is governed by a BSD license that
 * can be found in the LICENSE file.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "acdc.h"
#include "caches.h"
#include "metadata-allocator.h"
#include "proc-status.h"
#include "alloc/nulloc.h"

static void print_usage() {
	printf("ACDC Benchmark usage:\n"
			"\n"
			"-a (run in ACDC mode)\n"
			"-f (run in false-sharing mode)\n"
			"-n: number of threads (default 1)\n"
			"-F: (fixed) number of objects (default 0: ACDC decides)\n"
			"-s: min. size (in 2^x bytes)\n"
			"-S: max. size (in 2^x bytes)\n"		
			"-l: min. liveness\n"
			"-L: max. liveness\n"
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
			"-H: meta data heap size in MB\n"
			"-N: node buffer size in number of nodes\n"
			"-C: class buffer size in number of nodes\n"
			"-P: allocator name (P)rinted at the summary\n"
			"-W: warmup metadata space and exclude it from the memory results\n"
			);
	exit(EXIT_FAILURE);
}

static void set_default_params(GOptions *gopts) {
	gopts->mode = ACDC;
	gopts->num_threads = 1;
	gopts->time_quantum = 1<<18;
	gopts->benchmark_duration = 100;
	gopts->seed = 1;
	gopts->metadata_heap_sz = 0; //0 means that the user did not give that option
	gopts->min_liveness = 1;
	gopts->max_liveness = 10;
	gopts->max_time_gap = -1;
	gopts->deallocation_delay = 0;
	gopts->min_object_sc = 4;
	gopts->max_object_sc = 8;
	gopts->fixed_number_of_objects = 0;
	gopts->node_buffer_size = 0; //0 means that the user did not give that option
	gopts->class_buffer_size = 0; //0 means that the user did not give that option
	gopts->shared_objects = 0;
	gopts->shared_objects_ratio = 0;
	gopts->receiving_threads_ratio = 100;
	gopts->list_based_ratio = 100;
	gopts->btree_based_ratio = 0;
	gopts->write_iterations = 1;
	gopts->write_access_ratio = 10; // 10 percent of all traversed objects are accessed too
	gopts->access_live_objects = 0;
	gopts->verbosity = 0;
	gopts->allocator_name = "UNDEFINED";
        gopts->do_metadata_warmup = 0;
        gopts->do_baseline_rss = 0;
}

/* 
 * this function takes the expected values of the random variables liveness
 *  and object size to estimate the space required for the metadata
 */
static void autodetect_metadata_parameters(GOptions *gopts) {

	double expected_lt = (double)(gopts->max_liveness + gopts->min_liveness) / 2.0;
	double expected_sc = (double)(gopts->max_object_sc + gopts->min_object_sc) / 2.0;
	double expected_sz = (double)((1 << (int)expected_sc) + 
				(1 << (int)(expected_sc + 1))) / 2.0;

	// per lifetime-size-class
	double expected_num_obj = (double)(gopts->max_liveness - expected_lt + 1);
	expected_num_obj *= (gopts->max_liveness - expected_lt + 1);
	expected_num_obj *= (gopts->max_object_sc - expected_sc + 1);
	expected_num_obj *= (gopts->max_object_sc - expected_sc + 1);

	double expected_lifetime_size_class_size = expected_sz * expected_num_obj;

	if (expected_lifetime_size_class_size > gopts->time_quantum) {
		printf("WARNING: The expected size of a single lifetime-size-class "
				"is larger than the time quantum (%lu > %lu). "
				" Use a larger time quantum!\n", 
				(long unsigned)expected_lifetime_size_class_size,
				gopts->time_quantum);
		printf("Autodetection of metadata heap size will fail!\n"
				"Try -H, -N, -C options instead if you want to use "
				"a small quantum in combination with large objects\n");
		exit(EXIT_FAILURE);
	}


	double receiving_threads_num = 
			(double)(gopts->num_threads * gopts->receiving_threads_ratio) 
			/ 100.0;
	double shared_objects_multiplier = 1 + 
			(double)(gopts->num_threads * gopts->shared_objects_ratio) 
			/ 100.0;

	// per thread, mind for the overapproximation
	double expected_lifetime_size_classes = 
		2 * (expected_lt * (double)gopts->time_quantum) / 
		expected_lifetime_size_class_size;

	if (gopts->shared_objects) {
		//mind the gap :)
		expected_lifetime_size_classes *=
			((double)gopts->max_time_gap + expected_lt) * shared_objects_multiplier;
	} else {
		expected_lifetime_size_classes *= expected_lt;
	}

	// add some space in case the distribution does not satisfy the expected
	// value for short runs
	gopts->class_buffer_size = (int)expected_lifetime_size_classes * 2;

	if (gopts->shared_objects) {
		gopts->node_buffer_size = gopts->class_buffer_size + (int)(
			(double)gopts->class_buffer_size * 
			receiving_threads_num);
	} else {
		gopts->node_buffer_size = gopts->class_buffer_size;
	}
	
	//1MB per thread for bookkeeping
	gopts->metadata_heap_sz = gopts->num_threads;

        //printf("size 1: %lu\n", gopts->metadata_heap_sz);
	
	//global metadata heap
	//add the buffers for nodes and classes, add extra space for aligning ect...
	gopts->metadata_heap_sz += (((
		gopts->class_buffer_size * L1_LINE_SZ +
		gopts->node_buffer_size * L1_LINE_SZ) *
		gopts->num_threads) / (1024*1024));
        
        //printf("class size 2: %lu\n", gopts->class_buffer_size * L1_LINE_SZ);
        //printf("node size 2: %lu\n", gopts->node_buffer_size * L1_LINE_SZ);
        //printf("class * threads size 2: %lu\n", gopts->class_buffer_size * L1_LINE_SZ * gopts->num_threads);
        //printf("node * threads size 2: %lu\n", gopts->node_buffer_size * L1_LINE_SZ * gopts->num_threads);
        //printf("size 2: %lu\n", gopts->metadata_heap_sz);
}

static void check_params(GOptions *gopts) {
	if (gopts->list_based_ratio < 0 || 
			gopts->list_based_ratio  > 100) {
		printf("Parameter error: -q value must be between 0 and 100\n");
		exit(EXIT_FAILURE);
	}
	gopts->btree_based_ratio = 100 - gopts->list_based_ratio;

	if (gopts->min_object_sc >= gopts->max_object_sc) {
		printf("max. size must be larger than min. size\n");
	}

	if (gopts->mode == FS) {
		gopts->receiving_threads_ratio = 100;
		gopts->shared_objects = 1;
		gopts->shared_objects_ratio = 100;
		gopts->access_live_objects = 0;
		gopts->write_access_ratio = 100;
	}
	if (gopts->max_time_gap < 0) gopts->max_time_gap = gopts->max_liveness;
/*
	if (gopts->num_threads > MAX_NUM_THREADS) {
		printf("Parameter error: -n value must be between 1 and %lu\n",
				MAX_NUM_THREADS);
		exit(EXIT_FAILURE);
	}
*/
	if (gopts->metadata_heap_sz == 0 ||
			gopts->node_buffer_size == 0 ||
			gopts->class_buffer_size == 0) {
	
	
		if ((gopts->metadata_heap_sz +
				gopts->node_buffer_size +
				gopts->class_buffer_size) != 0) {
			printf("Parameter error: Specify -H, -N, AND -C or none of them for automatic parameter selection\n");
			exit(EXIT_FAILURE);
		} else {
			autodetect_metadata_parameters(gopts);
		}

	}
}

void set_allocation_pointers(GOptions *gopts) {
        
        //default
        acdc_alloc = malloc;
        acdc_free = free;
        
        if (strncmp(gopts->allocator_name, "nulloc", strlen("nulloc")) == 0) {
                acdc_alloc = nulloc_alloc;
                acdc_free = nulloc_free;
                gopts->do_baseline_rss = 1;
        }
}

static void print_params(GOptions *gopts) {
	printf("gopts->mode = %d\n", gopts->mode);
	printf("gopts->num_threads = %d\n", gopts->num_threads);
	printf("gopts->time_quantum = %lu\n", gopts->time_quantum);
	printf("gopts->benchmark_duration = %d\n", gopts->benchmark_duration);
	printf("gopts->seed = %d\n", gopts->seed);
	printf("gopts->metadata_heap_sz = %lu MB\n", gopts->metadata_heap_sz);
	printf("gopts->min_liveness = %d\n", gopts->min_liveness);
	printf("gopts->max_liveness = %d\n", gopts->max_liveness);
	printf("gopts->max_time_gap = %d\n", gopts->max_time_gap);
	printf("gopts->deallocation_delay = %d\n", gopts->deallocation_delay);
	printf("gopts->min_object_sc = %d\n", gopts->min_object_sc);
	printf("gopts->max_object_sc = %d\n", gopts->max_object_sc);
	printf("gopts->fixed_number_of_objects = %d\n", gopts->fixed_number_of_objects);
	printf("gopts->node_buffer_size = %lu\n", gopts->node_buffer_size);
	printf("gopts->class_buffer_size = %lu\n", gopts->class_buffer_size);
	printf("gopts->list_based_ratio = %d\n", gopts->list_based_ratio);
	printf("gopts->write_iterations = %d\n", gopts->write_iterations);
	printf("gopts->write_access_ratio = %d\n", gopts->write_access_ratio);
	printf("gopts->access_live_objects = %d\n", gopts->access_live_objects);
	printf("gopts->shared_objects = %d\n", gopts->shared_objects);
	printf("gopts->shared_objects_ratio = %d\n", gopts->shared_objects_ratio);
	printf("gopts->receiving_threads_ratio = %d\n", gopts->receiving_threads_ratio);
	printf("gopts->allocator_name = %s\n", gopts->allocator_name);
	printf("gopts->do_metadata_warmup = %d\n", gopts->do_metadata_warmup);
	printf("gopts->verbosity = %d\n", gopts->verbosity);
}

int main(int argc, char **argv) {

        GOptions gopts;

	gopts.pid = getpid();

	set_default_params(&gopts);
	const char *optString = "afn:t:d:r:H:l:L:D:g:s:S:F:N:C:OR:T:q:i:w:AP:Wvh";

	int opt = getopt(argc, argv, optString);
	while (opt != -1) {
		switch (opt) {
			case 'a':
				gopts.mode = ACDC;
				break;
			case 'f':
				gopts.mode = FS;
				break;
			case 'n':
				gopts.num_threads = atoi(optarg);
				break;
			case 't':
				gopts.time_quantum = atol(optarg);
				break;
			case 'd':
				gopts.benchmark_duration = atoi(optarg);
				break;
			case 'r':
				gopts.seed = atoi(optarg);
				break;
			case 'H':
				gopts.metadata_heap_sz = atoi(optarg);
				break;
			case 'l':
				gopts.min_liveness = atoi(optarg);
				break;
			case 'L':
				gopts.max_liveness = atoi(optarg);
				break;
			case 'D':
				gopts.deallocation_delay = atoi(optarg);
				break;
			case 'g':
				gopts.max_time_gap = atoi(optarg);
				break;
			case 's':
				gopts.min_object_sc = atoi(optarg);
				break;
			case 'S':
				gopts.max_object_sc = atoi(optarg);
				break;
			case 'F':
				gopts.fixed_number_of_objects = atoi(optarg);
				break;
			case 'N':
				gopts.node_buffer_size = atoi(optarg);
				break;
			case 'C':
				gopts.class_buffer_size = atoi(optarg);
				break;
			case 'O':
				gopts.shared_objects = 1;
				break;
			case 'R':
				gopts.shared_objects_ratio = atoi(optarg);
				break;
			case 'T':
				gopts.receiving_threads_ratio = atoi(optarg);
				break;
			case 'q':
				gopts.list_based_ratio = atoi(optarg);
				break;
			case 'i':
				gopts.write_iterations = atoi(optarg);
				break;
			case 'w':
				gopts.write_access_ratio = atoi(optarg);
				break;
			case 'A':
				gopts.access_live_objects = 1;
				break;
			case 'W':
				gopts.do_metadata_warmup = 1;
				break;
			case 'P':
				gopts.allocator_name = optarg;
				break;
			case 'v':
				gopts.verbosity++;
				break;
			case 'h':
				print_usage(); //and exit
				break;
			default:
				break;

		}
		opt = getopt(argc, argv, optString);
	}

	check_params(&gopts); //and exit on error

        set_allocation_pointers(&gopts);

	print_params(&gopts);

	init_metadata_heap(gopts.metadata_heap_sz, gopts.do_metadata_warmup);

	run_acdc(&gopts);

	return (EXIT_SUCCESS);
}

