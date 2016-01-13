#include "bstree.h"

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

static void* emalloc(size_t size)
{
	void* mem = malloc(size);
	if(!mem) error_exit("Out of memory!\n");
	return mem;
}

static int default_compare(const void* a, const void* b)
{
	return strcmp(a, b);
}

void bs_tree_init(bs_tree_t* tree, bs_tree_compare_t compare)
{
	tree->count = 0;
	tree->compare = compare ? compare : default_compare;
	tree->root = NULL;
}

static int destroy_node(bs_tree_node_t* node)
{
	free(node);
	return 0;
}

void bs_tree_destroy(bs_tree_t* tree)
{
	bs_tree_traverse(tree, destroy_node);	
}

static bs_tree_node_t* create_node(bs_tree_node_t* parent, void* key, void* value)
{
	bs_tree_node_t* node = emalloc(sizeof(bs_tree_node_t));
	
	node->key = key;
	node->value = value;
	node->parent = parent;
	
	node->left = NULL;
	node->right = NULL;
	
	return node;
}

static void set_node(bs_tree_t* tree, bs_tree_node_t* node, void* key, void* value)
{
	int cmp = tree->compare(node->key, key);
	
	if(cmp <= 0)
	{
		if(node->left) set_node(tree, node->left, key, value);
		else node->left = create_node(node, key, value);
	}
	else
	{
		if(node->right) set_node(tree, node->right, key, value);
		else node->right = create_node(node, key, value);
	}
}

int bs_tree_set(bs_tree_t* tree, void* key, void* value)
{
	if(!tree->root)
		tree->root = create_node(NULL, key, value);
	else
		set_node(tree, tree->root, key, value);
}

static bs_tree_node_t* get_node(bs_tree_t* tree, bs_tree_node_t* node, void* key)
{
	int cmp = tree->compare(node->key, key);
	
	if(cmp == 0)
		return node;
	else if(cmp < 0)
		return node->left ? get_node(tree, node->left, key) : NULL;
	else
		return node->right ? get_node(tree, node->right, key) : NULL;
}

void* bs_tree_get(bs_tree_t* tree, void* key)
{
	if(!tree->root) return NULL;
	else
	{
		bs_tree_node_t* node = get_node(tree, tree->root, key);
		return node ? node->value : NULL;
	}
}

static int traverse_nodes(bs_tree_node_t* node, bs_tree_traverse_t traverse)
{
	int rc = 0;
	
	if(node->left)
	{
		rc = traverse_nodes(node->left, traverse);
		if(rc != 0) return rc;
	}
	
	if(node->right)
	{
		rc = traverse_nodes(node->right, traverse);
		if(rc != 0) return rc;
	}
	
	return traverse(node);
}

int bs_tree_traverse(bs_tree_t* tree, bs_tree_traverse_t traverse)
{
	if(tree->root) return traverse_nodes(tree->root, traverse);
	return 0;
}

static bs_tree_node_t* find_min(bs_tree_node_t* node)
{
	while(node->left) node = node->left;
	return node;
}

static void replace_node_in_parent(bs_tree_t* tree, bs_tree_node_t* node, bs_tree_node_t* new_node)
{
	if(node->parent)
	{
		if(node->parent->left == node) node->parent->left = new_node;
		else node->parent->right = new_node;
	}
	else
		tree->root = new_node;
		
	if(new_node)
		new_node->parent = node->parent;
}

static void swap_nodes(bs_tree_node_t* a, bs_tree_node_t* b)
{
	void* temp = NULL;
	temp = b->key; b->key = a->key; a->key = temp;
	temp = b->value; b->value = a->value; a->value = temp;
}

static bs_tree_node_t* delete_node(bs_tree_t* tree, bs_tree_node_t* node, void* key)
{
	int cmp = tree->compare(node->key, key);
	
	if(cmp < 0)
	{
		if(node->left) delete_node(tree, node->left, key);
		else return NULL;
	}
	else if(cmp > 0)
	{
		if(node->right) delete_node(tree, node->right, key);
		else return NULL;
	}
	else
	{
		if(node->left && node->right)
		{
			bs_tree_node_t* successor = find_min(node->right);
			swap_nodes(successor, node);
			
			replace_node_in_parent(tree, successor, successor->right);
			
			return successor;
		}
		else if(node->left) replace_node_in_parent(tree, node, node->left);
		else if(node->right) replace_node_in_parent(tree, node, node->right);
		else replace_node_in_parent(tree, node, NULL);
		
		return node;
	}
}

void* bs_tree_remove(bs_tree_t* tree, void* key)
{
	void* value = NULL;
	
	if(tree->root)
	{
		bs_tree_node_t* node = delete_node(tree, tree->root, key);
		
		if(node)
		{
			value = node->value;
			free(node);
		}
	}
	
	return value;
}
