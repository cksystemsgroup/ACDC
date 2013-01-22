#include <sys/types.h>

struct spin_barrier {
  u_int64_t count __attribute__ ((aligned(64)));
  unsigned int max __attribute__ ((aligned(64)));
};
typedef struct spin_barrier spin_barrier_t;

int spin_barrier_init(volatile spin_barrier_t *barrier, unsigned int num_threads);
int spin_barrier_wait(volatile spin_barrier_t *barrier);

