#ifdef __cplusplus
extern "C" {
#endif

#include "vector.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

static void error_exit(const char* fmt, ...)
{
	fprintf(stderr, "\nError:\n");
	
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	
	exit(1);
}

static void vec_realloc(vector_t* vec)
{
	void* new_data = realloc(vec->data, vec->capacity * vec->datum_size);
	if(!new_data) error_exit("Failed to reallocate vector; Out of memory!\n");
	vec->data = new_data;
}

void vec_init(vector_t* vector, size_t datum_size)
{
	vector->data = NULL;
	vector->datum_size = datum_size;
	vector->length = 0;
	vector->capacity = 0;
}

void vec_copy(vector_t* me, vector_t* other)
{
	if(me->datum_size != other->datum_size) error_exit("Attempted to copy vectors with different data sizes\n");
	if(me->capacity < other->capacity)
	{
		me->capacity = other->capacity;
		vec_realloc(me);
	}
	
	memcpy(me->data, other->data, me->datum_size * other->capacity);
}

void vec_copy_region(vector_t* me, vector_t* other, size_t index, size_t start, size_t amount)
{
	if(me->datum_size != other->datum_size) error_exit("Attempted to copy vectors with different data sizes\n");
	
	if(start < 0 || start > other->length ||
		start + amount > other->length) error_exit("Attempted to copy from region outside of bounds of vector\n");
	
	vec_reserve(me, index + amount);

	if(index + amount > me->length) me->length = index + amount;
	memcpy(&me->data[index * me->datum_size], &other->data[start * other->datum_size], amount * other->datum_size);
}

void vec_reserve(vector_t* vector, size_t amount)
{
	if(vector->capacity < amount)
	{
		vector->capacity = amount;
		vec_realloc(vector);
	}
}

void vec_resize(vector_t* vector, size_t amount, void* p_initial_value)
{
	vec_reserve(vector, amount);
	vector->length = amount;
	
	if(p_initial_value)
	{
		for(int i = 0; i < vector->length; ++i)
			memcpy(&vector->data[i * vector->datum_size], p_initial_value, vector->datum_size);
	}
}

void* vec_get(vector_t* vector, size_t index)
{
	if(index >= vector->length) 
		error_exit("Provided out of bounds index (%i) to vec_get (vector length: %i)\n", (int)index, (int)vector->length);
	return &vector->data[index * vector->datum_size];
}

void vec_set(vector_t* vector, size_t index, void* object)
{
	if(index >= vector->length) 
		error_exit("Provided out of bounds index (%i) to vec_set (vector length: %i)\n", (int)index, (int)vector->length);
	memcpy(&vector->data[index * vector->datum_size], object, vector->datum_size);
}

void vec_push_back(vector_t* vector, void* object)
{
	while(vector->length + 1 >= vector->capacity)
	{
		if(vector->capacity == 0)
			vector->capacity = 8;
		vector->capacity *= 2;
		vec_realloc(vector);
	}
	
	memcpy(&vector->data[vector->length * vector->datum_size], object, vector->datum_size);
	vector->length++;
}

void vec_pop_back(vector_t* vector, void* into)
{
	if(vector->length == 0) error_exit("Attempted to perform vec_pop_back on empty vector");
	memcpy(into, &vector->data[(--vector->length) * vector->datum_size], vector->datum_size);
}

void vec_insert(vector_t* vector, void* object, size_t index)
{
	if(index >= vector->length) error_exit("Provided out of bounds index (%i) to vec_insert (vector length: %i)", (int)index, (int)vector->length);
	
	while(vector->length + 1 >= vector->capacity)
	{
		vector->capacity *= 2;
		vec_realloc(vector);
	}
	
	memmove(&vector->data[(index + 1) * vector->datum_size], &vector->data[index * vector->datum_size], (vector->length - index) * vector->datum_size);
	memcpy(&vector->data[index * vector->datum_size], object, vector->datum_size);
	vector->length += 1;
}

void vec_remove(vector_t* vector, size_t index)
{
	if(index >= vector->length) error_exit("provided out of bounds index (%i) to vec_remove (vector length: %i)", (int)index, (int)vector->length);
	
	memmove(&vector->data[index * vector->datum_size], &vector->data[(index + 1) * vector->datum_size], (vector->length - index - 1) * vector->datum_size);
	vector->length -= 1;
}

void* vec_back(vector_t* vector)
{
	if(vector->length == 0) return NULL;
	return &vector->data[(vector->length - 1) * vector->datum_size];
}

void vec_flip(vector_t* vector)
{
	if(vector->length == 0) return;
	
	unsigned char* lo = vector->data;
	unsigned char* hi = &vector->data[vector->length - 1];
	unsigned char swap;
	
	while(lo < hi)
	{
		swap = *lo;
		*lo++ = *hi;
		*hi-- = swap;
	}
}

void vec_sort(vector_t* vector, vector_compare_t compare)
{
	qsort(vector->data, vector->length, vector->datum_size, (int(*)(const void*, const void*))compare);
}

void vec_traverse(vector_t* vector, vector_traverse_t traverse)
{
	for(int i = 0; i < vector->length; ++i)
		traverse(&vector->data[i * vector->datum_size]);
}

void vec_clear(vector_t* vector)
{
	vector->length = 0;
}

void vec_destroy(vector_t* vector)
{
	if(vector->data)
		free(vector->data);
	vector->data = NULL;
	vector->length = 0;
	vector->capacity = 0;
}

#ifdef __cplusplus
}
#endif
