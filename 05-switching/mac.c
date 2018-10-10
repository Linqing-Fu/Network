#include "mac.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>

mac_port_map_t mac_port_map;

// initialize mac_port table
void init_mac_port_table()
{
	bzero(&mac_port_map, sizeof(mac_port_map_t));

	for (int i = 0; i < HASH_8BITS; i++) {
		init_list_head(&mac_port_map.hash_table[i]);
	}

	pthread_mutex_init(&mac_port_map.lock, NULL);

	pthread_create(&mac_port_map.thread, NULL, sweeping_mac_port_thread, NULL);
}

// destroy mac_port table
void destory_mac_port_table()
{
	pthread_mutex_lock(&mac_port_map.lock);
	mac_port_entry_t *entry, *q;
	for (int i = 0; i < HASH_8BITS; i++) {
		list_for_each_entry_safe(entry, q, &mac_port_map.hash_table[i], list) {
			list_delete_entry(&entry->list);
			free(entry);
		}
	}
	pthread_mutex_unlock(&mac_port_map.lock);
}

// lookup the mac address in mac_port table
iface_info_t *lookup_port(u8 mac[ETH_ALEN])
{
	// TODO: implement the lookup process here
	fprintf(stdout, "TODO: implement the lookup process here.\n");
	pthread_mutex_lock(&mac_port_map.lock);

	mac_port_entry_t *temp;
	u8 index = hash8(mac, ETH_ALEN);
	int find_the_node = 0;

	list_for_each_entry(temp, &mac_port_map.hash_table[index], list){
		find_the_node = 0;
		for(int i = 0; i < ETH_ALEN; i++){
			if(mac[i] == temp->mac[i]){
				find_the_node = 1;
			} else {
				find_the_node = 0;
				break;
			}
		}
		if(find_the_node == 1)
			break;
	}
	pthread_mutex_unlock(&mac_port_map.lock);
	if(find_the_node){
		return temp->iface;
	}
	return NULL;

}

// insert the mac -> iface mapping into mac_port table
void insert_mac_port(u8 mac[ETH_ALEN], iface_info_t *iface)
{
	// TODO: implement the insertion process here
	fprintf(stdout, "TODO: implement the insertion process here.\n");

	pthread_mutex_lock(&mac_port_map.lock);

	//check if the node already in mac_port table
	mac_port_entry_t *temp;
	u8 index = hash8(mac, ETH_ALEN);
	int find_the_node = 0;

	list_for_each_entry(temp, &mac_port_map.hash_table[index], list){
		find_the_node = 0;
		for(int i = 0; i < ETH_ALEN; i++){
			if(mac[i] == temp->mac[i]){
				find_the_node = 1;
			} else {
				find_the_node = 0;
				break;
			}
		}
		if(find_the_node == 1)
			break;
	}

	if(find_the_node == 0){
		//create a node to save data
		mac_port_entry_t *insert_node = malloc(sizeof(mac_port_entry_t));
		memcpy(insert_node->mac, mac, sizeof(u8) * ETH_ALEN);
		insert_node->iface = iface;
		insert_node->visited = time(NULL);

		//add it
		list_add_tail(&insert_node->list, &mac_port_map.hash_table[index]);

	} else {
		//already in the table, so update the time
		temp->visited = time(NULL);
	}
	
	pthread_mutex_unlock(&mac_port_map.lock);

}

// dumping mac_port table
void dump_mac_port_table()
{
	mac_port_entry_t *entry = NULL;
	time_t now = time(NULL);

	fprintf(stdout, "dumping the mac_port table:\n");
	pthread_mutex_lock(&mac_port_map.lock);
	for (int i = 0; i < HASH_8BITS; i++) {
		list_for_each_entry(entry, &mac_port_map.hash_table[i], list) {
			fprintf(stdout, ETHER_STRING " -> %s, %d\n", ETHER_FMT(entry->mac), \
					entry->iface->name, (int)(now - entry->visited));
		}
	}

	pthread_mutex_unlock(&mac_port_map.lock);
}

// sweeping mac_port table, remove the entry which has not been visited in the
// last 30 seconds.
int sweep_aged_mac_port_entry()
{
	// TODO: implement the sweeping process here
	fprintf(stdout, "TODO: implement the sweeping process here.\n");
	pthread_mutex_lock(&mac_port_map.lock);

	mac_port_entry_t *entry, *q;
	for (int i = 0; i < HASH_8BITS; i++) {
		list_for_each_entry_safe(entry, q, &mac_port_map.hash_table[i], list) {
			if((time(NULL) - entry->visited) > MAC_PORT_TIMEOUT){
				list_delete_entry(&entry->list);
				free(entry);
			}
		}
	}


	pthread_mutex_unlock(&mac_port_map.lock);

	return 0;
}

// sweeping mac_port table periodically, by calling sweep_aged_mac_port_entry
void *sweeping_mac_port_thread(void *nil)
{
	while (1) {
		sleep(1);
		int n = sweep_aged_mac_port_entry();

		if (n > 0)
			log(DEBUG, "%d aged entries in mac_port table are removed.", n);
	}

	return NULL;
}
