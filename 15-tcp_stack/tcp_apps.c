#include "tcp_sock.h"

#include "log.h"

#include <unistd.h>
#include <pthread.h>

// tcp server application, listens to port (specified by arg) and serves only one
// connection request
void *tcp_server(void *arg)
{
	u16 port = *(u16 *)arg;
	struct tcp_sock *tsk = alloc_tcp_sock();

	struct sock_addr addr;
	addr.ip = htonl(0);
	addr.port = port;
	if (tcp_sock_bind(tsk, &addr) < 0) {
		log(ERROR, "tcp_sock bind to port %hu failed", ntohs(port));
		exit(1);
	}

	if (tcp_sock_listen(tsk, 3) < 0) {
		log(ERROR, "tcp_sock listen failed");
		exit(1);
	}

	log(DEBUG, "listen to port %hu.", ntohs(port));

	struct tcp_sock *csk = tcp_sock_accept(tsk);

	log(DEBUG, "accept a connection.");

	char rbuf[1001];
	char wbuf[1024];
	int rlen = 0;
	while (1) {
		rlen = tcp_sock_read(csk, rbuf, 1000);
		printf("rlen:%d\n", rlen);
		if (rlen < 0) {
			log(DEBUG, "tcp_sock_read return negative value, finish transmission.");
			break;
		} 
		else if (rlen > 0) {
			printf("tcp_sock read %d bytes of data.\n", rlen);
			rbuf[rlen] = '\0';
			sprintf(wbuf, "server echoes: %s", rbuf);
			printf("\n");
			if (tcp_sock_write(csk, wbuf, strlen(wbuf)) < 0) {
				log(DEBUG, "tcp_sock_write return negative value, finish transmission.");
				break;
			}
		}
	}

	log(DEBUG, "close this connection.");

	tcp_sock_close(csk);
	
	return NULL;
}

// tcp client application, connects to server (ip:port specified by arg), each
// time sends one bulk of data and receives one bulk of data 
void *tcp_client(void *arg)
{
	struct sock_addr *skaddr = arg;

	struct tcp_sock *tsk = alloc_tcp_sock();

	if (tcp_sock_connect(tsk, skaddr) < 0) {
		log(ERROR, "tcp_sock connect to server ("IP_FMT":%hu)failed.", \
				NET_IP_FMT_STR(skaddr->ip), ntohs(skaddr->port));
		exit(1);
	}

	char *wbuf = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int wlen = strlen(wbuf);
	char rbuf[1001];
	int rlen = 0;

	int n = 10;
	for (int i = 0; i < n; i++) {
		if (tcp_sock_write(tsk, wbuf + i, wlen - n) < 0){
			break;

		}

		rlen = tcp_sock_read(tsk, rbuf, 1000);
		if (rlen < 0) {
			log(DEBUG, "tcp_sock_read return negative value, finish transmission.");
			break;
		}
		else if (rlen > 0) {
			printf("tcp_sock read %d bytes of data\n", rlen);
			rbuf[rlen] = '\0';
			fprintf(stdout, "%s\n", rbuf);
		}
		else {
			fprintf(stdout, "*** read data == 0.\n");
		}
		sleep(1);
	}

	tcp_sock_close(tsk);

	return NULL;
}


int tcp_sock_read(struct tcp_sock *tsk, char *buf, int len){
	printf("in read\n");
	pthread_mutex_lock(&tsk->rcv_buf->lock);

	if(ring_buffer_empty(tsk->rcv_buf)){
		pthread_mutex_unlock(&tsk->rcv_buf->lock);
		sleep_on(tsk->wait_recv);
		pthread_mutex_lock(&tsk->rcv_buf->lock);
	}

	int read = read_ring_buffer(tsk->rcv_buf, buf, len);


	pthread_mutex_unlock(&tsk->rcv_buf->lock);
	printf("out read\n");
	return read;
}


int tcp_sock_write(struct tcp_sock *tsk, char *buf, int len){
	int data_size = min(len, 1500 - ETHER_HDR_SIZE - IP_BASE_HDR_SIZE - TCP_BASE_HDR_SIZE);
	int total_size =  data_size + ETHER_HDR_SIZE + IP_BASE_HDR_SIZE + TCP_BASE_HDR_SIZE;

	char *packet = malloc(total_size);
	memset(packet, 0, total_size);

	memcpy((char *)packet + ETHER_HDR_SIZE + IP_BASE_HDR_SIZE + TCP_BASE_HDR_SIZE, buf, data_size);

	tcp_send_packet(tsk, packet, total_size);

	sleep_on(tsk->wait_send);
	return data_size;

}