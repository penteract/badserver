// string a starts with the string b
// memcmp(a,b,strlen(b))==0
bool startswith(char* a, char* b){
    while(*a==*b){
        if(*(b++)==0) return true;
        a++;
    }
    return (*b)==0;
}

char redirect[] = "HTTP/1.1 308 Permanent Redirect\r\n"
"Content-Length: 0\r\n"
"Connection: close\r\n"
"Location: XXXXX XXXX/\r\n\r\n";
char* redirectdest;


// Send a redirect message to sock
int sendRedirect(int sock, char* path){
    strncpy(redirectdest,path,15);
    int c = send(sock,redirect,strlen(redirect),0);
    if(c<0){perror("send failed"); close(sock); return -1;}
    close(sock);
    return 0;
}
