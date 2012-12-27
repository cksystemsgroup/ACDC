#ifndef ACDC_H
#define ACDC_H

//global acdc options
typedef struct acdc_options GOptions;
struct global_options {
  int num_threads;  //number of mutator threads

};

//thread local mutator options
typedef struct mutator_options MOptions;
struct mutator_options {
  int thread_id;
};

//thread context specific data
typedef struct mutator_context MContext;
struct mutator_context {
  GOptions *gopts; //pointer to global options. same for all threads
  MOptions opt; //thread local options
  unsigned int time;
};

#endif
