#ifndef _BINTREE_H_
#define _BINTREE_H_

#include <stddef.h>

typedef struct BintreeNode
{
	void *key;
	void *value;
	struct BintreeNode *left;
	struct BintreeNode *right;
} BintreeNode;

typedef struct
{
	BintreeNode *root;
	size_t key_size;
} Bintree;

typedef void (*BintreeFreeFunction)(void *value, void *arg);

Bintree bintree_create(size_t key_size);
BintreeNode **bintree_search(Bintree *tree, void *key);
void bintree_add_node(Bintree *tree, BintreeNode **nodeptr, void *key, void *value);
void bintree_clear(Bintree *tree, BintreeFreeFunction func, void *arg);

#endif
