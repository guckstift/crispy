#include <stdlib.h>
#include <string.h>
#include "array.h"

void array_free(void *array)
{
	int64_t *base = array;
	
	if(array) {
		free(base - 1);
	}
}

int64_t array_length(void *array)
{
	int64_t *base = array;
	return array ? base[-1] : 0;
}

void *_array_resize(void *array, int64_t itemsize, int64_t length)
{
	int64_t *base = array;
	base = realloc(array ? base - 1 : 0, sizeof(int64_t) + itemsize * length);
	*base = length;
	return base + 1;
}

void *_array_push(void *array, int64_t itemsize, void *item)
{
	int64_t length = array_length(array);
	char *base = _array_resize(array, itemsize, length + 1);
	memcpy(base + length * itemsize, item, itemsize);
	return base;
}
