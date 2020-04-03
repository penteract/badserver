//Static files

#define PORT 8080

#include<headers.c>

char errmsg[] = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain; charset=UTF-8\r\nConnection: close\r\nContent-Length: 94\r\n\r\n404 Not Found (or something else for which I can't be bothered to make a proper error message)";

char headers[] = "HTTP/1.1 200 OK\r\nContent-Type: text/XXXX ; charset=UTF-8\r\nConnection: close\r\n\r\nContent-Length:Y     \r\n\r\n";

char* typeinheader;
char* leninheader;

int snd(int sock, char* msg, int len , int z){
    printf("\nsending message to socket %d\n",sock);
    puts(msg);
    int c = send(sock,msg,len,z);
    printf("sent: %d\n",c);
    if(c<0){perror("send failed"); close(sock); return -1;}
    return 0;
}


char pageData[20001];

#define NUMFILES 3

char* files[NUMFILES];
char* fileType[NUMFILES];

char* fileData[NUMFILES];

int fileSize[NUMFILES];

int setup(){
    typeinheader = strchr(headers, 'X');
    leninheader = strchr(typeinheader, 'Y');
    char* filled = pageData;
    FILE * file;
    files[0]="index.html";
    files[1]="styles.css";
    files[2]="traditional.css";
    for(int i=0;i<NUMFILES;i++){
        fileType[i]=strchr( files[i], '.')+1;
        fileData[i]=filled;
        file = fopen(files[i],"r");
        if (file==0){return -2;}
        fileSize[i] = fread(filled,1,20000+pageData-filled ,file);
        if(ferror(file)) return -3;
        if(!feof(file)) return -4;
        filled+=fileSize[i];
        *(filled++)=0;
        fclose(file);
    }
    return 0;
}


int sendFile(int sock, int idx){
    // Adjust headers
    typeinheader[3]=' ';
    memcpy(typeinheader, fileType[idx], strlen(fileType[idx]));
    sprintf(leninheader, "%d\r\n\r\n", fileSize[idx]);

    //Send headers
    int c = snd(sock, headers, strlen(headers), 0);
    if(c<0){perror("send failed"); close(sock); return -1;}
    //send file
    c = snd(sock, fileData[idx], fileSize[idx], 0);
    if(c<0){perror("send failed"); close(sock); return -1;}
    return 0;
}

int sendError(int sock){
    int c = send(sock, errmsg, strlen(errmsg), 0);
    if(c<0){perror("send failed"); close(sock); return -1;}
    return 0;
}

// Binary search on a list of size 3 :)
int bs(char* query){
    int l=0;
    int r=NUMFILES;
    // Invariant dat is in (l..r]
    while(r>l+1){
        int m = (l+r)/2;
        //puts(query);
        //printf("%d,%d\n",m,strcmp(query,files[m]));
        if (strcmp(query,files[m]) >0) l=m;
        else r=m;
    }
    if (memcmp(query,files[l],strlen(files[l]))==0) return l;
    return -1;
}


void process(int sock, char* request){
    bool b = true;
    b = b && *(request++) == 'G';
    b = b && *(request++) == 'E';
    b = b && *(request++) == 'T';
    b = b && *(request++) == ' '; //TODO: check if RFC allows more/other whitespace
    b = b && *(request++) == '/';
    //if(memcmp(request,"duel/",5)==0){request+=5;}
    if(!b){
        sendError(sock);
        return;
    }
    int c = 0; //index of "index.html"
    if (*request != ' ') c=bs(request);
    if (c==-1){
        puts("404:");
        puts(request);
        sendError(sock);
        return;
    }
    sendFile(sock,c);
    return;
}

#include<main.c>
