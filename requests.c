char errmsg[] = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html; charset=utf18\r\nConnection: close\r\nCache-Control: no-store\r\nContent-Length: 94\r\n\r\n404 Not Found (or something else for which I can't be bothered to make a proper error message)";

char okmsg[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf18\r\nConnection: close\r\nCache-Control: no-store\r\nContent-Length: 0\r\n\r\n";

char headers[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf8\r\nConnection: close\r\nX-Content-Type-Options: nosniff\r\nCache-Control: no-store\r\nTransfer-Encoding: Chunked\r\n\r\n";
char initialBody[5008];
//"xx\r\n<html><head><title>SET Version -1 </title></head><body>\r\n";
char startscript[] = "xx\r\n<script>"
"gamenum=NNNN ;"
"</script>\r\n";
char* gamenum;

char blankscript[] = "xx\r\n<script>"
"blank(NN ) ;"
"</script>\r\n";
char* blanknum;


char errscript[] = "xx\r\n<script>"
"error();"
"</script>\r\n";
char donescript[] = "xx\r\n<script>"
"done();"
"</script>\r\n";
char addscript[] = "xx\r\n<script>"
"setpic(\"X\", \"1111\"); "
"setpic(\"Y\", \"2222\"); "
"setpic(\"Z\", \"3333\"); "
"</script>\r\n";

char composedscript[0x1000];

char* addids[3];
char* addvals[3];

// char msg5[] = "05\r\nxxxxx\r\n";

int snd(int sock, char* msg){
    printf("\nsending message to socket %d\n",sock);
    puts(msg);
    int c = send(sock,msg,strlen(msg),0);
    printf("sent: %d\n",c);
    if(c<0){perror("send failed"); close(sock); return -1;}
    return 0;
}

char tohexdig(int n){
  if(n<10) return 0x30+n;
  else return 0x61-10+n;
}
int mkChunk(char* str){
    int l = strlen(str) - 6; // xx/r/nbody/r/n
    if (l>=256 || l<0){
        return -1;
    }
    str[0] = tohexdig(l/16);
    str[1] = tohexdig(l%16);
    //puts(str);
    return 0;
}
int setup(){
    blanknum = strchr(blankscript, 'N');
    gamenum = strchr(startscript, 'N');
    addids[0] = strchr(addscript, 'X');
    addids[1] = strchr(addscript, 'Y');
    addids[2] = strchr(addscript, 'Z');
    addvals[0] = strchr(addscript, '1');
    addvals[1] = strchr(addscript, '2');
    addvals[2] = strchr(addscript, '3');
    
    int x = mkChunk(startscript);
    x = x || mkChunk(errscript);
    x = x || mkChunk(blankscript);
    x = x || mkChunk(addscript);
    x = x || mkChunk(donescript);
    if (x) return x;
    FILE * file = fopen("init.html","r");
    if (file==0){return -2;}
    int k = fread(initialBody+6,1,5000,file);
    if(ferror(file)) return -3;
    if(!feof(file)) return -4;
    sprintf(initialBody,"%04x",k);
    initialBody[4]='\r';
    initialBody[5]='\n';
    initialBody[6+k]='\r';
    initialBody[7+k]='\n';
    return 0;
}
