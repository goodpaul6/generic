#include "hashmap.h"

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
	fprintf(stderr, "\n");
	exit(1);
}

static void* emalloc(size_t size)
{
	void* mem = malloc(size);
	if(!mem) error_exit("Out of memory!\n");
	return mem;
}

static char* my_strdup(const char* str)
{
	char* dup = emalloc(strlen(str) + 1);
	strcpy(dup, str);
	return dup;
}

static unsigned long map_hash_function(const char* str)
{
	unsigned long hash = 5381;
	int c;

	while ((c = *str++))
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash;
}

static hashmap_node_t* create_node(const char* key, void* value)
{
	hashmap_node_t* node = malloc(sizeof(hashmap_node_t));
	if(!node) error_exit("failed to create hashmap node (key: '%s'); out of memory", key);
	
	node->key = my_strdup(key);
	node->value = value;
	node->next = NULL;
	
	return node;
}

void map_init(hashmap_t* map)
{	
	memset(map->buckets, 0, sizeof(map->buckets));
	vec_init(&map->active_buckets, sizeof(unsigned long));
}

void map_set(hashmap_t* map, const char* key, void* value)
{
	unsigned long pos = map_hash_function(key) % HASHMAP_BUCKET_AMT;
	
	hashmap_node_t* node = create_node(key, value);
	
	if(!map->buckets[pos])
		vec_push_back(&map->active_buckets, &pos);
	
	node->next = map->buckets[pos];
	map->buckets[pos] = node;
}

void* map_get(hashmap_t* map, const char* key)
{
	unsigned long pos = map_hash_function(key) % HASHMAP_BUCKET_AMT;
	
	if(!map->buckets[pos])
		return NULL;
	else 
	{
		hashmap_node_t* node = map->buckets[pos];
		
		while(node)
		{
			if(strcmp(node->key, key) == 0)
				return node->value;
			node = node->next;
		}
	}
	
	return NULL;
}

void* map_remove(hashmap_t* map, const char* key)
{
	unsigned long pos = map_hash_function(key) % HASHMAP_BUCKET_AMT;
	
	if(!map->buckets[pos])
		return NULL;
	else 
	{
		hashmap_node_t* node = map->buckets[pos];
		hashmap_node_t* prev = NULL;
		
		while(node)
		{
			if(strcmp(node->key, key) == 0)
			{
				if(prev)
					prev->next = node->next;
				else
				{
					map->buckets[pos] = NULL;
					int to_remove = -1;
					for(size_t i = 0; i < map->active_buckets.length; ++i)
					{
						if(*(unsigned long*)vec_get(&map->active_buckets, i) == pos)
						{
							to_remove = i;
							break;
						}
					}
					
					if(to_remove >= 0) vec_remove(&map->active_buckets, to_remove);
				}
				
				void* value = node->value;
				
				free(node->key);
				free(node);
				
				return value;
			}
			
			prev = node;
			node = node->next;
		}
	}
	
	return NULL;
}

static void free_bucket(hashmap_node_t* bucket)
{
	if(!bucket) return;
	
	hashmap_node_t* node = bucket;
	hashmap_node_t* next = NULL;
	
	while(node)
	{
		next = node->next;
		free(node->key);
		free(node);
		node = next;
	}
}

void map_traverse(hashmap_t* map, hashmap_traverse_t traverse, void* data)
{
	for(size_t i = 0; i < map->active_buckets.length; ++i)
	{
		unsigned long pos = *(unsigned long*)vec_get(&map->active_buckets, i);
		hashmap_node_t* node = map->buckets[pos];
		while(node)
		{
			traverse(node->key, node->value, data);
			node = node->next;
		}
	}
}

void map_clear(hashmap_t* map)
{
	for(size_t i = 0; i < map->active_buckets.length; ++i)
	{
		unsigned long pos = *(unsigned long*)vec_get(&map->active_buckets, i);
		hashmap_node_t* node = map->buckets[pos];
		free_bucket(node);
		map->buckets[pos] = NULL;
	}
	
	vec_clear(&map->active_buckets);
}

void map_destroy(hashmap_t* map)
{
	map_clear(map);
	vec_destroy(&map->active_buckets);
}
