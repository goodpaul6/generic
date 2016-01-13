#ifndef BSTREE_H
#define BSTREE_H

#include <stddef.h>

typedef struct bs_tree_node
{
	void* key;
	void* value;
	
	struct bs_tree_node* left;
	struct bs_tree_node* right;
	struct bs_tree_node* parent;
} bs_tree_node_t;

typedef int (*bs_tree_compare_t)(const void* a, const void* b);
typedef int (*bs_tree_traverse_t)(bs_tree_node_t* node);

typedef struct bs_tree
{
	size_t count;
	bs_tree_compare_t compare;
	bs_tree_node_t* root;
} bs_tree_t;

void bs_tree_init(bs_tree_t* tree, bs_tree_compare_t compare);

int bs_tree_set(bs_tree_t* tree, void* key, void* value);
void* bs_tree_get(bs_tree_t* tree, void* key);
void* bs_tree_remove(bs_tree_t* tree, void* key);

int bs_tree_traverse(bs_tree_t* tree, bs_tree_traverse_t traverse);

void bs_tree_destroy(bs_tree_t* tree);

#endif
