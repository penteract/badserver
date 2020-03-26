// https://www.binarytides.com/socket-programming-c-linux-tutorial/
#include<stdio.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<string.h>
#include<unistd.h>

int main(int argc , char *argv[])
{
    int socket_desc, new_socket, k;
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    //https://stackoverflow.com/questions/24194961/how-do-i-use-setsockoptso-reuseaddr
    int enable = 1;
    setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    struct sockaddr_in server, client;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_family = AF_INET;
    server.sin_port = htons( 8080 );
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
    
    char* message;




    char reply[2001];
    while (new_socket>0){
        puts("Accepted");
        message = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf8\r\nX-Content-Type-Options: nosniff\r\nTransfer-Encoding: Chunked\r\n\r\n2A\r\n<HTML><title>ABC</title><body><p>hello</p>\r\n";
        c = recv(new_socket, reply, 2000, 0);
        if(c>0){
            reply[c]=0;
            printf("count: %d\n",c);
            puts(reply);
            c = send(new_socket , message , strlen(message) , 0);
            if(c<0) return 1;
            sleep(10);
            message = "15\r\ngoodbye</body></HTML>\r\n0\r\n";
            c = send(new_socket , message , strlen(message) , 0);
            if(c<0) return 1;
        } else perror("recv failed");
        close(new_socket);
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
