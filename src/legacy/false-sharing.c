#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "caches.h"

#ifdef OPTIMAL_MODE
#include "custom-alloc.h"
#endif

#define PL_SZ 48

struct shared_node {
  char payload[PL_SZ];
};

struct shared_node *nodes[7];

pthread_barrier_t barrier1;
pthread_barrier_t barrier2;

int number_of_runs = 10;


//allocate objects for the other threads
void *producer(void *args) {

  int i, j, r;
  int start = 1;

#ifdef OPTIMAL_MODE 
  init_free_list_alloc(NULL, L1_SZ);
#endif

  for (i = 0; i < number_of_runs; ++i) {
    
    //allocate objects for the other threads

    if (start == 0) {
      //starting with the 2nd round, wait for the
      //consumers to finish
      r = pthread_barrier_wait(&barrier1);
      if (!(r == 0 || r == PTHREAD_BARRIER_SERIAL_THREAD)) {
        printf("unable to wait at barrier1\n");
        exit(1);
      }
    } else {
      //in the first round, don't wait for consumers
      start = 0;
    }

    //printf("producing\n");


    for(j = 0; j < 7; ++j) {
#ifdef OPTIMAL_MODE 
      nodes[j] = (struct shared_node*)free_list_malloc(sizeof(struct shared_node));
#else
      nodes[j] = malloc(sizeof(struct shared_node));
#endif
      //printf("%p\n", nodes[j]);
    }


    //producer is ready
    r = pthread_barrier_wait(&barrier2);
    if (!(r == 0 || r == PTHREAD_BARRIER_SERIAL_THREAD)) {
      printf("unable to wait at barrier2\n");
      exit(1);
    }

    
  }
  //finally, tell the consumers they can terminate
  r = pthread_barrier_wait(&barrier1);
  if (!(r == 0 || r == PTHREAD_BARRIER_SERIAL_THREAD)) {
    printf("unable to wait at barrier1\n");
    exit(1);
  }

}

//consume data and free object
void *consumer(void *args) {
  //consume the unshared private data
  long id = (long)args;
  int i, j, k, r;
  int start = 1;

  for (i = 0; i < number_of_runs; ++i) {

    //wait for producer to be finished
    r = pthread_barrier_wait(&barrier2);
    if (!(r == 0 || r == PTHREAD_BARRIER_SERIAL_THREAD)) {
      printf("unable to wait at barrier2\n");
      exit(1);
    }

    //printf("%ld: consuming\n", id);
    for (k = 0; k < 10000000; ++k) {
    	for (j = 1; j < PL_SZ; ++j) {
      	nodes[id]->payload[j-1] = nodes[id]->payload[j] + 1;
    	}
    }
#ifdef OPTIMAL_MODE
    //free_list_free((void*)nodes[id]);
#else
    //free(nodes[id]);
#endif

    //consumer is ready
    r = pthread_barrier_wait(&barrier1);
    if (!(r == 0 || r == PTHREAD_BARRIER_SERIAL_THREAD)) {
      printf("unable to wait at barrier1\n");
      exit(1);
    }
  
  }
}



int main(int argc, char **argv) {
  int i, r;
  int num_threads = 8;

  if (pthread_barrier_init(&barrier1, NULL, num_threads)) {
    printf("unable to allocate barrier\n");
    exit(1);
  }
  if (pthread_barrier_init(&barrier2, NULL, num_threads)) {
    printf("unable to allocate barrier\n");
    exit(1);
  }

  pthread_t prod;
  pthread_t cons[7];

  for (i = 0; i < 7; ++i) {
    if (pthread_create(&cons[i], NULL, consumer, (void*)(long)i)) {
      printf("unable to create consumer\n");
      exit(1);    
    }
  }

  if (pthread_create(&prod, NULL, producer, NULL)) {
    printf("unable to create producer\n");
    exit(1);
  }

  if (pthread_join(prod, NULL)) {
    printf("unable to join producer\n");
    exit(1);
  }


  for (i = 0; i < 7; ++i) {
    if (pthread_join(cons[i], NULL)) {
      printf("unable to join consumer\n");
      exit(1);    
    }
  }

  return EXIT_SUCCESS;
}


