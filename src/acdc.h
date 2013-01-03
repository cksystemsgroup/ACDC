#ifndef ACDC_H
#define ACDC_H

#include <glib.h>

#include "collections.h"

//global acdc options
typedef enum {
  ACDC, //default mode
  FALSESHARING
} benchmark_mode_t;
typedef struct global_options GOptions;
struct global_options {
  //benchmark options
  benchmark_mode_t mode; //-m: acdc, false-sharing, ...
  int num_threads;  //-n: number of mutator threads
  int time_threshold; //-t: allocated bytes until time advances
  int benchmark_duration; //-d: How long acdc will run
  int seed; //-r:
  
  //options for object creation
  int min_lifetime; //-l: must be >= 1 and <= max_lifetime
  int max_lifetime; //-L:
  int min_object_sc; //-s: minimal sizeclass
  int max_object_sc; //-S: max sizeclass

  //sharing options
  int share_objects; //-O:
  int share_ratio; //-R: share_ratio% of all objects will be shared
  int share_thread_ratio; //-T: share_thread_ratio% of all threads will be involved

  //misc options
  int verbosity; //-v
};

//thread local mutator options
typedef struct mutator_options MOptions;
struct mutator_options {
  int thread_id;
  GRand *rand; //GLib's Mersenne Twister PRNG
};

//mutator measurement data
typedef struct mutator_stat MStat;
struct mutator_stat {
  unsigned long bytes_allocated;
  unsigned long bytes_deallocated;
  unsigned long objects_allocated;
  unsigned long objects_deallocated;
};

//thread context specific data
typedef struct mutator_context MContext;
struct mutator_context {
  GOptions *gopts; //pointer to global options. same for all threads
  MOptions opt; //thread local options
  MStat *stat; //mutator stats
  CollectionPool *collection_pools; //one pool for each possible lifetime
  unsigned int time;
};






void run_acdc(GOptions *gopts);







#endif
