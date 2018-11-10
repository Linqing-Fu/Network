#include "./include/2bit_trie.h"

// void lookup(struct t trie, u32 key){
// 	u64 index = 0;
// 	u64 vector = trie.N[index].vector;
// 	u32 offset = 0;
// 	int value = extract(key, offset, 6);
// 	while(vector & (1ULL << value)){
// 		u32 base = trie.N[index].base1;
// 		u32 bc = __builtin_popcount(vector & ((2ULL << value) - 1));
// 		index = base + bc - 1;
// 		vector = trie.N[index].vector;
// 		offset += 6;
// 		value = extract(key, offset, 6);  
// 	}

// 	base = trie.N[index].base0;
// 	bc = __builtin_popcount((~t.N[index].vector) & ((2ULL << value) - 1));
// 	return trie.L[base + bc - 1];
// }


Node* CreateTrie(){
	Node *node = (Node *)malloc(sizeof(Node));
	// memset(node, 0, sizeof(Node));
	node->match = 0;
	node->ip = 0;
	node->mask = 0;
	node->inode = -1;
	for(int i = 0; i < MAX_CHILD; i++){
		node->child[i] = NULL;
	}
	// node->child[0] = NULL;
	// node->child[1] = NULL;
	// node->child[2] = NULL;
	// node->child[3] = NULL;
	return node;
}

int extract(u32 ip, int offset){
	// printf("calculate value\n");
	unsigned int temp = 0xFFFFFFFF;
	int value;
	if((32 - offset - KEY) >= 0){
		value = (ip >> (32 - (offset + KEY))) & (temp >> (32 - KEY));
	} else {
		value = ip & (temp >> (32 - KEY));
	}
	return value;
}

void insert_node(Trie_node root, u32 ip, int mask, int inode){
	if(root == NULL)
		return;
	Node *t = root;
	int offset = 0;
	int index = extract(ip, offset);
	int num = (mask % KEY == 0)?(mask/KEY):(mask/KEY + 1);
	for(int i = 0; i < num; i++){
		if(t->child[index] == NULL){
			Node *tmp = CreateTrie();
			t->child[index] = tmp;
		}
		t = t->child[index];
		offset += KEY;
		index = extract(ip, offset);
	}
	if(((t->mask < mask) && t->match) || (t->match == 0)){
		t->match = 1;
		t->ip = ip;
		t->mask = mask;
		t->inode = inode;
	}
}

void Leaf_Push(Trie_node *root, Node *former_match){
	#if KEY == 2
		if(((*root)->child[0] == NULL) && ((*root)->child[1] == NULL) && ((*root)->child[2] == NULL) && ((*root)->child[3] == NULL)){
			return;
		}
	#elif KEY == 3
		if(((*root)->child[0] == NULL) && ((*root)->child[1] == NULL) && ((*root)->child[2] == NULL) && ((*root)->child[3] == NULL) &&
		   ((*root)->child[4] == NULL) && ((*root)->child[5] == NULL) && ((*root)->child[6] == NULL) && ((*root)->child[7] == NULL)){
			return;
		}
	#elif KEY == 4
		if(((*root)->child[0] == NULL) && ((*root)->child[1] == NULL) && ((*root)->child[2] == NULL) && ((*root)->child[3] == NULL) &&
		   ((*root)->child[4] == NULL) && ((*root)->child[5] == NULL) && ((*root)->child[6] == NULL) && ((*root)->child[7] == NULL) &&
		   ((*root)->child[8] == NULL) && ((*root)->child[9] == NULL) && ((*root)->child[10] == NULL) && ((*root)->child[11] == NULL) &&
		   ((*root)->child[12] == NULL) && ((*root)->child[13] == NULL) && ((*root)->child[14] == NULL) && ((*root)->child[15] == NULL)){			
			return;
		}
	#endif

	Node *push_node = NULL;
	push_node = ((*root)->match)?(*root):former_match;
	for(int i = 0; i < MAX_CHILD; i++){
		if((*root)->child[i] == NULL){
			(*root)->child[i] = push_node;
			if(push_node != NULL){
				// printf("push_node ip:%X\n", push_node->ip);
			}
		} else {
			if((*root)->child[i]->match){
				// printf("match ip:%X\n", (*root)->child[i]->ip);
				Leaf_Push(&((*root)->child[i]), NULL);
			} else {
				Leaf_Push(&((*root)->child[i]), push_node);
			}
		}
	}
	if((*root)->match){
		Node *new_node = NULL;
		new_node = CreateTrie();
		for(int i = 0; i < MAX_CHILD; i++){
			new_node->child[i] = (*root)->child[i];
			(*root)->child[i] = NULL;
		}
		(*root) = new_node;
		// MBIT_Print_Tree((*root));
	}
}

