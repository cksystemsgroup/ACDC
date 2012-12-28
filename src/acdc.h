#ifndef ACDC_H
#define ACDC_H

#include <glib.h>

//global acdc options
typedef struct acdc_options GOptions;
struct global_options {
  //benchmark options
  int mode; //acdc, false-sharing, ...
  int num_threads;  //number of mutator threads
  int benchmark_duration; //How long acdc will run
  int seed;
  
  //options for object creation
  int max_lifetime;
  int min_object_sc; //minimal sizeclass
  int max_object_sc; //max sizeclass

  //sharing options
  int share_objects;
  int share_ratio; // share_ratio% of all objects will be shared
  int share_thread_ratio; //share_thread_ratio% of all threads will be involved

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
  unsigned int time;
};










#endif
