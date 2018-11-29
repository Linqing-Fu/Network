#include "mospf_daemon.h"
#include "mospf_proto.h"
#include "mospf_nbr.h"
#include "mospf_database.h"

#include "ip.h"

#include "list.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

extern ustack_t *instance;

pthread_mutex_t mospf_lock;

void mospf_init()
{
	pthread_mutex_init(&mospf_lock, NULL);

	instance->area_id = 0;
	// get the ip address of the first interface
	iface_info_t *iface = list_entry(instance->iface_list.next, iface_info_t, list);
	instance->router_id = iface->ip;
	instance->sequence_num = 0;
	instance->lsuint = MOSPF_DEFAULT_LSUINT;

	iface = NULL;
	list_for_each_entry(iface, &instance->iface_list, list) {
		iface->helloint = MOSPF_DEFAULT_HELLOINT;
		init_list_head(&iface->nbr_list);
	}

	init_mospf_db();
}

void *sending_mospf_hello_thread(void *param);
void *sending_mospf_lsu_thread(void *param);
void *checking_nbr_thread(void *param);

void mospf_run()
{
	pthread_t hello, lsu, nbr;
	pthread_create(&hello, NULL, sending_mospf_hello_thread, NULL);
	pthread_create(&lsu, NULL, sending_mospf_lsu_thread, NULL);
	pthread_create(&nbr, NULL, checking_nbr_thread, NULL);
}

void *sending_mospf_hello_thread(void *param)
{
	int len = ETHER_HDR_SIZE + IP_BASE_HER_SIZE + MOSPF_HDR_SIZE + MOSPF_HELLO_SIZE; 
	while(1){
		fprintf(stdout, "TODO: send mOSPF Hello message periodically.\n");
		sleep(iface->helloint);
		iface_info_t *iface;
		list_for_each_entry(iface, &(instance->iface_list), list){
			char *packet = (char *)malloc(len);
			memset(packet, 0, len);
			
			//ether header
			struct ether_header *ether_hdr = (struct ether_header *)packet;
			u8 dst_mac[ETH_LEN] = {0x1, 0x0, 0x5E, 0x0, 0x0, 0x5};
			ether_hdr->ether_type = ntohs(ETH_P_IP);
			memcpy(ether_hdr->ether_shost, iface->mac, ETH_LEN);
			memcpy(ether_hdr->ether_dhost, dst_mac, ETH_LEN);		

			//ip header
			struct iphdr *ip = packet_to_ip_hdr(packet);
			ip_init_hdr(ip, iface->ip, 0xE0000005, (IP_BASE_HER_SIZE + MOSPF_HDR_SIZE + MOSPF_HELLO_SIZE), 90);

			//mospf header
			struct mospf_hdr *mospf = (struct mospf_hdr *)((char *)ip + IP_BASE_HER_SIZE);
			mospf_init_hdr(mospf, 1, MOSPF_HDR_SIZE + MOSPF_HELLO_SIZE, instance->router_id, 0x0);

			//mospf msg
			struct mospf_hello *hello = (struct mospf_hello *)((char*)mospf + MOSPF_HDR_SIZE);
			mospf_init_hello(hello, iface->mask);

			mospf->checksum = mospf_checksum(mospf);
			ip->checksum = ip_checksum(ip);

			iface_send_packet(iface, packet, len);


		}


	}
	return NULL;
}

void *checking_nbr_thread(void *param)
{
	fprintf(stdout, "TODO: neighbor list timeout operation.\n");
	while(1){
		sleep(1);
		pthread_mutex_lock(&mospf_lock);
		iface_info_t *iface;
		list_for_each_entry(iface, &(instance->iface_list), list){
			mospf_nbr_t *p = NULL;
			mospf_nbr_t *q = NULL;
			if(!list_empty(&(iface->nbr_list))){
				list_for_each_entry_safe(p, q, &(iface->nbr_list)){
					p->alive++;
					if(p->alive > (3 * MOSPF_DEFAULT_HELLOINT)){
						list_delete_entry(&(p->list));
						iface->num_nbr--;
						send_mospf_lsu();
					}
				}
			}
		}


		pthread_mutex_unlock(&mospf_lock);
	}

	return NULL;
}

void handle_mospf_hello(iface_info_t *iface, const char *packet, int len)
{
	fprintf(stdout, "TODO: handle mOSPF Hello message.\n");
	struct iphdr *ip = (struct iphdr *)(packet + ETHER_HDR_SIZE);
	struct mospf_hdr *mospf = (struct mospf_hdr *)((char *)ip + IP_HDR_SIZE(ip));
	struct mospf_hello *hello = (struct mospf_hello *)((char*)mospf + MOSPF_HDR_SIZE);

	mospf_nbr_t *p = NULL;
	pthread_mutex_lock(&mospf_lock);
	if((iface->nbr_list).next != (iface->nbr_list)){
		list_for_each_entry(p, &(iface->nbr_list), list){
			if(p->nbr_id == mospf->rid){
				p->alive = 0;///////////////////////////////
				pthread_mutex_unlock(&mospf_lock);
				return;
			}
		}	
	}

	mospf_nbr_t *new = (mospf_nbr_t *)malloc(sizeof(mospf_nbr_t));
	memset((char *)new, 0, sizeof(mospf_nbr_t));

	new->nbr_id = ntohl(mospf->rid);
	new->nbr_ip = ntohl(ip->saddr);
	new->mask = ntohl(hello->mask);
	new->alive = 0;////////////////////////////

	list_add_tail(&(new->list), &(iface->nbr_list));
	iface->num_nbr ++;
	send_mospf_lsu();
	pthread_mutex_unlock(&mospf_lock);

	return;

}

void *sending_mospf_lsu_thread(void *param)
{
	fprintf(stdout, "TODO: send mOSPF LSU message periodically.\n");

	return NULL;
}

void handle_mospf_lsu(iface_info_t *iface, char *packet, int len)
{
	fprintf(stdout, "TODO: handle mOSPF LSU message.\n");
}

void handle_mospf_packet(iface_info_t *iface, char *packet, int len)
{
	struct iphdr *ip = (struct iphdr *)(packet + ETHER_HDR_SIZE);
	struct mospf_hdr *mospf = (struct mospf_hdr *)((char *)ip + IP_HDR_SIZE(ip));

	if (mospf->version != MOSPF_VERSION) {
		log(ERROR, "received mospf packet with incorrect version (%d)", mospf->version);
		return ;
	}
	if (mospf->checksum != mospf_checksum(mospf)) {
		log(ERROR, "received mospf packet with incorrect checksum");
		return ;
	}
	if (ntohl(mospf->aid) != instance->area_id) {
		log(ERROR, "received mospf packet with incorrect area id");
		return ;
	}

	// log(DEBUG, "received mospf packet, type: %d", mospf->type);

	switch (mospf->type) {
		case MOSPF_TYPE_HELLO:
			handle_mospf_hello(iface, packet, len);
			break;
		case MOSPF_TYPE_LSU:
			handle_mospf_lsu(iface, packet, len);
			break;
		default:
			log(ERROR, "received mospf packet with unknown type (%d).", mospf->type);
			break;
	}
}
