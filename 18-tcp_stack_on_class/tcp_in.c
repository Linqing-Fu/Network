#include "tcp.h"
#include "tcp_sock.h"
#include "tcp_timer.h"

#include "log.h"
#include "ring_buffer.h"

#include <stdlib.h>
#include <unistd.h>
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
    tcp_set_state(csk, TCP_SYN_RECV);

    csk->sk_sip = cb->daddr;
    csk->sk_dip = cb->saddr;
    csk->sk_sport = cb->dport;
    csk->sk_dport = cb->sport;

    csk->parent = tsk;

    csk->rcv_nxt = cb->seq_end;

    // csk->iss = tcp_new_iss();
    // csk->snd_nxt = csk->iss;
    csk->iss = tsk->iss;
    csk->snd_una = tsk->snd_una;
    csk->snd_nxt = tsk->snd_nxt;
	csk->rcv_nxt = cb->seq_end;
	csk->snd_wnd = tsk->snd_wnd;
	csk->rcv_wnd = tsk->rcv_wnd;


    struct sock_addr skaddr;
    skaddr.ip = htonl(csk->sk_sip);
    skaddr.port = htons(csk->sk_sport);

    if(tcp_sock_bind(csk, &skaddr) < 0){
    	exit(-1);
    }
    // tcp_set_state(csk, TCP_SYN_RECV);
    list_add_tail(&csk->list, &tsk->listen_queue);
    return csk;
}

void tcp_sock_listen_dequeue(struct tcp_sock *tsk) {
    if (tsk->parent) {
        if (!list_empty(&tsk->parent->listen_queue)) {
            struct tcp_sock *entry = NULL;
            list_for_each_entry(entry, &(tsk->parent->listen_queue), list) 
                if (entry == tsk)
                    list_delete_entry(&(entry->list));                
        }
    }
}

// Process the incoming packet according to TCP state machine. 

void tcp_process(struct tcp_sock *tsk, struct tcp_cb *cb, char *packet)
{
	// fprintf(stdout, "TODO: implement this function please.\n");
	printf("in tcp_process\n");

	int state = tsk->state;
	struct tcp_sock *p;
  	struct tcphdr *tcp_hdr;


	if (state == TCP_CLOSED) {
		printf("1\n");
		tcp_send_reset(cb);
		return ;
	}

	if (state == TCP_LISTEN) {
		printf("2\n");
		if(cb->flags & TCP_SYN){
			p = set_up_child_tsk(tsk, cb);
			tcp_send_control_packet(p, TCP_SYN | TCP_ACK);
      		tcp_hash(p);
		} else {
			tcp_send_reset(cb);
		}

		return;
	}

	if (state == TCP_SYN_SENT) {
		printf("3\n");
		tcp_hdr = packet_to_tcp_hdr(packet);
    	if (tcp_hdr->flags & (TCP_SYN | TCP_ACK)) {
        	tsk->rcv_nxt = cb->seq_end;//markflag
        	tsk->snd_una = cb->ack;
        	tcp_send_control_packet(tsk, TCP_ACK);
        	tcp_set_state(tsk, TCP_ESTABLISHED);
        	wake_up(tsk->wait_connect);
    	} else{
        	tcp_send_reset(cb);
    	}
		return ;
	}

	if (!is_tcp_seq_valid(tsk, cb)) {
		log(ERROR, "tcp_process(): received packet with invalid seq, drop it.");
		return ;
	}
// printf("aaaaaaaaaaaaaaaaaa\n");
	if (cb->flags & TCP_RST) {
		printf("4\n");
		tcp_set_state(tsk, TCP_CLOSED);
		tcp_unhash(tsk);
		return ;
	}

	if (cb->flags & TCP_SYN) {
		printf("5\n");
		tcp_send_reset(cb);
		tcp_set_state(tsk, TCP_CLOSED);
		return ;
	}
// printf("bbbbbbbbbbbbbbbbbb\n");
	if (!(cb->flags & TCP_ACK)) {
		printf("6\n");
		tcp_send_reset(cb);
		log(ERROR, "tcp_process(): TCP_ACK is not set.");
		return ;
	}
	
	//receive data packet
	if (cb->flags == (TCP_PSH | TCP_ACK)){ //markflag (state == TCP_ESTABLISHED) && 
		printf("received tcp packet PSH | ACK\n");
		pthread_mutex_lock(&tsk->rcv_buf->lock);
		int empty = ring_buffer_empty(tsk->rcv_buf);
		write_ring_buffer(tsk->rcv_buf, cb->payload, cb->pl_len);
		tsk->rcv_wnd -= cb->pl_len;
		pthread_mutex_unlock(&tsk->rcv_buf->lock);
		if(empty){
			printf("wake up wait recv\n");
			wake_up(tsk->wait_recv);
		}
		tcp_send_control_packet(tsk, TCP_ACK);
		return;
	}
	tsk->snd_una = cb->ack;
	tsk->rcv_nxt = cb->seq_end;
	// SYN_RCVD -> ESTABLISHED
	if ((state == TCP_SYN_RECV) && (cb->ack == tsk->snd_nxt)){
		printf("7\n");
	  	if (cb->flags & TCP_ACK){
	  		printf("should be change to ESTABLISHED\n");
	  		tcp_set_state(tsk, TCP_ESTABLISHED);
	  		tcp_sock_listen_dequeue(tsk);
			tcp_sock_accept_enqueue(tsk);
			//in tcp_sock.c tcp_sock_accept: set TCP_ESTABLISHED
			printf("wake up wait_accept\n");
			wake_up(tsk->parent->wait_accept);
		}
		else {
			tcp_send_reset(cb);
		}
		return;
	}
	//client
	// FIN_WAIT_1 -> FIN_WAIT_2
	if ((state == TCP_FIN_WAIT_1) && (cb->ack == tsk->snd_nxt)){
		printf("8\n");
		tcp_set_state(tsk, TCP_FIN_WAIT_2);
		// tcp_send_control_packet(tsk, TCP_ACK);//markflag
		return;
	}
	// FIN_WAIT_2 -> TIME_WAIT
	if ((state == TCP_FIN_WAIT_2) && (cb->flags & (TCP_ACK | TCP_FIN))) {
		printf("9\n");
		tcp_set_state(tsk, TCP_TIME_WAIT);
		tcp_set_timewait_timer(tsk);
		tcp_send_control_packet(tsk, TCP_ACK); // markflag |TCP_FIN);
		return;
	}
	//server
	if ((state == TCP_ESTABLISHED) && (cb->flags & TCP_FIN)) {
		printf("10\n");
		tcp_set_state(tsk, TCP_CLOSE_WAIT);
		printf("wake up wait recv\n");
		wake_up(tsk->wait_recv);
		tcp_send_control_packet(tsk, TCP_ACK);
	}
	//markflag
	// if ((state == TCP_CLOSE_WAIT) && (cb->flags | (TCP_ACK|TCP_FIN))){
	// 	tcp_set_state(tsk, TCP_LAST_ACK);
	// 	tcp_send_control_packet(tsk, TCP_ACK |TCP_FIN);
	// }

	//server
	// LAST_ACK -> CLOSED
	if ((state == TCP_LAST_ACK) && (cb->ack == tsk->snd_nxt)){
		printf("11\n");
	  	tcp_set_state(tsk, TCP_CLOSED);
	}


	// 11.
	if ((state == TCP_ESTABLISHED) && (cb->flags == TCP_ACK)){
		printf("received ACK packet.\n");
		printf("wake up wait send\n");
		wake_up(tsk->wait_send);
	}
}