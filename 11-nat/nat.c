#include "ip.h"
#include "tcp.h"
#include "icmp.h"
#include "rtable.h"
#include "log.h"
#include "nat.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

static struct nat_table nat;

void handle_ip_packet(iface_info_t *iface, char *packet, int len)
{
	struct iphdr *ip = packet_to_ip_hdr(packet);
	u32 daddr = ntohl(ip->daddr);
	if (daddr == iface->ip && ip->protocol == IPPROTO_ICMP) {
		struct icmphdr *icmp = (struct icmphdr *)IP_DATA(ip);
		if (icmp->type == ICMP_ECHOREQUEST) {
			icmp_send_packet(packet, len, ICMP_ECHOREPLY, 0);
		}

		free(packet);
	}
	else {
		nat_translate_packet(iface, packet, len);
	}
}

// get the interface from iface name
static iface_info_t *if_name_to_iface(const char *if_name)
{
	iface_info_t *iface = NULL;
	list_for_each_entry(iface, &instance->iface_list, list) {
		if (strcmp(iface->name, if_name) == 0)
			return iface;
	}

	log(ERROR, "Could not find the desired interface according to if_name '%s'", if_name);
	return NULL;
}

// determine the direction of the packet, DIR_IN / DIR_OUT / DIR_INVALID
static int get_packet_direction(char *packet)
{
	fprintf(stdout, "TODO: determine the direction of this packet.\n");
	struct iphdr *ip = packet_to_ip_hdr(packet);
	rt_entry_t *src = longest_prefix_match(ntohl(ip->saddr));
	rt_entry_t *dst = longest_prefix_match(ntohl(ip->daddr));

	if((src->iface->ip == (nat.internal_iface)->ip) && (dst->iface->ip == (nat.external_iface)->ip)){
		return DIR_OUT;
	} else if ((src->iface->ip == (nat.external_iface)->ip) && (ntohl(ip->daddr) == (nat.external_iface)->ip)){
		return DIR_IN;
	} else{
		return DIR_INVALID;
	}
}


int assign_external_port(){
	for(int i = 1; i < 65536; i++){
		if(nat.assigned_ports[i] == 0){
			nat.assigned_ports[i] = 1;
			return i;
		}
	}
	printf("no available port!\n");
	return -1;
}

u8 my_hash(u32 ip, u16 port){
	char buf[6];
	memcpy(buf, &ip, 4);
	memcpy(buf, &port, 2);
	return hash8(buf, 6);
}

//update ip & tcp header
void update_ip_tcp(char *packet, u32 ip_addr, u16 port, int dir){
	struct iphdr *ip = packet_to_ip_hdr(packet);
	struct tcphdr *tcp = packet_to_tcp_hdr(packet);
	if(dir == DIR_OUT){
		//change source ip & port into nat's external_ip 
		ip->saddr = htonl(ip_addr);
		tcp->sport = htons(port);

	} else if (dir == DIR_IN){
		//change dest ip & port into ip found in nat_mapping
		ip->daddr = htonl(ip_addr);
		tcp->dport = htons(port);
	}
	ip->checksum = ip_checksum(ip);
	tcp->checksum = tcp_checksum(ip, tcp);

}

//time out
void update_conn(struct nat_mapping *p, struct tcphdr *tcp){
	if(tcp-> flags == (TCP_FIN + TCP_ACK)){
		(p->conn).external_fin = 1;
	} else if(tcp->flags == TCP_RST){
		if((p->conn).internal_fin + (p->conn).external_fin == 2){
			nat.assigned_ports[p->external_port] = 0;
			list_delete_entry(&(p->list));
		}
	}
}


