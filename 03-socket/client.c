/* client application */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

typedef struct transport
{
    int size;
    char path[100];
    int start;
    int end;
};
 
int main(int argc, char *argv[])
{
    int sock_set[3];
    struct sockaddr_in server[3];

    char message[1000], server_reply[2000];
    int total_letter[26];
    memset(total_letter, 0, 104);
     
    // create socket
    for(int i = 0; i < 3; i++){
        sock_set[i] = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_set[i] == -1) {
            printf("create socket%d failed", i);
		      return -1;
        }
        printf("Socket%d created   ", i);
    }
    

    FILE *fp_conf = fopen("workers.conf", "rb");
    if (!fp_conf){
        printf("can't open file :worders.conf\n");
        return -1;
    }

    for(int i = 0; i < 3; i++){
        char *ip = (char *)malloc(10);
        fscanf(fp_conf, "%s", ip);
        printf("%s\n", ip);
        server[i].sin_addr.s_addr = inet_addr(ip);
        server[i].sin_family = AF_INET;
        server[i].sin_port = htons(8888);
 
        // connect to server
        if (connect(sock_set[i], (struct sockaddr *)&(server[i]), sizeof(server[i])) < 0) {
            perror("connect to server failed");
            free(ip);
            return 1;
        }
        printf("server%d connected\n", i);
        free(ip);
    }
     
    // while(1) {
        printf("transport message \n");
        

        //file path size
        int message_size = strlen(argv[1]);
        int net_msize = htonl(message_size);
        
        FILE *fp = fopen(argv[1], "rb");

        fseek(fp, 0, SEEK_END);
        int total_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        for(int i = 0; i < 3; i++){
            struct transport send_message;

            //start & end position        
            int start = htonl(total_size / 3 * i);
            int end = htonl(total_size / 3 * (i + 1));

            send_message.size = net_msize;
            memcpy(send_message.path, argv[1], message_size);
            send_message.start = start;
            send_message.end = end;

            memset(message, 0, 1000);
            memcpy(message, &send_message, 112);
       

            // send some data
            if (send(sock_set[i], message, 112, 0) < 0) {//message
                printf("send failed");
                return 1;
            }
            // printf("%s\n", a); 

            memset(server_reply, 0, 2000);

            // receive a reply from the server
            if (recv(sock_set[i], server_reply, 2000, 0) < 0) {
                printf("fail:  %d\n", i);
                printf("recv failed");
                // break;
            }
            int letter[26];
            memcpy((char *)letter, server_reply, 104);

            for(int i = 0; i < 26; i++){
                letter[i] = ntohl(letter[i]);
                total_letter[i] += letter[i];
            }
            close(sock_set[i]);
        
        }


        printf("server reply : ");
        for(int i = 0; i < 26; i++){
            printf("%c: %d\n", 'a' + i, total_letter[i]);
        }
        
    // }
     
    return 0;
}
