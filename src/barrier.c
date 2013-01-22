#include "barrier.h"



int spin_barrier_init(volatile spin_barrier_t *barrier, unsigned int num_threads) {
	barrier->count = 0;
	barrier->max = num_threads;
	return 0;
}
int spin_barrier_wait(volatile spin_barrier_t *barrier) {

	u_int64_t old_count = __sync_fetch_and_add(&barrier->count, 1);
	u_int64_t next = (old_count / barrier->max) * barrier->max + barrier->max;

	while (barrier->count < next) {} // spin

	return 0;
}
