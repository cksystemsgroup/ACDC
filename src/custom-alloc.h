



/* initialize free-list in a contiguous memory region
 * starting at start with size bytes. If start is NULL
 * then size bytes will be newly allocated
 */
#ifdef __cplusplus
extern "C" {
#endif
void init_free_list_alloc(void *start, size_t size);

void *free_list_malloc(size_t size);
void free_list_free(void *ptr);
#ifdef __cplusplus
}
#endif

