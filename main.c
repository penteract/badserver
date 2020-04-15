// https://www.binarytides.com/socket-programming-c-linux-tutorial/

int main(int argc , char *argv[])
{
    #ifdef SETUP 
    SETUP
    #else
    if (setup()){
        puts("Setup failed");
        return 1;
    }
    #endif
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
    server.sin_port = htons( PORT );
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
    

    char request[101];
    while (new_socket>0){
        // puts("Accepted");
        c = recv(new_socket, request, 100, 0);
        if(c>0){
            request[c]=0; //for parser safety, not just for printing convenience
            printf("count: %d\n",c);
            puts(request);
            process(new_socket,request);
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


