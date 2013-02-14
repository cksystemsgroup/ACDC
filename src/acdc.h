/*
 * Copyright (c) 2012, the ACDC Project Authors.
 * All rights reserved. Please see the AUTHORS file for details.
 * Use of this source code is governed by a BSD license that
 * can be found in the LICENSE file.
 */

#ifndef ACDC_H
#define ACDC_H

#include <pthread.h>
#include <sys/types.h>

#define debug(...) _debug(__FILE__, __LINE__, __VA_ARGS__)
void _debug(char *filename, int linenum, const char *format, ...);

//global acdc options
typedef enum {
  ACDC, //default mode
  FS
} benchmark_mode_t;
typedef struct global_options GOptions;
struct global_options {
  //benchmark options
  benchmark_mode_t mode; //-m: acdc, false-sharing, ...
  int num_threads;  //-n: number of mutator threads
  int time_quantum; //-t: allocated bytes until time advances
  int benchmark_duration; //-d: How long acdc will run
  int seed; //-r:
  size_t metadata_heap_sz; //-H: in kB
  
  //options for object creation
  int min_lifetime; //-l: must be >= 1 and <= max_lifetime
  int max_lifetime; //-L:
  int fixed_number_of_objects; //-F:
  int deallocation_delay; //-D:
  int max_time_gap; //-g: defaults to max_lifetime
  int min_object_sc; //-s: minimal sizeclass
  int max_object_sc; //-S: max sizeclass
  int list_ratio; //-q:
  int btree_ratio; //derived from -q
  int node_buffer_size; //-N: used to recycle nodes for LSClasses
  int class_buffer_size; //-C: used to recycle nodes for LSClasses
  
  //options for object access
  int write_iterations; //-i:
  int access_live_objects; //-A
  int write_access_ratio; //-w: 

  //sharing options
  int share_objects; //-O:
  int share_ratio; //-R: share_ratio% of all objects will be shared
  int share_thread_ratio; //-T: share_thread_ratio% of all threads will be involved

  //misc options
  int verbosity; //-v
  pid_t pid;
};


//mutator measurement data
typedef struct mutator_stat MStat;
struct mutator_stat {
  unsigned long long running_time;
  unsigned long long allocation_time;
  unsigned long long deallocation_time;
  unsigned long long access_time;
  unsigned long bytes_allocated;
  unsigned long bytes_deallocated;
  unsigned long objects_allocated;
  unsigned long objects_deallocated;
  unsigned long *lt_histogram;
  unsigned long *sz_histogram;
  long vm_peak;
  long rss_hwm;
  long current_rss;
  long resident_set_size_counter;
};


//memory object abstraction

//object header for every allocated object
//the min. size for an object must be sizeof(Object)
//typedef struct mem_object Object;
typedef void Object;
typedef struct shared_mem_object SharedObject;
struct shared_mem_object {
  u_int64_t sharing_map; 
  //a bit at pos i indicates that thread i may access this object
};
typedef struct mem_object_lnode LObject;
struct mem_object_lnode {
  //Object o;
  LObject *next;
};
typedef struct mem_object_btnode BTObject;
struct mem_object_btnode {
  //Object o;
  BTObject *left;
  BTObject *right;
};



//Collection stuff
typedef enum {
  LIST,
  BTREE,
  FALSE_SHARING,
  OPTIMAL_LIST, 
  OPTIMAL_BTREE,
  OPTIMAL_FALSE_SHARING
} collection_type;

//set of objects with common size and lifetime
typedef struct lifetime_size_class LSClass;
struct lifetime_size_class {
  size_t object_size;
  unsigned int lifetime;
  size_t num_objects;
  collection_type type;

  //which threads should share an object
  volatile u_int64_t sharing_map;

  //mark which threads already have this OColelction
  volatile u_int64_t reference_map;
  
  //pointer to the start of the objects
  Object *start;
};

typedef struct lifetime_size_class_node LSCNode;
struct lifetime_size_class_node {
  LSCNode *prev;
  LSCNode *next;
  LSClass *ls_class;
};

//list of LSClasses with the same lifetime
typedef struct lifetime_class {
  LSCNode *first;
  LSCNode *last;
} LClass;

typedef struct mutator_context MContext;

LSClass *allocate_LSClass(MContext *mc, collection_type ctype, size_t sz,
		unsigned long nelem, u_int64_t sharing_map);

void deallocate_LSClass(MContext *mc, LSClass *oc); 
void traverse_LSClass(MContext *mc, LSClass *oc);


//thread context specific data
struct mutator_context {
  GOptions *gopts; //pointer to global options. same for all threads
  int thread_id;
  int rand;
  MStat *stat; //mutator stats
  unsigned int time;
  LClass *expiration_class; // one LClass for each possible lifetime

  LSCNode *node_buffer_memory;
  int node_buffer_counter;
  LClass node_cache;

  LSClass *class_buffer_memory;
  int class_buffer_counter;
  LClass class_cache;
};


void lclass_insert_after(LClass *list, LSCNode *after, LSCNode *c);
void lclass_insert_before(LClass *list, LSCNode *before, LSCNode *c);
void lclass_insert_beginning(LClass *list, LSCNode *c);
void lclass_insert_end(LClass *list, LSCNode *c);
void lclass_remove(LClass *list, LSCNode *c);

void run_acdc(GOptions *gopts);

void init_metadata_heap(size_t heapsize);
void *malloc_meta(size_t size);
void *calloc_meta(size_t nelem, size_t size);
void *malloc_meta_aligned(size_t size, size_t alignment);
void *calloc_meta_aligned(size_t nelem, size_t size, size_t alignment);

Object *allocate(MContext *mc, size_t size);
void deallocate(MContext *mc, Object *o, size_t size);
Object *allocate_aligned(MContext *mc, size_t size, size_t alignment);
void deallocate_aligned(MContext *mc, Object *o, size_t size, size_t alignment);
void write_object(Object *o, size_t size, size_t offset);
unsigned int get_sizeclass(size_t size);

void get_random_object_props(MContext *mc, 
		size_t *size, 
		unsigned int *lifetime, 
		unsigned int *num_objects,
    collection_type *type,
    u_int64_t *sharing_map
    );

unsigned int get_random_thread(MContext *mc);


#endif
