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
#include "memory.h"
#include "proc_status.h"

static void print_usage() {
	printf("ACDC Benchmark usage:\n"
			"\n"
			"-a (run in ACDC MODE)\n"
			"-f (run in FALSE_SHARING MODE)\n"
			" Options for all modes:\n"
			"-n number of threads\n"
			"-d benchmark duration\n"
			"-s min. sizeclass (1<<x)\n"
			"-S max. sizeclass (1<<x)\n"		
			"-r seed value\n"
			"-w write ratio\n"
			"-i write iterations.\n"
			"-k skip traversal\n"
			" Options for ACDC MODE:\n"
			"-t time quantum\n"
			"-l min. object lifetime\n"
			"-L max. object lifetime\n"
			"-D deallocation delay\n"
			"-g max. time gap\n"
			"-N node buffer size\n"
			"-O share objects\n"
			"-R share ratio\n"
			"-T share thread ratio\n"
			"-b %% ratio of btree collections\n"
			"-q %% ratio of linked-list collections\n"
			);
	exit(EXIT_FAILURE);
}

static void set_default_params(GOptions *gopts) {
	gopts->mode = ACDC;
	gopts->num_threads = 1;
	gopts->time_quantum = 1<<18;
	gopts->benchmark_duration = 100;
	gopts->seed = 1;
	gopts->min_lifetime = 1;
	gopts->max_lifetime = 10;
	gopts->max_time_gap = -1;
	gopts->deallocation_delay = 0;
	gopts->min_object_sc = 4;
	gopts->max_object_sc = 8;
	gopts->node_buffer_size = 1000; //TODO estimate from other parameters
	gopts->share_objects = 0;
	gopts->share_ratio = 0;
	gopts->share_thread_ratio = 100;
	gopts->list_ratio = 100;
	gopts->btree_ratio = 0;
	gopts->write_iterations = 1;
	gopts->write_ratio = 10; // 10 percent of all traversed objects are accessed too
	gopts->skip_traversal = 0;
	gopts->verbosity = 0;
	//gopts->false_sharing_ratio = 100;
}

static void check_params(GOptions *gopts) {
	//TODO; exit on wrong parameter settings
	
	if (gopts->list_ratio + 
			gopts->btree_ratio  != 100) {
		printf("If using -b and -q, their arguments must add"
				" up to 100%%\n");
		exit(EXIT_FAILURE);
	}

	if (gopts->mode == FS) {
		gopts->share_thread_ratio = 100;
		gopts->share_objects = 1;
		gopts->share_ratio = 100;
		gopts->skip_traversal = 0;
		gopts->write_ratio = 100;
	}
	if (gopts->max_time_gap < 0) gopts->max_time_gap = gopts->max_lifetime;
}


static void print_params(GOptions *gopts) {
	printf("gopts->mode = %d\n", gopts->mode);
	printf("gopts->num_threads = %d\n", gopts->num_threads);
	printf("gopts->time_quantum = %d\n", gopts->time_quantum);
	printf("gopts->benchmark_duration = %d\n", gopts->benchmark_duration);
	printf("gopts->seed = %d\n", gopts->seed);
	printf("gopts->min_lifetime = %d\n", gopts->min_lifetime);
	printf("gopts->max_lifetime = %d\n", gopts->max_lifetime);
	printf("gopts->max_time_gap = %d\n", gopts->max_time_gap);
	printf("gopts->deallocation_delay = %d\n", gopts->deallocation_delay);
	printf("gopts->min_object_sc = %d\n", gopts->min_object_sc);
	printf("gopts->max_object_sc = %d\n", gopts->max_object_sc);
	printf("gopts->node_buffer_size = %d\n", gopts->node_buffer_size);
	printf("gopts->list_ratio = %d\n", gopts->list_ratio);
	printf("gopts->btree_ratio = %d\n", gopts->btree_ratio);
	printf("gopts->write_iterations = %d\n", gopts->write_iterations);
	printf("gopts->write_ratio = %d\n", gopts->write_ratio);
	printf("gopts->skip_traversal = %d\n", gopts->skip_traversal);
	printf("gopts->share_objects = %d\n", gopts->share_objects);
	printf("gopts->share_ratio = %d\n", gopts->share_ratio);
	printf("gopts->share_thread_ratio = %d\n", gopts->share_thread_ratio);
	printf("gopts->verbosity = %d\n", gopts->verbosity);
}

int main(int argc, char **argv) {

	//initial_break = sbrk(0);


	GOptions *gopts = malloc(sizeof(GOptions));

	gopts->pid = getpid();

	set_default_params(gopts);
	const char *optString = "afn:t:d:r:l:L:D:g:s:S:N:OR:T:b:q:i:w:kvh";

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
			case 'N':
				gopts->node_buffer_size = atoi(optarg);
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
			case 'i':
				gopts->write_iterations = atoi(optarg);
				break;
			case 'w':
				gopts->write_ratio = atoi(optarg);
				break;
			case 'k':
				gopts->skip_traversal = 1;
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