int search_node(Trie_node root, u32 ip){
	// printf("begin search\n");
	if(root == NULL){
		printf("tree is empty\n");
		return -1;
	}
	int longest_match_inode = -1;
	u32 print_ip = 0;
	Node *t = root;
	int offset = 0;
	int index = extract(ip, offset);
	int num = (32 % KEY == 0)?(32/KEY):(32/KEY + 1);
	for(int i = 0; i < num; i++){
		// if(t->match == 1){
		// 	printf("find a match\n");
		// 	longest_match_inode = t->inode;
		// }
		t = t->child[index];
		if(t == NULL){
			break;
		}
		if(t->match == 1){
			longest_match_inode = t->inode; 
			print_ip = t->ip;
		}
		offset += KEY;
		index = extract(ip, offset);
		
	}
	// printf("ip:%X\n", print_ip);
	return longest_match_inode;

}

int search_node_lp(Trie_node root, u32 ip){
	// printf("begin search\n");
	if(root == NULL){
		printf("tree is empty\n");
		return -1;
	}
	int longest_match_inode = -1;
	// int largest_mask = -1;
	Node *t = root;
	int offset = 0;
	int index = extract(ip, offset);
	int num = (32 % KEY == 0)?(32/KEY):(32/KEY + 1);
	for(int i = 0; i < num; i++){
		t = t->child[index];
		if(t == NULL){
			break;
		} 
		if(t->match == 1){
			longest_match_inode = t->inode;
		}
		offset += KEY;
		index = extract(ip, offset);
	}
	return longest_match_inode;
}



