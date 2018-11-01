#include "icmp.h"
#include "ip.h"
#include "rtable.h"
#include "arp.h"
#include "base.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// send icmp packet
// void icmp_send_packet(const char *in_pkt, int len, u8 type, u8 code)
// {
// 	fprintf(stderr, "TODO: malloc and send icmp packet.\n");
// 	long size = 0;
// 	struct iphdr *ip = packet_to_ip_hdr(in_pkt);
	
// 	if(type != ICMP_ECHOREPLY){
//         size = ETHER_HDR_SIZE + IP_BASE_HDR_SIZE + ICMP_HDR_SIZE 
//         + IP_HDR_SIZE(ip) + ICMP_COPIED_DATA_LEN;
//     } else {
//         size = len - (IP_HDR_SIZE(ip) - IP_BASE_HDR_SIZE);
// 	}
// 	char *packet = (char *)malloc(size);

//     struct ether_header *eh = (struct ether_header *)packet;
//     struct iphdr *ip_header = (struct iphdr*)(packet + ETHER_HDR_SIZE);
//     struct icmphdr *icmp_header = (struct icmphdr*)(packet + ETHER_HDR_SIZE + IP_BASE_HDR_SIZE);

//     eh->ether_type = htons(ETH_P_IP);

//     u32 daddr = ntohl(ip->saddr);
//     u32 saddr = longest_prefix_match(daddr)->iface->ip;

// 	ip_init_hdr(ip_header, saddr, daddr, size - ETHER_HDR_SIZE, IPPROTO_ICMP);

//     memset(icmp_header, 0, ICMP_HDR_SIZE);
// 	icmp_header->type = type;
// 	icmp_header->code = code;

//     // memset(icmp_header + 4, 0x0, 4);
//     if(type == ICMP_ECHOREPLY){
//         memcpy(((char *)icmp_header + 4), (char *)(ip + ETHER_HDR_SIZE + IP_HDR_SIZE(ip) + 4), len - ETHER_HDR_SIZE - IP_HDR_SIZE(ip) - 4);       
//     } else {
//         memcpy(((char *)icmp_header + 8), (char *)ip, IP_HDR_SIZE(ip) + 8);
//         // memcpy(packet, ip + IP_HDR_SIZE(ip), ICMP_COPIED_DATA_LEN);      
//     }
// 	icmp_header->checksum = icmp_checksum(icmp_header, size - ETHER_HDR_SIZE - IP_BASE_HDR_SIZE);

// 	ip_send_packet(packet, size);
// }

void icmp_send_packet(const char *in_pkt, int len, u8 type, u8 code)
{
    struct iphdr *in_ip_hdr = packet_to_ip_hdr(in_pkt);
    u32 out_daddr = ntohl(in_ip_hdr->saddr);
    u32 out_saddr = longest_prefix_match(out_daddr)->iface->ip;

    long tot_size = 0;
    char *out_pkt = NULL;
    if (type != ICMP_ECHOREPLY)
        tot_size = ETHER_HDR_SIZE + IP_BASE_HDR_SIZE + ICMP_HDR_SIZE +
            IP_HDR_SIZE(in_ip_hdr) + 8;
    else
        tot_size = len - IP_HDR_SIZE(in_ip_hdr) + IP_BASE_HDR_SIZE;

    out_pkt = (char*) malloc(tot_size);

    struct icmphdr *icmp = (struct icmphdr *)(out_pkt + ETHER_HDR_SIZE + IP_BASE_HDR_SIZE);

    struct ether_header *eh = (struct ether_header *)out_pkt;
    eh->ether_type = htons(ETH_P_IP);

    struct iphdr *out_ip_hdr = packet_to_ip_hdr(out_pkt);
    ip_init_hdr(out_ip_hdr,
                out_saddr,
                out_daddr,
                tot_size - ETHER_HDR_SIZE,
                IPPROTO_ICMP);

    memset(icmp, 0, ICMP_HDR_SIZE);
    icmp->code = code;
    icmp->type = type;

    if (type != ICMP_ECHOREPLY)
        memcpy(((char*)icmp + ICMP_HDR_SIZE),
               (char*)in_ip_hdr,
               IP_HDR_SIZE(in_ip_hdr) + 8);
    else
        memcpy(((char*)icmp) + ICMP_HDR_SIZE - 4,
               (char *)in_pkt + ETHER_HDR_SIZE + IP_HDR_SIZE(in_ip_hdr) + 4,
               len - ETHER_HDR_SIZE - IP_HDR_SIZE(in_ip_hdr) - 4);
    icmp->checksum =
        icmp_checksum(icmp, tot_size-ETHER_HDR_SIZE-IP_BASE_HDR_SIZE);
    ip_send_packet(out_pkt, tot_size);
}
