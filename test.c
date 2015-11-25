#include "vector.h"
#include "bstree.h"
#include "hashmap.h"
#include "script.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

void write_key_value_pairs(char* key, void* value)
{
	printf("%s = %s\n", key, (char*)value);
}

int main(int argc, char* argv[])
{
	/* non-comprehensive bs_tree test:
	
	bs_tree_t tree;
	
	bs_tree_init(&tree, NULL);
	
	int v1 = 0;
	int v2 = 10;
	int v3 = 20;
	
	bs_tree_set(&tree, "first", &v1);
	bs_tree_set(&tree, "second", &v3);
	
	int v = *(int*)bs_tree_get(&tree, "second");
	
	printf("%d\n", v);
	
	bs_tree_destroy(&tree);*/
	
	/* non-comprehensive hashmap test:
	hashmap_t map;
	
	map_init(&map);
	map_set(&map, "hello", "world");
	map_set(&map, "test", "20");
	map_set(&map, "something", "another");
	
	printf("%s\n", map_get(&map, "hello"));
	
	map_traverse(&map, write_key_value_pairs);
	
	map_destroy(&map);*/
	
	script_t script;
	
	script_init(&script);
	
	script_load_run_file(&script, "test.txt");
	
	script_destroy(&script);

	return 0;
}
