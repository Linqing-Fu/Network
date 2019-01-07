#include "tcp_sock.h"

#include "log.h"

#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <netinet/in.h>

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

	struct transport
    {
        int size;
        char path[100];
        int start;
        int end;
    }recv_message;

	while((rlen = tcp_sock_read(csk, rbuf, 1000)) >= 0){
		printf("tcp_sock read %d bytes of data.\n", rlen);
		
		
		memcpy(&recv_message, rbuf, rlen);
		int size = ntohl(recv_message.size);
		char *file_name = recv_message.path;
		int start = ntohl(recv_message.start);
		int end = ntohl(recv_message.end);

		// rbuf[rlen] = '\0';

		printf("start%d end%d\n", start, end);

        FILE *fp = fopen(file_name, "rb");
        int letter[26];
        memset(letter, 0, 104);
        char buf[end - start];

        fseek(fp, start, SEEK_SET);
        fread(buf, 1, end - start, fp);

        for(int i = 0; i < end - start; i++){
            if(buf[i] >= 'A' && buf[i] <= 'Z'){
                letter[buf[i] - 'A']++;
            } else if (buf[i] >= 'a' && buf[i] <= 'z'){
                letter[buf[i] - 'a']++;
            }
        }
        for(int i = 0; i < 26; i++){
            printf("%c: %d\n", 'a' + i, letter[i]);
        }
        int net[26];
        for(int i = 0; i < 26; i++){
            net[i] = htonl(letter[i]);
        }

        memcpy(wbuf, (char*)net, 104);

        if(tcp_sock_write(csk, wbuf, 104) < 0){
        	log(DEBUG, "tcp_sock_write return negative value, finish transmission.");
			break;
        }

	}

	log(DEBUG, "close this connection.");

	tcp_sock_close(csk);

	// while (1) {
	// 	rlen = tcp_sock_read(csk, rbuf, 1000);
	// 	printf("rlen:%d\n", rlen);
	// 	if (rlen < 0) {
	// 		log(DEBUG, "tcp_sock_read return negative value, finish transmission.");
	// 		break;
	// 	} 
	// 	else if (rlen > 0) {
			
	// 		sprintf(wbuf, "server echoes: %s", rbuf);
	// 		printf("\n");
	// 		if (tcp_sock_write(csk, wbuf, strlen(wbuf)) < 0) {
	// 			log(DEBUG, "tcp_sock_write return negative value, finish transmission.");
	// 			break;
	// 		}
	// 	}
	// }

	// log(DEBUG, "close this connection.");

	// tcp_sock_close(csk);
	
	return NULL;
}

typedef struct transport
	{
    	int size;
    	char path[100];
    	int start;
    	int end;
	};

