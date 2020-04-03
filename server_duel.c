// Pair up players who visit this server into 2 Player games.

#define PORT 8081

#include<headers.c>

#include<cards.c>
#include<requests.c>


struct game{
    //sockets TODO: make this an array of players
    int p1;
    int p2;

    card deck[81];
    int dealt; //number of cards dealt
    int out; //number of cards currently in play
};

typedef struct game game;


// How not to make a webserver

//note: if this constant is made much larger, could lead to bad performance if the server is nearly full.
#define MAXGAMES 500

struct game games[MAXGAMES];
int live; // Number of live games
int curr; // current game (awaiting players or possibly empty)
//Invariant: games[i].p1 is nonzero for exactly 'live' indicies i

bool is_started(game* g){
    return g->p1!=0 && g->p2!=0;
}

int new_game(int sock){
    if (live>=MAXGAMES) return -1;
    while(games[curr].p1){curr=(curr+1)%MAXGAMES;}
    games[curr].p1=sock;
    live+=1;
    return curr;
}
int remove_game(int idx){
    if(games[idx].p2 == 0) return 1;
    live-=1;
    games[idx].p1=0;
    games[idx].p2=0;
    return 0;
}

bool add_cards(struct game* g){
    while(true){
        for(int i=0; i < g->out; i++){
            for(int j=i+1; j < g->out; j++){
                for(int k=j+1; k < g->out; k++){
                    if(isTriple(g->deck[i], g->deck[j], g->deck[k])){
                        printf("%d %d %d\n",i,j,k);
                        return false;
                    }
                }
            }
        }
        if(g->dealt>78) return true;
        for(int n=0;n<3;n++){
            g->deck[g->out++]=g->deck[g->dealt++];
        }
    }
}

void init_deck(struct game* g){
    int i=0;
    card cur = 0b01010100;
    while(cur!=0xFF){
        cur+=1;
        cur|=(~(cur|(cur>>1)))&0x55;
        g->deck[i] = cur;
        i++;
    }
    // Reminder: Shuffling is easy to get wrong
    // https://en.wikipedia.org/wiki/Fisher-Yates_shuffle
    for(int i=0;i<81;i++){
        //invariant: cards at [0..i) are random (close to uniformly across all posibilities)
        //Select j close to uniformly from the remaining cards (those at [i..81) )
        int j = i+rand()%(81-i);
        // Swap cards at i and j
        if(i!=j){
            g->deck[j]^=g->deck[i];
            g->deck[i]^=g->deck[j];
            g->deck[j]^=g->deck[i];
        }
    }
    g->dealt=12;
    g->out=12;
    add_cards(g);
}

#define SENDALL(msg) \
 if (snd(g->p1,msg)){\
        snd(g->p2,errscript);\
        remove_game(idx);\
        return -1;\
    }\
    if (snd(g->p2,msg)){\
        snd(g->p1,errscript);\
        remove_game(idx);\
        return -1;\
    }

int start_game(int idx){
    struct game* g = &(games[idx]);
    init_deck(g);
    //Assume that both have been sent the preamble
    //modify startscript
    sprintf(gamenum,"%4d",idx);
    gamenum[4]=' ';
    SENDALL(startscript)
    //modify blankscript
    sprintf(blanknum,"%2d",g->out);
    blanknum[2]=' ';
    SENDALL(blankscript)
    
    for(int i=0;i< g->out;i+=3){
        //modify addscript
        for(int j=0;j<3;j++){
            *(addids[j])= 'a'+i+j;
            toStr(addvals[j], g->deck[i+j]);
        }
        SENDALL(addscript)
    }
    return 0;
}

int send_init(int sock){
    if(snd(sock,headers)) return -1;
    if(snd(sock,initialBody)) return -1;
    if (snd(sock,lorem)){
        return -1;
    }
    if (snd(sock,lorem)){
        return -1;
    }
    return 0;
}

int new_player(int sock){
    // New player trying to start a game
    if (games[curr].p1!=0 && games[curr].p2==0){
        if(send_init(sock)){
            return -3;
        }
        games[curr].p2=sock;
        int old=curr;
        if(start_game(old)){
            //TODO reset game
        }
        curr=(curr+1)%MAXGAMES;
        return old;
    }
    else {
        int idx = new_game(sock);
        if(idx<0) return -1;
        if(send_init(sock)){return -2;}
        return idx;
    }
}

