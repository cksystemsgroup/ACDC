#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "caches.h"

struct lnode {
	struct lnode *next;
	void *object;
};
struct list {
	struct lnode *start;
	char padding[56];
};

int debug = 0;
int number_of_runs = 1000;
int list_sz = 1000;
int done = 0; //termination flag
int num_producers = 4;
int num_consumers = 4;
int finished_producers = 0;

struct list *consumer_lists;

static int get_empty_consumer(long unsigned int *waitcounter) {
	int i;
	while (!done) {
		for (i = 0; i < num_consumers; ++i) {
			if (consumer_lists[i].start == NULL) {
				if (__sync_bool_compare_and_swap(
							&consumer_lists[i].start,
							NULL,
							1)) {
					//found an empty consumer
					if (debug) printf("empty consumer: %d\n", i);
					return i;
				}
			}			
		}
		(*waitcounter)++;
	}
}

static struct lnode *create_lnode(size_t payloadsz) {
	struct lnode *n = malloc(sizeof(struct lnode));
	n->object = malloc(payloadsz);
	n->next = NULL;
	return n;
}

static struct lnode *create_list(unsigned int elements) {
	struct lnode *l;
	
	struct lnode *start = create_lnode(16);
	l = start;

	int i;
	for (i = 1; i < elements ; ++i) {
		//sz goes from 1<<4 to 1<<20 bytes
		size_t sz = 1 << ((i%16)+4);
		l->next = create_lnode(sz);
		l = l->next;
	}
	return start;
}

static void destroy_list(struct lnode *start) {
	struct lnode *tmp;
	while (start->next != NULL) {
		tmp = start;
		start = start->next;
		free(tmp->object);
		free(tmp);
	}
}

void *producer(void *args) {
	long id = (long)args;
	int number_of_productions = 0;
	long unsigned int waitcounter = 0;

	while (!done) {
		int c = get_empty_consumer(&waitcounter);
		if (debug) printf("producer,%lu,waited,%lu\n", id, waitcounter);
		waitcounter = 0;
		//if we selected an consumer after "done"
		//we do not add a new list
		if (done) {
			return;
		}
		//assert consumer_lists[c].start == 1
		consumer_lists[c].start = create_list(list_sz);

		//the first producer reaching the threshold
		//triggers benchmark termination
		number_of_productions++;
		if (number_of_productions > number_of_runs) {
			int finished_so_far = __sync_add_and_fetch(&finished_producers, 1);
			if (finished_so_far = num_producers) done = 1;
		}
	}
}

void *consumer(void *args) {
	long id = (long)args;
	long unsigned int waitcounter = 0;

	while (!done) {
		waitcounter++;
		struct lnode *start = consumer_lists[id].start;
		if (start == NULL || start == (void*)1) continue; //spin on empty list

		destroy_list(start);
		consumer_lists[id].start = NULL;
		if (debug) printf("consumer,%lu,waited,%lu\n", id, waitcounter);
		waitcounter = 0;
	}

	//after the producers have terminated, we free everything
	//that was produced after the consumer loop terminates
	
	struct lnode *start = consumer_lists[id].start;
	if (start != NULL && start != (void*)1) {
		destroy_list(start);
		consumer_lists[id].start = NULL;
	}
	
}



int main(int argc, char **argv) {
  int i, r;

  pthread_t *producers = calloc(num_producers, sizeof(pthread_t));
  pthread_t *consumers = calloc(num_consumers, sizeof(pthread_t));

  consumer_lists = calloc(num_consumers, sizeof(struct list));

  for (i = 0; i < num_producers; ++i) {
    if (pthread_create(&producers[i], NULL, producer, (void*)(long)i)) {
      printf("unable to create producer\n");
      exit(1);    
    }
  }
  for (i = 0; i < num_consumers; ++i) {
    if (pthread_create(&consumers[i], NULL, consumer, (void*)(long)i)) {
      printf("unable to create consumer\n");
      exit(1);    
    }
  }


  for (i = 0; i < num_consumers; ++i) {
    if (pthread_join(consumers[i], NULL)) {
      printf("unable to join consumer\n");
      exit(1);    
    }
  }
  for (i = 0; i < num_producers; ++i) {
    if (pthread_join(producers[i], NULL)) {
      printf("unable to join producer\n");
      exit(1);    
    }
  }

  free(producers);
  free(consumers);


  return EXIT_SUCCESS;
}