// tcp client application, connects to server (ip:port specified by arg), each
// time sends one bulk of data and receives one bulk of data 
void *tcp_client(void *arg)
{

	

	char message[1000], server_reply[2000];
    int total_letter[26];
    memset(total_letter, 0, 104);


	struct tcp_sock *tsk[2];
	tsk[0] = alloc_tcp_sock();
	tsk[1] = alloc_tcp_sock();
	// tsk[2] = alloc_tcp_sock();

	FILE *fp_conf = fopen("workers.conf", "rb");
    if (!fp_conf){
        printf("can't open file :worders.conf\n");
        return NULL;
    }

	for(int i = 0; i < 2; i ++){
		char *ip = (char *)malloc(10);
		fscanf(fp_conf, "%s", ip);
        printf("%s\n", ip);

		// struct sock_addr *skaddr = arg;
		// printf("bbbbbbbbbbbbbbbbbbb\n");
		struct sock_addr skaddr;
		skaddr.ip = inet_addr(ip);
		printf("skaddr ip:%u\n", skaddr.ip);
		skaddr.port = htons(10001);
		// printf("aaaaaaaaaaaaaaaaaaaaaaaaaaa\n");
		// printf("%u\n", &skaddr);
		if(tcp_sock_connect(tsk[i], &skaddr) < 0){
			log(ERROR, "tcp_sock connect to server ("IP_FMT":%hu)failed.", \
				NET_IP_FMT_STR(skaddr.ip), ntohs(skaddr.port));
			exit(1);	
		}

		printf("server %d connected\n", i);
		free(ip);

	}

	printf("transport message \n");
        

    //file path size
    int message_size = strlen(arg);
    int net_msize = htonl(message_size);
      
    FILE *fp = fopen(arg, "rb");

    fseek(fp, 0, SEEK_END);
    int total_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    for(int i = 0; i < 2; i++){
    	struct transport send_message;

        //start & end position        
        int start = htonl(total_size / 2 * i );
        int end = htonl(total_size / 2 * (i + 1));

        send_message.size = net_msize;
        memcpy(send_message.path, arg, message_size);
        send_message.start = start;
        send_message.end = end;

        memset(message, 0, 1000);
        memcpy(message, &send_message, 112);

        if(tcp_sock_write(tsk[i], message, 112) < 0){
        	break;
        }

        char rbuf[1001];
        int rlen = 0;
        memset(rbuf, 0, 1001);

        int letter[26];

        rlen = tcp_sock_read(tsk[i], rbuf, 1000);
        memcpy((char*)letter, rbuf, 104);
        
        if(rlen < 0){
        	log(DEBUG, "tcp_sock_read return negative value, finish transmission.");
			break;
        }

        for(int j = 0; j < 26; j++){
        	letter[j] = ntohl(letter[j]);
        	total_letter[j] += letter[j];
        }

        tcp_sock_close(tsk[i]);

    }

    printf("server reply : \n");
    
    for(int i = 0; i < 26; i++){
	    printf("%c: %d\n", 'a' + i, total_letter[i]);
    }

    return 0;

	// if (tcp_sock_connect(tsk, skaddr) < 0) {
	// 	log(ERROR, "tcp_sock connect to server ("IP_FMT":%hu)failed.", 
	// 			NET_IP_FMT_STR(skaddr->ip), ntohs(skaddr->port));
	// 	exit(1);
	// }

	// char *wbuf = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	// int wlen = strlen(wbuf);
	// char rbuf[1001];
	// int rlen = 0;

	// int n = 10;
	// for (int i = 0; i < n; i++) {
	// 	//sent to server
	// 	if (tcp_sock_write(tsk, wbuf + i, wlen - n) < 0){
	// 		break;

	// 	}

	// 	rlen = tcp_sock_read(tsk, rbuf, 1000);
	// 	if (rlen < 0) {
	// 		log(DEBUG, "tcp_sock_read return negative value, finish transmission.");
	// 		break;
	// 	}
	// 	else if (rlen > 0) {
	// 		printf("tcp_sock read %d bytes of data\n", rlen);
	// 		rbuf[rlen] = '\0';
	// 		fprintf(stdout, "%s\n", rbuf);
	// 	}
	// 	else {
	// 		fprintf(stdout, "*** read data == 0.\n");
	// 	}
	// 	sleep(1);
	// }

	// tcp_sock_close(tsk);

	// return NULL;
}


int tcp_sock_read(struct tcp_sock *tsk, char *buf, int len){
	printf("in read\n");
	// pthread_mutex_lock(&tsk->rcv_buf->lock);
	// if(ring_buffer_empty(tsk->rcv_buf)){
	// 	pthread_mutex_unlock(&tsk->rcv_buf->lock);
	// 	sleep_on(tsk->wait_recv);
	// 	printf("sleep on wait recv\n");
	// 	pthread_mutex_lock(&tsk->rcv_buf->lock);
	// }
	// int read = read_ring_buffer(tsk->rcv_buf, buf, len);


	// pthread_mutex_unlock(&tsk->rcv_buf->lock);
	printf("head:%d tail:%d\n", tsk->rcv_buf->head, tsk->rcv_buf->tail);
	if(ring_buffer_empty(tsk->rcv_buf)) {
		printf("emptyyyyyyyyyyyyyyyyyyyyyy\n");
		if(tsk->state == TCP_CLOSE_WAIT){
    		return -1;
    	}
		printf("sleep on wait recv\n");
		sleep_on(tsk->wait_recv);
	}

	pthread_mutex_lock(&tsk->rcv_buf->lock);
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
	printf("sleep on wait send\n");
	sleep_on(tsk->wait_send);
	return data_size;

}