// https://www.binarytides.com/socket-programming-c-linux-tutorial/
#include<stdio.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<string.h>
#include<unistd.h>

int main(int argc , char *argv[])
{
    int socket_desc;
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    struct sockaddr_in server;
    server.sin_addr.s_addr = inet_addr("172.217.169.78");
    server.sin_family = AF_INET;
    server.sin_port = htons( 80 );

    int c = connect(socket_desc , (struct sockaddr *) &server, sizeof(server));
    if (c<0){
        puts("connect error");
        return 1;
    }
    puts("Connected");
    printf("%d\n",c);
    char* message = "GET / HTTP/1.1\r\n\r\n";
    c = send(socket_desc , message , strlen(message) , 0);
    if(c<0) return 1;
    char reply[2001];
    c=recv(socket_desc, reply, 2000, 0);
    while(c>0){
        reply[c]=0;
        puts(reply);
        printf("count: %d",c);
//        puts("\n");
        c=recv(socket_desc, reply, 2000, 0);
    };
    //printf("%d\n",c);
    //puts("reply recieved\n");
    //puts(reply);
    close(socket_desc);
	return 0;
}
