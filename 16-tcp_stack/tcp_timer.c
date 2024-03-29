#include "tcp.h"
#include "tcp_timer.h"
#include "tcp_sock.h"

#include <unistd.h>

static struct list_head timer_list;

//markflag
// scan the timer_list, find the tcp sock which stays for at 2*MSL, release it
void tcp_scan_timer_list()
{
	struct tcp_sock *tsk;
	struct tcp_timer *t, *q;
	list_for_each_entry_safe(t, q, &timer_list, list) {
		t->timeout -= TCP_TIMER_SCAN_INTERVAL;
		if (t->timeout <= 0 && t->type == 0) {
			list_delete_entry(&t->list);

			// only support time wait now
			tsk = timewait_to_tcp_sock(t);
			if (! tsk->parent)
				tcp_bind_unhash(tsk);
			tcp_set_state(tsk, TCP_CLOSED);
			free_tcp_sock(tsk);
		} else if (t->timeout <= 0 && t->type == 1){
			tsk = retranstimer_to_tcp_sock(t);
			if(t->retrans_num == 3){
				//retrans >= 3, close this socket
				tcp_sock_close(tsk);
				return;
			}

			//resend the first data / SYN|FIN packet
			//t is the corresponding retrans_timer 
			if(!list_empty(&tsk->snd_buffer)){
				struct snd_buffer_node *temp = list_entry(tsk->snd_buffer.next, struct snd_buffer_node, list);
				char *packet_resend = malloc(sizeof(temp->packet));
				memcpy(packet_resend, temp->packet, temp->len);

				ip_send_packet(packet_resend, temp->len);
				t->timeout = TCP_RETRANS_INTERVAL;
				for(int i = 0; i < t->retrans_num; i++){
					t->timerout *= 2;
				}
				t->retrans_num ++;
			}
		}
	}
}

// set the timewait timer of a tcp sock, by adding the timer into timer_list
void tcp_set_timewait_timer(struct tcp_sock *tsk)
{
	struct tcp_timer *timer = &tsk->timewait;

	timer->type = 0;
	timer->timeout = TCP_TIMEWAIT_TIMEOUT;
	list_add_tail(&timer->list, &timer_list);

	tcp_sock_inc_ref_cnt(tsk);
}

//markflag 
//set retrans timer
void tcp_set_retrans_timer(struct tcp_sock *tsk){
	struct tcp_timer *temp = &tsk->retrans_timer;
	timer->type = 1;
	timer->timerout = TCP_RETRANS_INTERVAL;
	list_add_tail(&temp->list, &timer_list);
}

//remove retrans_timer from timer_list
void tcp_remove_retrans_timer(struct tcp_sock *tsk){
	list_delete_entry(&(tsk->retrans_timer.list));
}

// scan the timer_list periodically by calling tcp_scan_timer_list
void *tcp_timer_thread(void *arg)
{
	init_list_head(&timer_list);
	while (1) {
		usleep(TCP_TIMER_SCAN_INTERVAL);
		tcp_scan_timer_list();
	}

	return NULL;
}
