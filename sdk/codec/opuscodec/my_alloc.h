#include <stddef.h>
#include <string.h>

void *malloc(size_t numbytes);
void *calloc(size_t nitems, size_t size);
void free(void *__restrict__ firstbyte);
void * realloc(void *firstbyte, size_t size);