#include <stdlib.h>
#include <stdio.h>
#include "custom-alloc.h"
#include "caches.h"

//every thread gets its own heap
static __thread void *free_list_heap = NULL;

struct free_list {
  struct free_list *next;
};


void init_free_list_alloc(void *start, size_t size) {

  //assert size is a multiple of L1_LINE_SZ
  if (size % L1_LINE_SZ) {
    printf("size not a multiple of L1_LINE_SZ\n");
    exit(1);
  }

  if (start == NULL) {
    //allocate heap and add an extra cache line for the free list
    int rc = posix_memalign(&start, L1_LINE_SZ, size + L1_LINE_SZ);
    if (rc) {
      printf("posix_memalign: %d\n", rc);
      exit(rc);
    }
  }

  //assert: sizeof(start) >= size + sizeof(struct free_list)
  //assert: start is L1_LINE_SZ aligned

  //the first words are reserved for the start of the free-list
  struct free_list *l = (struct free_list*)start;
  printf("free_list starts at %p\n", l);

  int i;
  for (i = L1_LINE_SZ; i <= size; i+=L1_LINE_SZ) {
    //next free line is the i+1st line
    //l->next = (struct free_list*)line_to_addr(i+1);
    l->next = (struct free_list*)((size_t)start + i);
    
    //printf("set next pointer to %p in %p\n", l->next, l);
    l = l->next;
  }
  //in the L1_LINES - 1st (the last line) terminate free-list
  //printf("terminate free-list in %p\n", &(l->next));
  l->next = NULL;


  //printf("free_list_heap starts at %p\n", free_list_heap);
  free_list_heap = start;
  //printf("free_list_heap starts at %p\n", free_list_heap);
}


void *free_list_malloc(size_t size) {

  //assert: size <= L1_LINE_SZ

  struct free_list *l = (struct free_list*)free_list_heap;
  struct free_list *free_element = l->next;

  if (free_element == NULL) {
    printf("out of memory\n");
    exit(1);
  }

  l->next = free_element->next;
  return (void*)free_element;
}

void free_list_free(void *ptr) {
  //assert: ptr is in heap and L1_LINE_SZ aligned
  struct free_list *l = (struct free_list*)free_list_heap;
  struct free_list *free_element = (struct free_list*)ptr;
  free_element->next = l->next;
  l->next = free_element;
}


