Cp_node* CreateCpTrie(){
	Cp_node *node = (Cp_node *)malloc(sizeof(Cp_node));
	node->ip = 0;
	// node->mask = 0;
	node->inode = -1;
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

int popcount(char *vector, int offset){
	int number = 0;
	for(int i = 0; i < offset; i++){
		if(vector[i] == 1){
			number++;
		}
	}
	return number;

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
	if(compress_tree->leaf_base == NULL){
		printf("cannot malloc leaf base\n");
	}
	compress_tree->internal_base = (Cp_node *)malloc(MAX_CHILD * sizeof(Cp_node));
	if(compress_tree->internal_base == NULL){
		printf("cannot malloc internal_base\n");
	}
	//inital
	memset(compress_tree->leaf_base, 0, MAX_CHILD * sizeof(Cp_node));
	memset(compress_tree->internal_base, 0, MAX_CHILD * sizeof(Cp_node));

	for(int i = 0; i < MAX_CHILD; i++){
		//set every inode = -1
		(compress_tree->leaf_base)[i].inode = -1;
		(compress_tree->internal_base)[i].inode = -1;
	}

	for(int i = 0; i < MAX_CHILD; i++){
		if(tree->child[i] != NULL){
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

	free(tree);

	//compress vector
	//count how many 1(internal node) in total
	int count = popcount(compress_tree->vector, MAX_CHILD);
	Cp_node *inter = (Cp_node *)malloc(count * sizeof(Cp_node));
	Cp_node *leaf = (Cp_node *)malloc((MAX_CHILD - count) * sizeof(Cp_node));
	
	int index_inter = 0;
	int index_leaf = 0;
	for(int i = 0; i < MAX_CHILD; i++){
		if(compress_tree->vector[i] == 1){
			inter[index_inter].ip = (compress_tree->internal_base)[i].ip;
			inter[index_inter].inode = (compress_tree->internal_base)[i].inode;
			memcpy(inter[index_inter].vector, (compress_tree->internal_base)[i].vector, MAX_CHILD);
			inter[index_inter].leaf_base = (compress_tree->internal_base)[i].leaf_base;
			inter[index_inter].internal_base = (compress_tree->internal_base)[i].internal_base;
			index_inter++;
		} else {
			leaf[index_leaf].ip = (compress_tree->leaf_base)[i].ip;
			leaf[index_leaf].inode = (compress_tree->leaf_base)[i].inode;
			memcpy(leaf[index_leaf].vector, (compress_tree->leaf_base)[i].vector, MAX_CHILD);			
			leaf[index_leaf].leaf_base = (compress_tree->leaf_base)[i].leaf_base;
			leaf[index_leaf].internal_base = (compress_tree->leaf_base)[i].internal_base;
			index_leaf++;
		}
	}
	free(compress_tree->leaf_base);
	free(compress_tree->internal_base);
	compress_tree->leaf_base = leaf;
	compress_tree->internal_base = inter;


}

int lookup_cp_tree(Cp_Trie_node compress_tree, u32 ip){
	if((compress_tree->leaf_base == NULL) && (compress_tree->internal_base == NULL)){
		printf("tree is empty\n");
		return -1;
	}
	int longest_match_inode = -1;
	Cp_node *t = compress_tree;
	int offset = 0;
	int index = extract(ip, offset);
	int num = (32 % KEY == 0)?(32/KEY):(32/KEY + 1);
	for(int i = 0; i < num; i++){
		if(t->vector[index] == 1){
			//child is internal node
			int index_in_internal = popcount(t->vector, index + 1);
			t = &((t->internal_base)[index_in_internal - 1]);
			// t = &((t->internal_base)[index]);
		} else {
			//child is leaf or this child doesn't exist
			int index_in_leaf = index - popcount(t->vector, index);
			t = &((t->leaf_base)[index_in_leaf]);
			// t = &((t->leaf_base)[index]);
			if(t->inode != -1){
				//is a leaf
				longest_match_inode = t->inode;
				break;
			} else {
				//doesn't exist
				break;
			}
		}
		offset += KEY;
		index = extract(ip, offset);
	}
	return longest_match_inode;
}

void MBIT_Print_Tree(Trie_node tree){
	for(int i = 0; i < MAX_CHILD; i++){
		if(tree->child[i]){
			printf("%d son\n", i);
			MBIT_Print_Tree(tree->child[i]);
			printf("%d son over\n", i);
		
		}
	}
	if(tree->match){
		printf("%x\n",tree->ip);
	}
}

int total_size(Cp_Trie_node compress_tree){
	long long unsigned int size = 0;
	if((compress_tree->leaf_base == NULL) && (compress_tree->internal_base == NULL)){
		return 0;
	} else {
		size += sizeof(compress_tree);
	}
	for(int i = 0; i < MAX_CHILD; i++){
		if(compress_tree->vector[i] == 1){
			int index_in_internal = popcount(compress_tree->vector, i + 1);
			size += total_size(&((compress_tree->internal_base)[index_in_internal - 1]));
		} else {
			int index_in_leaf = i - popcount(compress_tree->vector, i);
			size += total_size(&((compress_tree->leaf_base)[index_in_leaf]));
		}
	}
	return size;
}

int main(){
	// printf("start\n");
	
	printf("Open file...\n");	
	FILE *fp = fopen("forwarding-table.txt", "rb");
	if(!fp){
		printf("cannot open file : forwarding-table.txt\n");
	}
	Trie_node tree = (Trie_node)CreateTrie();
	// Trie_node tree_2bit = (Trie_node)CreateTrie();

	printf("Create tree.....\n");
	//create tree
	for(int i = 0; i < 697882; i++){
		unsigned int ip_0;
		unsigned int ip_1;
		unsigned int ip_2;
		unsigned int ip_3;
		int mask;
		int inode;
		fscanf(fp, "%u.%u.%u.%u %d %d", &ip_0, &ip_1, &ip_2, &ip_3, &mask, &inode);
		u32 ip = (((ip_0 & 0xFF)<<24) | (ip_1 & 0xFF)<<16 | (ip_2 & 0xFF)<<8 | (ip_3 & 0xFF) );
		// printf("%X\n", ip);
		insert_node(tree, ip, mask, inode);
		// insert_node(tree_2bit, ip, mask, inode);		
	}
	// MBIT_Print_Tree(tree);
	// #if KEY != 1
		// Leaf_Push(&tree, NULL);
	// #endif
	// MBIT_Print_Tree(tree);

	#ifdef CPRESS
	Cp_Trie_node compress_tree = CreateCpTrie();

	// printf("Compressing tree...\n");
	Compress(tree, compress_tree);
	printf("single node size:%lu\n", sizeof(compress_tree));
	long long unsigned int total = total_size(compress_tree);
	printf("total size is %llu\n", total);

	#endif
	// printf("Compress finished\n");

	// printf("Input an ip search in tree:\n");
	

	fseek(fp, 0, SEEK_SET);
	float total_time = 0;
	for(int i = 0; i < 697882; i++){
		unsigned int ip_0 ;//= 223;
		unsigned int ip_1 ;//= 255;
		unsigned int ip_2 ;//= 254;
		unsigned int ip_3 ;//= 0;
		int mask;// = 24;
		int inode;// = 7;
		fscanf(fp, "%u.%u.%u.%u %d %d", &ip_0, &ip_1, &ip_2, &ip_3, &mask, &inode);
		// scanf("%u.%u.%u.%u", &ip_0, &ip_1, &ip_2, &ip_3);

		u32 search_ip = (((ip_0 & 0xFF)<<24) | (ip_1 & 0xFF)<<16 | (ip_2 & 0xFF)<<8 | (ip_3 & 0xFF) );
		
		struct timeval start, end;
		gettimeofday(&start, NULL);
		#if KEY == 1
			int longest_match_inode = search_node(tree, search_ip);
		#else
			#ifdef CPRESS
				int longest_match_inode = lookup_cp_tree(compress_tree, search_ip);
			#else
				int longest_match_inode = search_node_lp(tree, search_ip);
			#endif
		#endif
		gettimeofday(&end, NULL);
		total_time += (end.tv_usec - start.tv_usec) + (end.tv_sec - start.tv_sec)*1000000;
		// if(longest_match_inode != inode){
			// printf("ip:%u.%u.%u.%u inode:%d & %d\n", ip_0, ip_1, ip_2, ip_3, longest_match_inode, inode);
		// } else {
		// printf("%d\n", longest_match_inode);
		// }
	}

	printf("Average time per lookup: %.10f\n", total_time / 697882);


	
	fclose(fp);


}