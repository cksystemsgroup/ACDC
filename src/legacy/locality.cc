#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <pthread.h>

#include "arch.h"
#include "caches.h"
#include "custom-alloc.h"

#define MAX_THREADS 128

//hoard declarations
extern "C" {
extern void *xxmalloc(size_t size);
extern void xxfree(void *ptr);
}


struct mutator_args {
  int mode;
};


static void *(*allocate)(size_t);
static void (*deallocate)(void*);

void *mutator(void *ptr) {

#ifdef OPTIMAL_MODE 
  init_free_list_alloc(NULL, L1_SZ);
#endif

  //the pointers array also needs to fit in the cache.
  //therefore we cannot store L1_LINES pointers
  //but (L1_SZ - (L1_LINES * 8) / L1_LINES
  void *pointers[L1_LINES-128] __attribute__((aligned(L1_LINE_SZ)));

  int i, j, k, reps;
  struct mutator_args *args = (struct mutator_args*)ptr;

  for (reps = 0; reps < 3000; ++reps) {
    //start with every 100th object and decrease distance
    for (k = 100; k > 0; --k) {
      
      //allocate everything
      for (i = 0; i < L1_LINES-128; i+=k) {
        pointers[i] = allocate(L1_LINE_SZ);
      }

      //touch and deallocate
      for (i = 0; i < L1_LINES-128; i+=k) {
        //printf("object %d\n", i);
        char *p = (char*)pointers[i];
        //touch every byte
        for (j = 1; j < L1_LINE_SZ; ++j) {
          char *a = p+(j-1);
          char *b = p+j;
          //printf("writing %p tp %p\n", b, a);
          *b = *a;
        }
        //return object
        deallocate(pointers[i]);
      }

    } //endfor k
  } //endfor reps
}

static void setup_function_pointers(void) {
#ifdef OPTIMAL_MODE 
  printf("\tOPTIMAL_MODE: 1\n");
  allocate = free_list_malloc;
  deallocate = free_list_free;
#endif

#ifdef GLIBC_MODE
  printf("\tGLIBC_MODE: 2\n");
  allocate = malloc;
  deallocate = free;
#endif

#ifdef HOARD_MODE
  printf("\tHOARD_MODE: 3\n");

  //TODO: use dlopen to get the function pointer to hoards malloc
  //TODO: check if we can get a static interface to hoard
  allocate = malloc;
  deallocate = free;
#endif
}

int main(int argc, char **argv) {

  //test_conversions();  

  int rc, i;
  struct mutator_args args;

  if (argc < 2) {
    printf("usage: locality num_threads\n");
    return EXIT_FAILURE;
  }


  int num_threads = atoi(argv[1]);

  if (num_threads > MAX_THREADS) {
    printf("num_threads > MAX_THREADS\n");
    exit(1);
  }

  setup_function_pointers();

  //statically allocate threads
  pthread_t workers[MAX_THREADS] __attribute__((aligned(L1_LINE_SZ)));

  for (i = 0; i < num_threads; ++i) {
    rc = pthread_create(&workers[i], NULL, mutator, (void*)&args);
    if (rc) {
      printf("Thread creation error %d\n", rc);
      exit(1);
    }
  }

  for (i = 0; i < num_threads; ++i) {
    rc = pthread_join(workers[i], NULL);
    if (rc) {
      printf("Thread join error %d\n", rc);
      exit(1);
    }
  }


  return EXIT_SUCCESS;
}