// do translation for the packet: replace the ip/port, recalculate ip & tcp
// checksum, update the statistics of the tcp connection
void do_translation(iface_info_t *iface, char *packet, int len, int dir)
{
	fprintf(stdout, "TODO: do translation for this packet.\n");
	struct iphdr *ip = packet_to_ip_hdr(packet);
	struct tcphdr *tcp = packet_to_tcp_hdr(packet);
	u32 saddr = ntohl(ip->saddr);
	u32 daddr = ntohl(ip->daddr);
	u16 sport = ntohs(tcp->sport);
	u16 dport = ntohs(tcp->dport);

	u32 serv_ip;
	u16 serv_port;
	//serv_ip _port: always public web
	serv_ip = (dir == DIR_OUT)?daddr:saddr;
	serv_port = (dir == DIR_OUT)?dport:sport;

	u8 hash_index = my_hash(serv_ip, serv_port);
	struct list_head *head = &(nat.nat_mapping_list[hash_index]);

	pthread_mutex_lock(&nat.lock);
	if(head->next != head){
		struct nat_mapping *p = NULL;
		list_for_each_entry(p, head, list){
			if(dir == DIR_OUT 
				&& p->internal_ip == saddr 
				&& p->internal_port == sport){

				p->update_time = time(NULL);
				update_ip_tcp(packet, p->external_ip, p->external_port, DIR_OUT);
				ip_send_packet(packet, len);
				update_conn(p, tcp);
				pthread_mutex_unlock(&nat.lock);
				return;

			} else if (dir == DIR_IN 
				&& p->external_ip == daddr
				&& p->external_port == dport){

				p->update_time = time(NULL);
				update_ip_tcp(packet, p->internal_ip, p->internal_port, DIR_IN);
				ip_send_packet(packet, len);
				update_conn(p, tcp);
				pthread_mutex_unlock(&nat.lock);
				return;
			}
		}
	}

	if(dir == DIR_OUT){
		//no corresponding map
		//add new map
		struct nat_mapping *new_map = (struct nat_mapping *)malloc(sizeof(struct nat_mapping));
		memset((char *)new_map, 0, sizeof(struct nat_mapping));

		new_map->internal_ip = saddr;
		new_map->internal_port = sport;
		new_map->external_ip = (nat.external_iface)->ip;
		new_map->external_port = assign_external_port();
		new_map->update_time = time(NULL);

		list_add_tail(&(new_map->list), head);
		update_ip_tcp(packet, new_map->external_ip, new_map->external_port, DIR_OUT);
		ip_send_packet(packet, len);
		pthread_mutex_unlock(&nat.lock);

		return;


	}

}

void nat_translate_packet(iface_info_t *iface, char *packet, int len)
{
	int dir = get_packet_direction(packet);
	if (dir == DIR_INVALID) {
		log(ERROR, "invalid packet direction, drop it.");
		icmp_send_packet(packet, len, ICMP_DEST_UNREACH, ICMP_HOST_UNREACH);
		free(packet);
		return ;
	}

	struct iphdr *ip = packet_to_ip_hdr(packet);
	if (ip->protocol != IPPROTO_TCP) {
		log(ERROR, "received non-TCP packet (0x%0hhx), drop it", ip->protocol);
		free(packet);
		return ;
	}

	do_translation(iface, packet, len, dir);
}

// nat timeout thread: find the finished flows, remove them and free port
// resource
void *nat_timeout()
{
	while (1) {
		fprintf(stdout, "TODO: sweep finished flows periodically.\n");
		sleep(1);
		time_t now = time(NULL);

		pthread_mutex_lock(&nat.lock);
		for(int i = 0; i < HASH_8BITS; i++){
			struct nat_mapping *p = NULL;
			struct nat_mapping *q = NULL;
			list_for_each_entry_safe(p, q, &(nat.nat_mapping_list[i]), list){
				if((now - p->update_time) > TCP_ESTABLISHED_TIMEOUT){
					nat.assigned_ports[p->external_port] = 0;
					list_delete_entry(&(p->list));
				}
			}
		}


		pthread_mutex_unlock(&nat.lock);
	}

	return NULL;
}

// initialize nat table
void nat_table_init()
{
	memset(&nat, 0, sizeof(nat));

	for (int i = 0; i < HASH_8BITS; i++)
		init_list_head(&nat.nat_mapping_list[i]);

	nat.internal_iface = if_name_to_iface("n1-eth0");
	nat.external_iface = if_name_to_iface("n1-eth1");
	if (!nat.internal_iface || !nat.external_iface) {
		log(ERROR, "Could not find the desired interfaces for nat.");
		exit(1);
	}

	memset(nat.assigned_ports, 0, sizeof(nat.assigned_ports));

	pthread_mutex_init(&nat.lock, NULL);

	pthread_create(&nat.thread, NULL, nat_timeout, NULL);
}

// destroy nat table
void nat_table_destroy()
{
	pthread_mutex_lock(&nat.lock);

	for (int i = 0; i < HASH_8BITS; i++) {
		struct list_head *head = &nat.nat_mapping_list[i];
		struct nat_mapping *mapping_entry, *q;
		list_for_each_entry_safe(mapping_entry, q, head, list) {
			list_delete_entry(&mapping_entry->list);
			free(mapping_entry);
		}
	}

	pthread_kill(nat.thread, SIGTERM);

	pthread_mutex_unlock(&nat.lock);
}
