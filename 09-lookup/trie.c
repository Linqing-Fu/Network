#include "trie.h"

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

int extract(u32 ip, int offset, int k){
	unsigned int temp = 0xFFFFFFFF;
	int value = (ip >> (32 - (offset + k))) & (temp >> (32 - k));
	return value;
}

void insert_node(Trie_node root, u32 ip, int mask, int inode){
	if(root == NULL)
		return;
	Node *t = root;
	int offset = 0;
	int index = extract(ip, offset, 1);
	for(int i = 0; i < mask; i++){
		if(t->child[index] == NULL){
			Node *tmp = CreateTrie();
			t->child[index] = tmp;
		}
		t = t->child[index];
		offset += 1;
		index = extract(ip, offset, 1);
	}
	t->leaf = 1;
	t->inode = inode;
}

int search_node(Trie_node root, u32 ip){
	if(root == NULL){
		printf("tree is empty\n");
		return -1;
	}
	int longest_match_inode = -1;
	Node *t = root;
	int offset = 0;
	int index = extract(ip, offset, 1);
	for(int i = 0; i < 32; i++){
		if(t->child[index] != NULL){
			if(t->leaf == 1){
				longest_match_inode = t->inode;
			}
			t = t->child[index];
			offset += 1;
			index = extract(ip, offset, 1);
		} else{
			break;
		}
	}
	return longest_match_inode;
		// if(longest_match_inode == -1){
		// 	printf("ip:%u is not in this tree\n", ip);
		// } else {
		// 	printf("according to the longest match, the inode is %d\n", longest_match_inode);
		// }


}

int main(){
	u8 ip_0[697882];
	u8 ip_1[697882];
	u8 ip_2[697882];
	u8 ip_3[697882];
	int mask[697882];
	int inode[697882];	
	FILE *fp = fopen("forwarding-table.txt", "rb");
	if(!fp){
		printf("cannot open file : forwarding-table.txt\n");
	}
	Trie_node tree = (Trie_node)CreateTrie();

	//create tree
	for(int i = 0; i < 697882; i++){
		fscanf(fp, "%u.%u.%u.%u %d %d\n", &ip_0[i], &ip_1[i], &ip_2[i], &ip_3[i], &mask[i], &inode[i]);
		u32 ip = (((ip_0[i] & 0xFF)<<24) | (ip_1[i] & 0xFF)<<16 | (ip_2[i] & 0xFF)<<8 | (ip_3[i] & 0xFF) );
		insert_node(tree, ip, mask[i], inode[i]);		
	}

	printf("Input an ip search in tree:\n");
	scanf("%u.%u.%u.%u\n", ip_0[0], ip_1[0], ip_2[0], ip_3[0]);
	u32 search_ip = (((ip_0[0] & 0xFF)<<24) | (ip_1[0] & 0xFF)<<16 | (ip_2[0] & 0xFF)<<8 | (ip_3[0] & 0xFF) );
	search_node(tree, search_ip);
	

}