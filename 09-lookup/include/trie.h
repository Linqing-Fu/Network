#ifndef __TRIE_H__
#define __TRIE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CHILD 2

typedef struct Tree{
	bool leaf;
	int inode;
	struct Tree *child[MAX_CHILD];
}Node, *Trie_node;




#endif