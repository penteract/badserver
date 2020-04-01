// https://www.binarytides.com/socket-programming-c-linux-tutorial/
// https://stackoverflow.com/a/16154717/1779797
#include <netinet/in.h>

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<string.h>
#include<unistd.h>
#include <stdbool.h>
#include<signal.h>

char headers[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf8\r\nConnection: close\r\nX-Content-Type-Options: nosniff\r\nCache-Control: no-store\r\nTransfer-Encoding: Chunked\r\n\r\n";

char init[] = "xx\r\n<html><head><title>Tell me if you see the title</title></head>"
"<body>\r\n";
char lorem[] = "xx\r\nlorem ipsum dolor sit amet<br />\r\n"
"I can't remember the rest, but it doesn't matter too much<br />\r\n"
"Sending data to test if anything shows<br />\r\n"
"Lets add some more characters to make it go faster<br />\r\n\r\n";
char tohexdig(int n){
  if(n<10) return 0x30+n;
  else return 0x61-10+n;
}
int make_chunk(char* str){
    int l = strlen(str) - 6; // xx/r/nbody/r/n
    if (l>=256 || l<0){
        return -1;
    }
    str[0] = tohexdig(l/16);
    str[1] = tohexdig(l%16);
    puts(str);
    return 0;
}
int snd(int sock, char* msg){
    printf("\nsending message to socket %d\n",sock);
    puts(msg);
    int c = send(sock,msg,strlen(msg),0);
    printf("sent: %d\n",c);
    if(c<0){perror("send failed"); close(sock); return -1;}
    return 0;
}

int main(int argc , char *argv[])
{
    if (make_chunk(init) ||  make_chunk(lorem)){
        puts("Setup failed");
        return 1;
    }
    int socket_desc, new_socket, k;
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    //https://stackoverflow.com/questions/24194961/how-do-i-use-setsockoptso-reuseaddr
    //TODO: remove when done debugging
    int enable = 1;
    signal(SIGPIPE, SIG_IGN);
    setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    struct sockaddr_in server, client;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_family = AF_INET;
    server.sin_port = htons( 8081 );
    int c = bind(socket_desc , (struct sockaddr *) &server, sizeof(server));
    if (c<0){
        perror("bind failed");
        return 1;
    }
    puts("Bound");
    printf("%d\n",c);
    listen(socket_desc , 3);
    puts("Waiting for incoming conns");
    c = sizeof(struct sockaddr_in);
    new_socket = accept(socket_desc, (struct sockaddr*) &client, (socklen_t*) &k);
/*    if (new_socket<0){
    }
    puts("Accepted");*/
    

    char reply[101];
    while (new_socket>0){
        puts("Accepted");
        c = recv(new_socket, reply, 100, 0);
        if(c>0){
            reply[c]=0; //for parser safety, not just for printing convenience
            printf("count: %d\n",c);
            puts(reply);
            if(snd(new_socket, headers)==0)
              if(snd(new_socket, init)==0)
                while(snd(new_socket,lorem)==0){sleep(1);}
            puts("done\n");
        } else {
            perror("recv failed");
            close(new_socket);
        }
        new_socket = accept(socket_desc, (struct sockaddr*) &client, (socklen_t*) &k);
    };
    perror("accept failed");
    //  return 1;
    //printf("%d\n",c);
    //puts("reply recieved\n");
    //puts(reply);
    //close(new_socket);
    close(socket_desc);
	return 0;
}


