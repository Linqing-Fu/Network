#ifndef __TRIE_H__
#define __TRIE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "types.h"

#if KEY == 1
	#define MAX_CHILD 2
#elif KEY == 2
	#define MAX_CHILD 4
#elif KEY == 3
	#define MAX_CHILD 8
#elif KEY == 4
 	#define	MAX_CHILD 16
#endif

typedef struct Tree{
	int match;
	u32 ip;
	int mask;
	int inode;
	struct Tree *child[MAX_CHILD];
}Node, *Trie_node;

typedef struct Compress_Tree{
	u32 ip; //debug
	// int mask;
	int inode;
	char vector[MAX_CHILD];
	struct Compress_Tree *leaf_base;
	struct Compress_Tree *internal_base;
}Cp_node, *Cp_Trie_node;


Node* CreateTrie();
int extract(u32 ip, int offset);
void insert_node(Trie_node root, u32 ip, int mask, int inode);
void Leaf_Push(Trie_node *root, Node *former_match);
int search_node(Trie_node root, u32 ip);
int search_node_lp(Trie_node root, u32 ip);
void MBIT_Print_Tree(Trie_node tree);
int total_size_cp_tree(Cp_Trie_node compress_tree);
int total_size_tree(Trie_node tree);
Cp_node* CreateCpTrie();
int popcount(char *vector, int offset);
void Compress(Trie_node tree, Cp_Trie_node compress_tree);
int lookup_cp_tree(Cp_Trie_node compress_tree, u32 ip);

#endif