//Errors: 1 - not a game in progress
//        2 - not 3 distinct dealt cards (should never come from a 0-latency well behaved client)
//        3 - not a set
#define SWP(a,b) \
    if ((a)>(b)){\
        a ^= b;\
        b ^= a;\
        a ^= b;\
    }

int play_move(unsigned int idx, int* set){
    printf("game %d, set %d %d %d\n",idx,set[0],set[1],set[2]);
    struct game* g = games+idx;
    if (idx>MAXGAMES || !is_started(g)){
        return 1;
    }
    char s[] = "XXXX XXXX XXXX";
    for(int i=0;i<3;i++) toStr(s+i*5,g->deck[set[i]]);
    puts(s);
    printf("game %d, set %d %d %d\n",idx,g->deck[set[0]],g->deck[set[1]],g->deck[set[2]]);
    //sort set
    SWP(set[0],set[1])
    SWP(set[1],set[2])
    SWP(set[0],set[1])
    if(set[0]==set[1] || set[1]==set[2])
        return 2;
    if( set[2]>= g->out)
        return 2;
    puts("not broken");
    if(!isTriple(g->deck[set[0]],g->deck[set[1]],g->deck[set[2]]))
        return 3;
    puts("Valid set");
    // We now have a good request
    int out = g->out;
    if(out > 12 || g->dealt==81){
        // If set[i] contains one of [out-3 .. out), this will overwrite deck[set[i]]
        // before it matters
        for(int i=2; i>=0; i--){
            g->deck[set[i]] = g->deck[out-1 - i];
        }
        g->out -=3;
        out-=3;
    }
    else{
        for(int i=0;i<3;i++) g->deck[set[i]]=g->deck[ g->dealt++];
    }
    //modify addscript
    for(int j=0;j<3;j++){
        *(addids[j])= 'a'+set[j];
        toStr(addvals[j], g->deck[set[j]]);
    }
    SENDALL(addscript)
    //See if more need to be dealt

    bool finished = add_cards(g);
    printf("out: %d; dealt: %d g1: %X",g->out,g->dealt, g->deck[1]);
    for(int i=out;i< g->out;i+=3){
        //modify addscript
        for(int j=0;j<3;j++){
            *(addids[j])= 'a'+i+j;
            toStr(addvals[j], g->deck[i+j]);
        }
        SENDALL(addscript)
    }
    sprintf(blanknum,"%2d",g->out);
    blanknum[2]=' ';
    SENDALL(blankscript)
    if (finished){
        SENDALL(donescript)
        remove_game(idx);
    }
    printf("done? %d,%d",finished,g->dealt);
    return 0;
}

#define CHECK(cond) \
    if(cond){\
        if (snd(sock, errmsg)) return;\
        close(sock);\
        return;\
    }

void process(int sock, char* request){
    bool b = true;
    b = b && *(request++) == 'G';
    b = b && *(request++) == 'E';
    b = b && *(request++) == 'T';
    b = b && *(request++) == ' '; //TODO: check if RFC allows more/other whitespace
    b = b && *(request++) == '/';
    if(memcmp(request,"duel/",5)==0){request+=5;}
    int c;
    CHECK(!b)
    if (memcmp(request," ",1)==0){
        puts("np");
        new_player(sock);
    }
    else { // Assume it is a move
        puts("mv");
        unsigned int gnum=0;
        while(*request>='0' && *request<='9'){
            gnum = gnum*10 + *request - '0';
            request++;
        }
        CHECK(*(request++) != '/')
        int sel[3];
        for(int i=0; i<3; i++){
            sel[i]=*(request++)-'a';
            CHECK(sel[i]<0 || sel[i]>=21)
        }
        //request is well formed
        int res = play_move(gnum,sel);
        if(res){
            snd(sock, errmsg);
        }
        else {
            snd(sock, okmsg);
        }
        close(sock);
    }
/*    else {
        if (snd(sock,errmsg)) return;
        close(sock);
    }*/
    
    return;
    // send(new_socket , message , strlen(message) , 0);
}


#include<main.c>
