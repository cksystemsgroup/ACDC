#ifndef COLLECTION_H
#define COLLECTION_H

#include <glib.h>

//object pool where threads keep refs to the memory chunks
typedef struct object_collection OCollection;
struct object_collection {
  size_t object_size;
  //pointer to start of collection
  //iteration function
};

typedef struct collection_pool CollectionPool;
struct collection_pool {
  unsigned int remaining_lifetime;
  GHashTable *collections; //hash table with one collection per size
};

#endif
