#ifndef VECTOR_H
#define VECTOR_H

#include <stddef.h>

typedef struct 
{
	unsigned char* data;
	size_t datum_size;
	
	size_t length;
	size_t capacity;
} vector_t;

typedef void (*vector_traverse_t)(void* value);
typedef int (*vector_compare_t)(const void* data, const void* a, const void* b);

void vec_init(vector_t* vector, size_t datum_size);

void vec_copy(vector_t* me, vector_t* other);
void vec_copy_region(vector_t* me, vector_t* other, size_t index, size_t start, size_t num);

void vec_reserve(vector_t* vector, size_t amount);
void vec_resize(vector_t* vector, size_t amount, void* p_initial_value);

void* vec_get(vector_t* vector, size_t index);
#define vec_get_value(vector, index, type) (*(type*)vec_get((vector), (index)))
void vec_set(vector_t* vector, size_t index, void* pvalue);
void vec_push_back(vector_t* vector, void* pvalue);
void vec_pop_back(vector_t* vector, void* into);
void vec_insert(vector_t* vector, void* pvalue, size_t index);
void vec_remove(vector_t* vector, size_t index);
void* vec_back(vector_t* vector);

void vec_flip(vector_t* vector);
void vec_sort(vector_t* vector, vector_compare_t compare);
void vec_traverse(vector_t* vector, vector_traverse_t traverse);

void vec_clear(vector_t* vector);
void vec_destroy(vector_t* vector);

#endif
