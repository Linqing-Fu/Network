#include "arp.h"
#include "base.h"
#include "types.h"
#include "packet.h"
#include "ether.h"
#include "arpcache.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #include "log.h"

// send an arp request: encapsulate an arp request packet, send it out through
// iface_send_packet
void arp_send_request(iface_info_t *iface, u32 dst_ip)
{
	fprintf(stderr, "TODO: send arp request when lookup failed in arpcache.\n");
	char *packet = malloc(ETHER_HDR_SIZE + ETHER_ARP_SIZE);
	//dest mac unknown, set FF:FF:FF:FF:FF:FF, broadcast
	memset(((struct ether_header *)packet)->ether_dhost, 0xFF, ETH_ALEN);
	memcpy(((struct ether_header *)packet)->ether_shost, iface->mac, ETH_ALEN);
	((struct ether_header *)packet)->ether_type = htons(ETH_P_ARP);

	struct ether_arp *temp = (struct ether_arp *)(packet + ETHER_HDR_SIZE);
	temp->arp_op = htons(ARPOP_REQUEST);
	temp->arp_hrd = htons(0x01);
	temp->arp_pro = htons(0x0800);
	temp->arp_hln = 0x6;
	temp->arp_pln = 0x4;
	memcpy(temp->arp_sha, iface->mac, ETH_ALEN);
	temp->arp_spa = htonl(iface->ip);
	memset(temp->arp_tha, 0, ETH_ALEN);
	temp->arp_tpa = htonl(dst_ip);
	iface_send_packet(iface, packet, ETHER_HDR_SIZE + ETHER_ARP_SIZE);


}

// send an arp reply packet: encapsulate an arp reply packet, send it out
// through iface_send_packet
void arp_send_reply(iface_info_t *iface, struct ether_arp *req_hdr)
{
	fprintf(stderr, "TODO: send arp reply when receiving arp request.\n");

	char *packet = malloc(ETHER_HDR_SIZE + ETHER_ARP_SIZE);

	memcpy(((struct ether_header *)packet)->ether_dhost, req_hdr->arp_sha, ETH_ALEN);
	memcpy(((struct ether_header *)packet)->ether_shost, iface->mac, ETH_ALEN);
	((struct ether_header *)packet)->ether_type = htons(ETH_P_ARP);

	struct ether_arp *temp = (struct ether_arp *)(packet + ETHER_HDR_SIZE);
	temp->arp_op = htons(ARPOP_REPLY);
	temp->arp_hrd = htons(0x01);
	temp->arp_pro = htons(0x0800);
	temp->arp_hln = 0x6;
	temp->arp_pln = 0x4;
	memcpy(temp->arp_sha, iface->mac, ETH_ALEN);
	temp->arp_spa = htonl(iface->ip);
	memcpy(temp->arp_tha, req_hdr->arp_sha, ETH_ALEN);
	temp->arp_tpa = htonl(req_hdr->arp_spa);
	iface_send_packet(iface, packet, ETHER_HDR_SIZE + ETHER_ARP_SIZE);


}



void handle_arp_packet(iface_info_t *iface, char *packet, int len)
{
	fprintf(stderr, "TODO: process arp packet: arp request & arp reply.\n");
	// struct ether_header *header = (struct ether_header *)packet;
	struct ether_arp *arp = (struct ether_arp *)(packet + ETHER_HDR_SIZE);
	if(ntohl(arp->arp_tpa) == iface->ip && ntohs(arp->arp_op) == ARPOP_REQUEST){
		arp_send_reply(iface, arp);
	} else {
		arpcache_insert(ntohl(arp->arp_spa), arp->arp_sha);
	}
}



// send (IP) packet through arpcache lookup 
//
// Lookup the mac address of dst_ip in arpcache. If it is found, fill the
// ethernet header and emit the packet by iface_send_packet, otherwise, pending 
// this packet into arpcache, and send arp request.
void iface_send_packet_by_arp(iface_info_t *iface, u32 dst_ip, char *packet, int len)
{
	struct ether_header *eh = (struct ether_header *)packet;
	memcpy(eh->ether_shost, iface->mac, ETH_ALEN);
	eh->ether_type = htons(ETH_P_IP);

	u8 dst_mac[ETH_ALEN];
	int found = arpcache_lookup(dst_ip, dst_mac);
	if (found) {
		// log(DEBUG, "found the mac of %x, send this packet", dst_ip);
		memcpy(eh->ether_dhost, dst_mac, ETH_ALEN);
		iface_send_packet(iface, packet, len);
	}
	else {
		// log(DEBUG, "lookup %x failed, pend this packet", dst_ip);
		arpcache_append_packet(iface, dst_ip, packet, len);
	}
}
