#ifndef ACDC_H
#define ACDC_H

#include <sys/types.h>
#include <glib.h>

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


//memory object abstraction
/*
 * Some Makros to access the reference counter and the thread mask
 * out of the combined rctm field. RCTM creates the combination
 * of a RC and a TM
 */
#define RC(_rctm) (_rctm >> 58)
#define TM(_rctm) ((_rctm << 6) >> 6)
#define RCTM(_rc, _tm) ((_rc << 58) | _tm)


//object header for every allocated object
//the min. size for an object must be sizeof(Object)
typedef struct mem_object Object;
struct mem_object {
  u_int64_t rctm; //6bit rc, 58 bit thread map
};
typedef struct mem_object_lnode LObject;
struct mem_object_lnode {
  Object o;
  LObject *next;
};




//Collection stuff
typedef enum {LIST, TREE} collection_t;
//object pool where threads keep refs to the memory chunks
typedef struct object_collection OCollection;
struct object_collection {
  size_t object_size;
  size_t num_objects;
  unsigned int id; // in case we have more collections of the same size
  collection_t type;
  //pointer to start of collection
  Object *start;
};

typedef struct collection_pool CollectionPool;
struct collection_pool {
  unsigned int remaining_lifetime;
  GHashTable *collections; //hash table with one OCollection per size and id
};




typedef struct mutator_context MContext;

OCollection *allocate_collection(MContext *mc, collection_t ctype, size_t sz,
		unsigned long nelem);
void deallocate_collection(MContext *mc, OCollection *oc); 

//thread context specific data
struct mutator_context {
  GOptions *gopts; //pointer to global options. same for all threads
  MOptions opt; //thread local options
  MStat *stat; //mutator stats
  CollectionPool *collection_pools; //one pool for each possible lifetime
  unsigned int time;
};


void run_acdc(GOptions *gopts);



Object *allocate(MContext *mc, size_t size);
void deallocate(MContext *mc, Object *o, size_t size);
unsigned int get_sizeclass(size_t size);




#endif
