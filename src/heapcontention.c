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

int number_of_runs = 10;
int num_producers = 4;
int num_consumers = 4;

struct list *consumer_lists;

static int get_empty_consumer() {
	int i;
	while (1) {
		for (i = 0; i < num_consumers; ++i) {
			if (consumer_lists[i].start == NULL) {
				if (__sync_bool_compare_and_swap(
							&consumer_lists[i].start,
							NULL,
							1)) {
					//found an empty consumer
					printf("empty consumer: %d\n", i);
					return i;
				}
			}			
		}
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
	struct lnode *start; 
	int i;
	for (i = 0; i < elements ; ++i) {
		//sz goes from 1<<4 to 1<<20 bytes
		size_t sz = 1 << ((i%16)+4);
		l = create_lnode(sz);
		if (i == 0) start = l; //remember the start of the list
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

	while (1) {
		int c = get_empty_consumer();
		//assert consumer_lists[c].start == 1
		consumer_lists[c].start = create_list(1000);
	}
}

void *consumer(void *args) {
	long id = (long)args;

	while (1) {
		struct lnode *start = consumer_lists[id].start;
		if (start == NULL || start == (void*)1) continue; //spin on empty list

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

  return EXIT_SUCCESS;
}


