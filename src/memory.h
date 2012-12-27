#ifndef MEMORY_H
#define MEMORY_H

#include <sys/types.h>

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
  size_t size; //the allocated size of the object
  char *payload; //mutable part of the object
};


void *allocate(size_t size, MContext *mc);
void deallocate(void *ptr, MContext *mc);


#endif
