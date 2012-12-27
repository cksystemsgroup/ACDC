#ifndef MEMORY_H
#define MEMORY_H

//object header for every allocated object
//the min. size for an object must be sizeof(Object)
typedef struct mem_object Object;
struct mem_object {
  size_t size; //the allocated size of the object
};


void *allocate(size_t size);
void deallocate(void *ptr);


#endif
