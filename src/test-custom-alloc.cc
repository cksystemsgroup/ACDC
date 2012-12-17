#include <stdlib.h>
#include "custom-alloc.h"
#include "caches.h"

int main(int argc, char **argv) {


  void *mem;
  posix_memalign(&mem, L1_LINE_SZ, 65 * L1_LINE_SZ);

  init_free_list_alloc(mem, 64 * L1_LINE_SZ);


  int i, reps;
  void *ptr[64];
  
  for (reps = 1; reps <=64; ++reps) {
    for (i = 0; i < 64; ++i) {
      ptr[i] = free_list_malloc(reps);
    }

    for (i = 0; i < 64; ++i) {
      free_list_free(ptr[i]);
    }
  }


  return EXIT_SUCCESS;
}

