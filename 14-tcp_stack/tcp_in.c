#include "tcp.h"
#include "tcp_sock.h"
#include "tcp_timer.h"

#include "log.h"
#include "ring_buffer.h"

#include <stdlib.h>
// update the snd_wnd of tcp_sock
//
// if the snd_wnd before updating is zero, notify tcp_sock_send (wait_send)
static inline void tcp_update_window(struct tcp_sock *tsk, struct tcp_cb *cb)
{
	u16 old_snd_wnd = tsk->snd_wnd;
	tsk->snd_wnd = cb->rwnd;
	if (old_snd_wnd == 0)
		wake_up(tsk->wait_send);
}

// update the snd_wnd safely: cb->ack should be between snd_una and snd_nxt
static inline void tcp_update_window_safe(struct tcp_sock *tsk, struct tcp_cb *cb)
{
	if (less_or_equal_32b(tsk->snd_una, cb->ack) && less_or_equal_32b(cb->ack, tsk->snd_nxt))
		tcp_update_window(tsk, cb);
}

#ifndef max
#	define max(x,y) ((x)>(y) ? (x) : (y))
#endif

// check whether the sequence number of the incoming packet is in the receiving
// window
static inline int is_tcp_seq_valid(struct tcp_sock *tsk, struct tcp_cb *cb)
{
	u32 rcv_end = tsk->rcv_nxt + max(tsk->rcv_wnd, 1);
	printf("cb->seq:%d rcv_end:%d rcv_nxt:%d cb->seq_end:%d\n", cb->seq, rcv_end, tsk->rcv_nxt, cb->seq_end);
	printf("%d %d\n", less_than_32b(cb->seq, rcv_end), less_or_equal_32b(tsk->rcv_nxt, cb->seq_end));
	if (less_than_32b(cb->seq, rcv_end) && less_or_equal_32b(tsk->rcv_nxt, cb->seq_end)) {
		return 1;
	}
	else {
		log(ERROR, "received packet with invalid seq, drop it.");
		return 0;
	}
}

struct tcp_sock *set_up_child_tsk(struct tcp_sock *tsk, struct tcp_cb *cb) {
    struct tcp_sock *csk = alloc_tcp_sock();
    printf("now state: %s\n", tcp_state_str[tsk->state]);
    tcp_set_state(csk, TCP_SYN_RECV);

    csk->sk_sip = cb->daddr;
    csk->sk_dip = cb->saddr;
    csk->sk_sport = cb->dport;
    csk->sk_dport = cb->sport;

    csk->parent = tsk;

    csk->rcv_nxt = cb->seq_end;

    csk->iss = tcp_new_iss();
    csk->snd_nxt = csk->iss;

    struct sock_addr skaddr;
    skaddr.ip = htonl(csk->sk_sip);
    skaddr.port = htons(csk->sk_sport);
    if(tcp_sock_bind(csk, &skaddr) < 0){
    	exit(-1);
    }
    list_add_tail(&csk->list, &tsk->listen_queue);
    return csk;
}

void tcp_sock_listen_dequeue(struct tcp_sock *tsk) {
    if (tsk->parent) {
        if (!list_empty(&tsk->parent->listen_queue)) {
            struct tcp_sock *entry = NULL;
            list_for_each_entry(entry, &(tsk->parent->listen_queue), list) 
                if (entry == tsk){
                    list_delete_entry(&(entry->list));
                    break;             
                }
        }
    }
}

// Process the incoming packet according to TCP state machine. 
void tcp_process(struct tcp_sock *tsk, struct tcp_cb *cb, char *packet)
{
	fprintf(stdout, "TODO: implement %s please.\n", __FUNCTION__);
	int state = tsk->state;
	if(state == TCP_CLOSED){
		tcp_send_reset(cb);
		return;
	}
	//server
	if(state == TCP_LISTEN){
		struct tcp_sock *p = set_up_child_tsk(tsk, cb);
	    tcp_send_control_packet(p, TCP_SYN | TCP_ACK);
	    // tcp_set_state(tsk, TCP_SYN_RECV); //markflag
    	tcp_hash(p);
		return;
	}
	//client
	if(state == TCP_SYN_SENT){
		struct tcphdr *tcp_hdr = packet_to_tcp_hdr(packet);
    	if (tcp_hdr->flags & (TCP_SYN | TCP_ACK)) {
        	// tsk->rcv_nxt = cb->seq_end;
        	tsk->rcv_nxt = cb->seq + 1;//markflag
        	tsk->snd_una = cb->ack;
        	tcp_send_control_packet(tsk, TCP_ACK);
        	tcp_set_state(tsk, TCP_ESTABLISHED);
        	wake_up(tsk->wait_connect);
    	} else{
        	tcp_send_reset(cb);
    	}
		return;
	}
	if(is_tcp_seq_valid(tsk, cb) == 0){
		printf("drop the packet\n");
		return;
	}
	if(TCP_RST & cb->flags){
		tcp_set_state(tsk, TCP_CLOSED);
		tcp_unhash(tsk);
		return;
	}
	if(TCP_SYN & cb->flags){
		tcp_send_reset(cb);
		tcp_set_state(tsk, TCP_CLOSED);
		return;
	}
	if(!(TCP_ACK & cb->flags)){
		tcp_send_reset(cb);
		// printf("emmmmmmmmmm\n");
		return;
	}
	tsk->snd_una = cb->ack;
	tsk->rcv_nxt = cb->seq_end;
	//server
	if((state == TCP_SYN_RECV) && (cb->ack == tsk->snd_nxt)){
		tcp_sock_listen_dequeue(tsk);
	    tcp_sock_accept_enqueue(tsk);
	    // tcp_set_state(tsk, TCP_ESTABLISHED);
	    wake_up(tsk->parent->wait_accept);
	    return;
	}

	//client
	if ((state == TCP_FIN_WAIT_1) && (cb->ack == tsk->snd_nxt))
        tcp_set_state(tsk, TCP_FIN_WAIT_2);
    //client
    if ((state == TCP_FIN_WAIT_2) && (cb->flags & (TCP_FIN | TCP_ACK))) {//markflag
    	tsk->rcv_nxt = cb->seq + 1;
        tcp_set_state(tsk, TCP_TIME_WAIT);
        tcp_set_timewait_timer(tsk); //closed by timewait for 2 MSL
        tcp_send_control_packet(tsk, TCP_ACK);
    }

    //server
    if ((state == TCP_ESTABLISHED) && (cb->flags & TCP_FIN)) {
    	printf("yes change to closewait\n");
    	tsk->rcv_nxt = cb->seq + 1;
        tcp_set_state(tsk, TCP_CLOSE_WAIT);
        tcp_send_control_packet(tsk, TCP_ACK);

    }

    //server
    if ((state == TCP_LAST_ACK) && (cb->ack == tsk->snd_nxt))
        tcp_set_state(tsk, TCP_CLOSED);

    if ((state == TCP_ESTABLISHED) && (cb->flags == TCP_ACK))
        tcp_send_control_packet(tsk, TCP_ACK);


}

