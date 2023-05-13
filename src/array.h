#ifndef ARRAY_H
#define ARRAY_H

#include <stdint.h>

#define array_resize(a, l)      (a) = _array_resize((a), sizeof*(a), (l))
#define array_push(a, v)        (a) = _array_push((a), sizeof(v), &(v))
#define array_for(a, i)         for(int64_t i = 0; i < array_length(a); i++)
#define array_for_ptr(a, T, p)  for(T *p = (a); p - (a) < array_length(a); p++)

void array_free(void *array);
int64_t array_length(void *array);
void *_array_resize(void *array, int64_t itemsize, int64_t length);
void *_array_push(void *array, int64_t itemsize, void *item);

#endif
