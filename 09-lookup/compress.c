Cp_node* CreateCpTrie(){
	Cp_node *node = (Cp_node *)malloc(sizeof(Cp_node));
	node->ip = 0;
	// node->mask = 0;
	node->inode = 0;
	for(int i = 0; i < MAX_CHILD; i++){
		node->vector[i] = 0;
	}
	node->leaf_base = NULL;
	node->internal_base = NULL;
	// node->child[0] = NULL;
	// node->child[1] = NULL;
	// node->child[2] = NULL;
	// node->child[3] = NULL;
	return node;
}


void Compress(Trie_node tree, Cp_Trie_node compress_tree){
	//check if tree is a leaf
	if(tree->match == 1){
		compress_tree->ip = tree->ip;
		compress_tree->inode = tree->inode;
		return;
	}

	//malloc 2 * MAX_CHILD for children
	compress_tree->leaf_base = (Cp_node *)malloc(MAX_CHILD * sizeof(Cp_node));
	compress_tree->internal_base = (Cp_node *)malloc(MAX_CHILD * sizeof(Cp_node));

	//inital
	memset(compress_tree->leaf_base, 0, MAX_CHILD * sizeof(Cp_node));
	memset(compress_tree->internal_base, 0, MAX_CHILD * sizeof(Cp_node));

	for(int i = 0; i < MAX_CHILD; i++){
		if(tree->child[i]->match == 0){
			//child is a internal node
			//save in internal_base
			compress_tree->vector[i] = 1;

			Compress(tree->child[i], &((compress_tree->internal_base)[i]));

		} else {
			//child is a leaf
			Compress(tree->child[i], &((compress_tree->leaf_base)[i]));

		}
	}

}