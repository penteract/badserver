// Pair up players who visit this server into 2 Player games.

#define PORT 8081

#include<headers.c>

#define REDIRECTTYPE "307 Temporary Redirect"
#include<utils.c>
#include<cards.c>

struct game{
    //sockets
    int players[2];
    //Game states
    // 0 0 Empty/finished
    // x 0 Waiting for second player
    // x y Started

    int scores[2];
    char names[2][NAMELENGTH];

    card deck[81];
    int dealt; //number of cards dealt
    int out; //number of cards currently in play
};
typedef struct game game;


#include<requests.c>


// How not to make a webserver

//note: if this constant is made much larger, could lead to bad performance if the server is nearly full.
#define MAXGAMES 500

struct game games[MAXGAMES];
int live; // Number of live games
int curr; // current game (awaiting players or possibly empty)
//Invariant: games[i].p1 is nonzero for exactly 'live' indicies i

bool is_started(game* g){
    return g->players[0]!=0 && g->players[1]!=0;
}
bool is_empty(game* g){
    return g->players[0]==0;
}
bool is_waiting(game* g){
    return g->players[0]!=0 && g->players[1]==0;
}

void add_player(game* g, int p, int sock, char* name){
    g->players[p]=sock;
    g->scores[p]=0;
    memcpy(g->names[p],name,NAMELENGTH);
}

int new_game(int sock){
    //TODO: sometimes check for closed connections
    if (live>=MAXGAMES) return -1;
    while(!is_empty(&games[curr])){curr=(curr+1)%MAXGAMES;}
    live+=1;
    return curr;
}
int remove_game(int idx){
    if(is_empty(&games[idx])) return 1;
    live-=1;
    for(int p=0;p<2;p++){
      if(games[idx].players[p]){
          close(games[idx].players[p]);
          games[idx].players[p]=0;
      }
    }
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
 if (snd(g->players[0],msg)){\
        snd(g->players[1],errscript);\
        remove_game(idx);\
        return -1;\
    }\
    if (snd(g->players[1],msg)){\
        snd(g->players[0],errscript);\
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
    for(int p=0;p<2;p++){
        memcpy(startname,g->names[p],NAMELENGTH);
        if (snd(g->players[p],startscript)){
            snd(g->players[1-p],errscript);
            remove_game(idx);
            return -1;
        }
    }
    startcompose();
    addblank(g->out);
    for(int p=0;p<2;p++){
        addscore(g,p);
    }
    for(int i=0;i< g->out;i+=3){
        addadd(g->deck, nats+i);
    }
    endcompose();
    SENDALL(composedscript)
    return 0;
}

int send_init(int sock){
    if(snd(sock,headers)) return -1;
    if(snd(sock,initialBody)) return -1;
    return 0;
}

int new_player(int sock, char* name){
    // New player trying to start a game
    if (is_waiting(&games[curr])){
        if(send_init(sock)){
            return -3;
        }
        add_player(&games[curr],1,sock,name);
        int old=curr;
        if(start_game(old)){
            //TODO reset game
        }
        curr=(curr+1)%MAXGAMES;
        return old;
    }
    else {
        int idx = new_game(sock);
        add_player(&games[idx],0,sock,name);
        if(idx<0) return -1;
        if(send_init(sock)){return -2;}
        return idx;
    }
}

#define SWP(a,b) \
    if ((a)>(b)){\
        a ^= b;\
        b ^= a;\
        a ^= b;\
    }

int find_player(game* g , char* name){
    for(int p=0;p<2;p++){
        if (memcmp(name,g->names[p],NAMELENGTH)==0) return p;
    }
    return -1;
}

//Errors: 1 - not a game in progress
//        2 - not 3 distinct dealt cards (should never come from a 0-latency well behaved client)
//        3 - not a set
//        4 - not a known player
//       -1 - connection failure
int play_move(unsigned int idx, char* pname, card* set){
    printf("game %d, set %d %d %d\n",idx,set[0],set[1],set[2]);
    struct game* g = games+idx;
    int p = find_player(g,pname);
    if(p<0) return 4;
    if (idx>MAXGAMES || !is_started(g)){
        return 1;
    }
    // Turn cards into indicies
    int idxs[3];
    for(int i=0;i<3;i++){
        card* c = memchr(g->deck,set[i],g->out);
        if (c==0) return 4;
        idxs[i]=c-g->deck;
    }
    //sort indicies
    SWP(idxs[0],idxs[1])
    SWP(idxs[1],idxs[2])
    SWP(idxs[0],idxs[1])
    if(idxs[0]==idxs[1] || idxs[1]==idxs[2])
        return 2;
    if( idxs[2]>= g->out)
        return 2;
    puts("not broken");
    if(!isTriple(set[0],set[1],set[2])){
        g->scores[p]-=1;
        setscore(g,p);
        SENDALL(scorescript)
        return 3;
    }
    puts("Valid set");
    // We now have a good request
    g->scores[p]+=1;
    int out = g->out;
    if(out > 12 || g->dealt==81){
        // If set[i] contains one of [out-3 .. out), this will overwrite deck[set[i]]
        // before it matters
        for(int i=2; i>=0; i--){
            g->deck[idxs[i]] = g->deck[out-1 - i];
        }
        g->out -=3;
        out-=3;
    }
    else{
        for(int i=0;i<3;i++) g->deck[idxs[i]]=g->deck[ g->dealt++];
    }
    startcompose();
    addadd(g->deck, idxs);
    //See if more need to be dealt

    bool finished = add_cards(g);
    printf("out: %d; dealt: %d g1: %X",g->out,g->dealt, g->deck[1]);
    for(int i=out;i< g->out;i+=3){
        addadd(g->deck, nats+i);
    }
    addblank(g->out);
    addscore(g,p);
    endcompose();
    SENDALL(composedscript);
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
    if(memcmp(request,"GET ",4)==0){
        sendRedirect(sock,"/\r\n\r\n");
        return;
    }
    bool b = true;
    b = b && *(request++) == 'P';
    b = b && *(request++) == 'O';
    b = b && *(request++) == 'S';
    b = b && *(request++) == 'T';
    b = b && *(request++) == ' '; //TODO: RFC allows more/other whitespace
    b = b && *(request++) == '/';
    if(memcmp(request,"duel/",5)==0){request+=5;}
    int c;
    CHECK(!b)
    // Expected: join/<NAME>
    if (memcmp(request,"join/",5)==0){
        request+=5;
        puts("np");
        char name[5] = "    ";
        readname(&request,name);
        new_player(sock,name);
    }
    // Expected: <gamenum>/<name>/<NCSF1><NCSF2><NCSF3>
    else { // Assume it is a move
        puts("mv");
        unsigned int gnum=readnat(&request);
        CHECK(*(request++) != '/')
        char name[5] = "    ";
        readname(&request,name);
        CHECK(*(request++) != '/')
        card sel[3];
        for(int i=0; i<3; i++){
            sel[i]=fromStr(&request);
            CHECK(sel[i]==0)
        }
        //request is well formed
        int res = play_move(gnum,name,sel);
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
}


#include<main.c>
