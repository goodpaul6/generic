#ifndef HASHMAP_H
#define HASHMAP_H

#ifdef __cplusplus
extern "C" {
#endif

#define HASHMAP_BUCKET_AMT 1024

#include <stddef.h>

#include "vector.h"

typedef struct hashmap_node
{
	struct hashmap_node* next;
	char* key;
	void* value;
} hashmap_node_t;

typedef void (*hashmap_traverse_t)(const char* key, void* value, void* data);

typedef struct 
{
	hashmap_node_t* buckets[HASHMAP_BUCKET_AMT];
	vector_t active_buckets;
	
} hashmap_t;

void map_init(hashmap_t* map);

void* map_get(hashmap_t* map, const char* key);
void map_set(hashmap_t* map, const char* key, void* value);
void* map_remove(hashmap_t* map, const char* key);

void map_traverse(hashmap_t* map, hashmap_traverse_t traverse, void* data);

void map_clear(hashmap_t* map);
void map_destroy(hashmap_t* map);

#ifdef __cplusplus
}
#endif

#endif
