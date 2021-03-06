char errmsg[] = "HTTP/1.1 404 Not Found\r\n"
"Content-Type: text/html; charset=UTF-8\r\n"
"Connection: close\r\nCache-Control: no-cache\r\n"
"Content-Length: 94\r\n"
"\r\n"
"404 Not Found (or something else for which I can't be bothered to make a proper error message)";

char okmsg[] = "HTTP/1.1 204 No Content\r\n"
"Connection: close\r\n"
"Cache-Control: no-cache\r\n"
"Content-Length: 0\r\n\r\n";

char headers[] = "HTTP/1.1 200 OK\r\n"
"Content-Type: text/html; charset=UTF-8\r\n"
"Connection: close\r\n"
"X-Content-Type-Options: nosniff\r\n"
"Cache-Control: no-cache\r\n"
"Transfer-Encoding: Chunked\r\n\r\n";

char initialBody[10008];
//"xx\r\n<html><head><title>SET Version -1 </title></head><body>\r\n";

char startscript[] = "xx\r\n<script>"
"gamenum=XXXX ;"
"name=\"NNNN\".split(\" \")[0];"
"</script>\r\n";
char* gamenum;
char* startname;

char blankscript[] = "xx\r\n<script>"
"blank(NN ) ;"
"</script>\r\n";
char* blanknum;

char scorescript[] = "xx\r\n<script>"
"setScore(PPPP ,\"NNNN\",MMMM );"
"</script>\r\n";
char* scorepl;
char* scorename;
char* scoreval;

char addscript[] = "xx\r\n<script>"
"setpic(\"X\", \"1111\"); "
"setpic(\"Y\", \"2222\"); "
"setpic(\"Z\", \"3333\"); "
"</script>\r\n";
char* addids[3];
char* addvals[3];

char errscript[] = "xx\r\n<script>"
"error();"
"</script>\r\n";
char donescript[] = "xx\r\n<script>"
"done();"
"</script>\r\n"
"\0\r\n\r\n";

/*char lorem[] = "xx\r\n<script>"
"document.getElementById(\"p1\").classList.add(\"hide\");"
"document.getElementById(\"p2\").classList.add(\"hide\");"
"</script>\r\n\r\n";

  "xx\r\n lorem ipsum dolor sit amet<br />\r\n"
"I can't remember the rest, but it doesn't matter too much<br />\r\n"
"Sending data to test if anything shows<br />\r\n"
"Lets add some more characters to make it go faster<br />\r\n\r\n";*/


#ifdef GAME
char composedscript[0x1000];
char* composeptr;

void startcompose(){
    strcpy(composedscript,"xxx\r\n");
    composeptr=composedscript+5;
}

int nats[21]= {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20};
void addadd(card* deck, int* set){
    for(int j=0;j<3;j++){
        *(addids[j])= 'a'+set[j];
        toStr(addvals[j], deck[set[j]]);
    }
    int len = strlen(addscript)-6;
    memcpy(composeptr, addscript+4, len);
    composeptr+=len;
}
void addblank(int n){
    sprintf(blanknum,"%2d", n);
    blanknum[2]=' ';
    int len = strlen(blankscript)-6;
    memcpy(composeptr, blankscript+4, len);
    composeptr+=len;
}
void setscore(game* g, int p){
    sprintf(scorepl,"%4d",p);
    scorepl[4]=' ';
    memcpy(scorename,g->names[p],4);
    sprintf(scoreval,"%4d",g->scores[p]);
    scoreval[4]=' ';
}
void addscore(game* g, int p){
    setscore(g,p);
    int len = strlen(scorescript)-6;
    memcpy(composeptr, scorescript+4, len);
    composeptr+=len;
}
void endcompose(){
    sprintf(composedscript,"%03x", (composeptr-composedscript)-5);
    composedscript[3]='\r';
    sprintf(composeptr,"\r\n");
}
#endif
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
    puts(str);
    return 0;
}
int setup(){
    redirectdest = strchr(redirect, 'X');
    blanknum = strchr(blankscript, 'N');
    scorepl = strchr(scorescript, 'P');
    scorename = strchr(scorescript, 'N');
    scoreval = strchr(scorescript, 'M');
    gamenum = strchr(startscript, 'X');
    startname = strchr(startscript, 'N');
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
    donescript[strlen(donescript)]='0';
    x = x || mkChunk(scorescript);
    //x = x || mkChunk(lorem);
    if (x) return x;
    FILE * file = fopen("init.html","r");
    if (file==0){return -2;}
    int k = fread(initialBody+6,1,10000,file);
    if(ferror(file)) return -3;
    if(!feof(file)) return -4;
    sprintf(initialBody,"%04x",k);
    initialBody[4]='\r';
    initialBody[5]='\n';
    initialBody[6+k]='\r';
    initialBody[7+k]='\n';
    fclose(file);
    return 0;
}


