#include "arpcache.h"
#include "arp.h"
#include "ether.h"
#include "packet.h"
#include "icmp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

static arpcache_t arpcache;

// initialize IP->mac mapping, request list, lock and sweeping thread
void arpcache_init()
{
	bzero(&arpcache, sizeof(arpcache_t));

	init_list_head(&(arpcache.req_list));

	pthread_mutex_init(&arpcache.lock, NULL);

	pthread_create(&arpcache.thread, NULL, arpcache_sweep, NULL);
}

// release all the resources when exiting
void arpcache_destroy()
{
	pthread_mutex_lock(&arpcache.lock);

	struct arp_req *req_entry = NULL, *req_q;
	list_for_each_entry_safe(req_entry, req_q, &(arpcache.req_list), list) {
		struct cached_pkt *pkt_entry = NULL, *pkt_q;
		list_for_each_entry_safe(pkt_entry, pkt_q, &(req_entry->cached_packets), list) {
			list_delete_entry(&(pkt_entry->list));
			free(pkt_entry->packet);
			free(pkt_entry);
		}

		list_delete_entry(&(req_entry->list));
		free(req_entry);
	}

	pthread_kill(arpcache.thread, SIGTERM);

	pthread_mutex_unlock(&arpcache.lock);
}

// lookup the IP->mac mapping
//
// traverse the table to find whether there is an entry with the same IP
// and mac address with the given arguments
int arpcache_lookup(u32 ip4, u8 mac[ETH_ALEN])
{
	// pthread_mutex_lock(&arpcache.lock);
	fprintf(stderr, "TODO: lookup ip address in arp cache.\n");
	for(int i = 0; i < 32; i++){
		if((arpcache.entries[i].ip4 == ip4) && arpcache.entries[i].valid){
			memcpy(mac, arpcache.entries[i].mac, ETH_ALEN);
			return 1;
		}
	}
	// pthread_mutex_unlock(&arpcache.lock);

	return 0;
}

// append the packet to arpcache
//
// Lookup in the list which stores pending packets, if there is already an
// entry with the same IP address and iface (which means the corresponding arp
// request has been sent out), just append this packet at the tail of that entry
// (the entry may contain more than one packet); otherwise, malloc a new entry
// with the given IP address and iface, append the packet, and send arp request.
void arpcache_append_packet(iface_info_t *iface, u32 ip4, char *packet, int len)
{
	fprintf(stderr, "TODO: append the ip address if lookup failed, and send arp request if necessary.\n");
	pthread_mutex_lock(&arpcache.lock);
	struct arp_req *p = NULL;
	// printf("aaaaaaaaaaaaaaaa\n");

	struct cached_pkt *new_packet = (struct cached_pkt *)malloc(sizeof(struct cached_pkt));
	// printf("bbbbbbbbbbbb\n");
	new_packet->packet = packet;
	new_packet->len = len;

	list_for_each_entry(p, &(arpcache.req_list), list){
		if(p->ip4 == ip4 && p->iface == iface){
			list_add_tail((struct list_head *)new_packet, &(p->cached_packets));
			pthread_mutex_unlock(&arpcache.lock);
			return;
		}
	}

	struct arp_req * new_arp_req = (struct arp_req*)malloc(sizeof(struct arp_req));
	new_arp_req->iface = iface;
	new_arp_req->ip4 = ip4;
	new_arp_req->retries = 0;
	new_arp_req->sent = time(NULL);
	list_add_tail((struct list_head*)new_arp_req, &(arpcache.req_list));
	init_list_head(&(new_arp_req->cached_packets));
	list_add_tail((struct list_head*)new_packet, &(new_arp_req->cached_packets));

	arp_send_request(iface, ip4);
	pthread_mutex_unlock(&arpcache.lock);


}

// insert the IP->mac mapping into arpcache, if there are pending packets
// waiting for this mapping, fill the ethernet header for each of them, and send
// them out
void arpcache_insert(u32 ip4, u8 mac[ETH_ALEN])
{
	pthread_mutex_lock(&arpcache.lock);

	fprintf(stderr, "TODO: insert ip->mac entry, and send all the pending packets.\n");
	int find = -1;
	for(int i = 0; i < 32; i++){
		if(arpcache.entries[i].ip4 == ip4){
			find = i;
			memcpy(arpcache.entries[find].mac, mac, ETH_ALEN);
			arpcache.entries[find].added = time(NULL);
			arpcache.entries[find].valid = 1;
			break; 
		}
	}

	int empty = -1;
	if(find == -1){
		//find an empty
		for(int i = 0; i < 32; i++){
			if(arpcache.entries[i].valid == 0){
				empty = i;
				break;
			}
		}
		if(empty == -1){
			//randomly replace
			srand((unsigned)time(NULL));
			empty = rand() % 32;
		}
		arpcache.entries[empty].ip4 = ip4;
		memcpy(arpcache.entries[empty].mac, mac, ETH_ALEN);
		arpcache.entries[empty].added = time(NULL);
		arpcache.entries[empty].valid = 1;
	}

	//find packets that have the same ip, send out
	struct arp_req *p, *q;
	list_for_each_entry_safe(p, q, &(arpcache.req_list), list){
		if(p->ip4 == ip4){
			struct cached_pkt *p_s, *q_s;
			list_for_each_entry_safe(p_s, q_s, &(p->cached_packets), list){
				struct ether_header* eh = (struct ether_header*)(p_s->packet);
				memcpy(eh->ether_dhost, mac, ETH_ALEN);
				iface_send_packet(p->iface, p_s->packet, p_s->len);
				list_delete_entry(&(p_s->list));
				free(p_s);
			}
			list_delete_entry(&(p->list));
			free(p);
		}
	}

	pthread_mutex_unlock(&arpcache.lock);

}

// sweep arpcache periodically
//
// For the IP->mac entry, if the entry has been in the table for more than 15
// seconds, remove it from the table.
// For the pending packets, if the arp request is sent out 1 second ago, while 
// the reply has not been received, retransmit the arp request. If the arp
// request has been sent 5 times without receiving arp reply, for each
// pending packet, send icmp packet (DEST_HOST_UNREACHABLE), and drop these
// packets.
void *arpcache_sweep(void *arg) 
{
	while (1) {
		sleep(1);
		pthread_mutex_lock(&arpcache.lock);
		fprintf(stderr, "TODO: sweep arpcache periodically: remove old entries, resend arp requests .\n");
		
		//check entries
		for(int i = 0; i < 32; i++){
			if((time(NULL) - arpcache.entries[i].added) > 15){
				arpcache.entries[i].valid = 0;
			}
		}

		//check req_list
		struct arp_req *p, *q;
		list_for_each_entry_safe(p, q, &(arpcache.req_list), list){
			if((time(NULL) - p->sent) > 1){
				//resend
				p->retries++;
				p->sent = time(NULL);
				arp_send_request(p->iface, p->ip4);
			} else if(p->retries > 5) {
				struct cached_pkt *p_s, *q_s;
				list_for_each_entry_safe(p_s, q_s, &(p->cached_packets), list){
					pthread_mutex_unlock(&arpcache.lock);
					icmp_send_packet(p_s->packet, p_s->len, ICMP_DEST_UNREACH, ICMP_HOST_UNREACH);
					pthread_mutex_lock(&arpcache.lock);
					list_delete_entry(&(p_s->list));
					free(p_s);
				}
			}
		
		}

		pthread_mutex_unlock(&arpcache.lock);
	
	}

	return NULL;
}

