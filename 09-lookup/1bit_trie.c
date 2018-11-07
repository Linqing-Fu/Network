#include "./include/1bit_trie.h"

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
	memset(node, 0, sizeof(Node));
	
	
	return node;
}

int extract(u32 ip, int offset){
	// printf("calculate value\n");
	unsigned int temp = 0xFFFFFFFF;
	int value = (ip >> (32 - (offset + KEY))) & (temp >> (32 - KEY));
	return value;
}

void insert_node(Trie_node root, u32 ip, int mask, int inode){
	if(root == NULL)
		return;
	Node *t = root;
	int offset = 0;
	int index = extract(ip, offset);
	for(int i = 0; i < mask / KEY ; i++){
		if(t->child[index] == NULL){
			Node *tmp = CreateTrie();
			t->child[index] = tmp;
		}
		t = t->child[index];
		offset += KEY;
		index = extract(ip, offset);
	}
	t->match = 1;
	t->ip = ip;
	t->mask = mask;
	t->inode = inode;
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
	for(int i = 0; i < 32 / KEY; i++){
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


// void Print_Tree(Trie_node root){
// 	if(root->child[0]){
// 		Print_Tree(root->child[0]);
// 	}
// 	if(root->child[1]){
// 		Print_Tree(root->child[1]);
// 	}
// 	if(root->match == 1){
// 		printf("%X %d\n", root->ip, root->mask);
// 	}
// }



int main(){
	// printf("start\n");
	
	printf("Open file...\n");	
	FILE *fp = fopen("forwarding-table.txt", "rb");
	if(!fp){
		printf("cannot open file : forwarding-table.txt\n");
	}
	Trie_node tree = (Trie_node)CreateTrie();

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

	//////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////
	/* 1bit:  Test all data  */

	// printf("Input an ip search in tree:\n");
	
	// fseek(fp, 0, SEEK_SET);
	// for(int i = 0; i < 697882; i++){
	// 	unsigned int ip_0;// = 223;
	// 	unsigned int ip_1;// = 227;
	// 	unsigned int ip_2;// = 132;
	// 	unsigned int ip_3;// = 80;
	// 	int mask;
	// 	int inode;
	// 	fscanf(fp, "%u.%u.%u.%u %d %d", &ip_0, &ip_1, &ip_2, &ip_3, &mask, &inode);
	// 	// scanf("%u.%u.%u.%u", &ip_0, &ip_1, &ip_2, &ip_3);

	// 	u32 search_ip = (((ip_0 & 0xFF)<<24) | (ip_1 & 0xFF)<<16 | (ip_2 & 0xFF)<<8 | (ip_3 & 0xFF) );
	// 	// printf("%X\n", search_ip);
	// 	// u32 ip = (((ip_0 & 0xFF)<<24) | (ip_1 & 0xFF)<<16 | (ip_2 & 0xFF)<<8 | (ip_3 & 0xFF) );
		
	// 	int longest_match_inode = search_node(tree, search_ip);

	// 	if(longest_match_inode == -1){
	// 		printf("ip:%u.%u.%u.%u is not in this tree\n", ip_0, ip_1, ip_2, ip_3);
	// 	} else {
	// 		printf("According to the longest match, the inode is %d\n", longest_match_inode);
	// 	}
	// }
	////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////
	/* 2bit:  Test all data  */

	// printf("Input an ip search in tree:\n");


	fseek(fp, 0, SEEK_SET);
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
		// printf("ip: %X\n", search_ip);
		// u32 ip = (((ip_0 & 0xFF)<<24) | (ip_1 & 0xFF)<<16 | (ip_2 & 0xFF)<<8 | (ip_3 & 0xFF) );
		int longest_match_inode = search_node(tree, search_ip);
		
		// if(longest_match_inode != inode){
			// printf("ip:%u.%u.%u.%u inode:%d & %d\n", ip_0, ip_1, ip_2, ip_3, longest_match_inode, inode);
		// } else {
		printf("%d\n", longest_match_inode);
		// }
	}
	////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////


	
	fclose(fp);


}