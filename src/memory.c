#include "memory.h"

void *allocate(size_t size) {
	void *ptr;
	//TODO: log allocator activity
	
	if (size < sizeof(Object)) {
		printf("Error: min object size is %d. Requested: %d\n",
				sizeof(Object),
				size);
		exit(1);
	}

	ptr = malloc(size);
	
	Object *o = (Object*)ptr;
	o->size = size;

	return ptr;
}

void deallocate(void *ptr) {
	//TODO: log allocator activity
	free(ptr);
}

