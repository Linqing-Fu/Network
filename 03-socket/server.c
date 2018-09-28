/* server application */
 
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
 
int main(int argc, const char *argv[])
{
    int s, cs;
    struct sockaddr_in server, client;
    char msg[2000];
    // create socket
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("create socket failed");
		return -1;
    }
    printf("socket created");
     
    // prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(8888);
     
    // bind
    if (bind(s,(struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("bind failed");
        return -1;
    }
    printf("bind done");
     
    // listen
    listen(s, 12345);
    printf("waiting for incoming connections...");
     
    // accept connection from an incoming client
    int c = sizeof(struct sockaddr_in);
    if ((cs = accept(s, (struct sockaddr *)&client, (socklen_t *)&c)) < 0) {
        perror("accept failed");
        return -1;
    }
    printf("connection accepted\n");
     
	int msg_len = 0;
    
    // receive a message from client
    while ((msg_len = recv(cs, msg, sizeof(msg), 0)) > 0) { //sizeof(msg)
 
        struct transport
        {
            int size;
            char path[100];
            int start;
            int end;
        }recv_message;

        memcpy(&recv_message, msg, msg_len);
        int size = ntohl(recv_message.size);
        // printf("size%d\n", size);
        char *file_name = recv_message.path;
        int start = ntohl(recv_message.start);
        int end = ntohl(recv_message.end);

        //open file

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
        memcpy(msg, (char *)net, 104);

        

        // send the message back to client
        write(cs, msg, 104);
    }
     
    if (msg_len == 0) {
        printf("client disconnected");
    }
    else { // msg_len < 0
        perror("recv failed");
		return -1;
    }
     
    return 0;
}
