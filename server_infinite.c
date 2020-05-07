// A single never stopping game with up to MAXCONNS players.

#define PORT 8082

#include<headers.c>

#define REDIRECTTYPE "307 Temporary Redirect"
#include<utils.c>
#include<cards.c>

#define MAXCONNS 500
struct game{
    int players[MAXCONNS];
    int scores[MAXCONNS];
    char names[MAXCONNS][NAMELENGTH];
    int curr; // number of live connections
    
    card deck[81];
    int dealt; //number of cards dealt
    int out; //number of cards currently in play
};
typedef struct game game;

#include<requests.c>

#define SWP(a,b) \
    if ((a)!=(b)){\
        a ^= b;\
        b ^= a;\
        a ^= b;\
    }


// How not to make a webserver


struct game gm;


int new_conn(game* g, int sock, char* name){
    if (g->curr>=MAXCONNS) return -1;

    g->scores[g->curr]=0;
    for(int i=0;i<NAMELENGTH;i++){
        g->names[g->curr][i]=name[i];
    }
    g->players[g->curr++]=sock;

    return (g->curr-1);
}

int remove_conn(game* g, int idx){
    if (idx<0 || idx>=g->curr){
        puts("Bad connection removal");
        return -1;
    }
    close(g->players[idx]);
    g->players[idx]=g->players[g->curr-1];
    g->scores[idx]=g->scores[g->curr-1];
    for(int i=0;i<NAMELENGTH;i++){
        g->names[idx][i]=g->names[g->curr-1][i];
    }
    g->curr-=1;
    return 0;
}

void shuffle_from(game* g, int keep){
    // Reminder: Shuffling is easy to get wrong
    // https://en.wikipedia.org/wiki/Fisher-Yates_shuffle
    for(int i=keep;i<81;i++){
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
}

void add_3(game* g){
    if(g->dealt>78){
        shuffle_from(g,g->out);
        g->dealt=g->out;
    }
    for(int n=0;n<3;n++){
        SWP(g->deck[g->out], g->deck[g->dealt])
        g->out++ ; g->dealt++;
    }
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
        add_3(g);
        if(g->out > 21){
            puts("ERROR in add_cards, too many cards without a set");
            exit(1);
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
    shuffle_from(g,0);
    g->dealt=12;
    g->out=12;
    add_cards(g);
}

void send_all(game* g, char* msg){
    for(int i=0; i< g->curr;i++){
        while(snd(g->players[i],msg)){
            remove_conn(g, i);
            if(i>=g->curr)break;
        }
    }
}

int send_init(int sock){
    if(snd(sock,headers)) return -1;
    if(snd(sock,initialBody)) return -1;
    return 0;
}

int new_player(int sock, char* name){
    game* g = &gm;
    int pl = new_conn(g, sock, name);
    if (send_init(sock)){
        remove_conn(g,pl);
        return -1;
    }

    // modify startscript
    sprintf(gamenum,"%4d",0);
    gamenum[4]=' ';
    memcpy(startname,name,NAMELENGTH);
    if (snd(sock, startscript)){
        remove_conn(g,pl);
        return -1;
    }

    startcompose();
    addblank(g->out);
    for(int i=0;i< g->out;i+=3){
        addadd(g->deck, nats+i);
    }
    endcompose();
    if (snd(sock, composedscript)){
        remove_conn(g,pl);
        return -1;
    }
    return 0;
}

#define SWPGT(a,b) \
    if ((a)>(b)){\
        a ^= b;\
        b ^= a;\
        a ^= b;\
    }

// TODO(not worth the effort unless MAXCONNS is increased):
//   find a better method than linear search
int find_player(game* g , char* name){
    for(int p=0; p < g->curr; p++){
        if (memcmp(name,g->names[p],NAMELENGTH)==0) return p;
    }
    return -1;
}

//Errors: 1 - not a game in progress
//        2 - not 3 distinct dealt cards (should never come from a 0-latency well behaved client)
//        3 - not a set
//        4 - not a known player
int play_move(unsigned int idx, char* pname, card* set){
    printf("game %d, set %d %d %d\n",idx,set[0],set[1],set[2]);
    struct game* g = &gm;
    int p = find_player(g,pname);
    if(p<0) return 4;
    // Turn cards into indicies
    int idxs[3];
    for(int i=0;i<3;i++){
        card* c = memchr(g->deck,set[i],g->out);
        if (c==0) return 4;
        idxs[i]=c-g->deck;
    }
    //sort indicies
    SWPGT(idxs[0],idxs[1])
    SWPGT(idxs[1],idxs[2])
    SWPGT(idxs[0],idxs[1])
    if(idxs[0]==idxs[1] || idxs[1]==idxs[2])
        return 2;
    if( idxs[2]>= g->out)
        return 2;
    puts("not broken");
    if(!isTriple(set[0],set[1],set[2])){
        g->scores[p]-=1;
        setscore((game*)(((int*)g) + p), 0);
        send_all(g, scorescript);
        return 3;
    }
    puts("Valid set");
    // We now have a good request
    g->scores[p]+=1;

    int out = g->out;
    if(out > 12){
        // If set[i] contains one of [out-3 .. out), this will overwrite deck[set[i]]
        // before it matters
        for(int i=2; i>=0; i--){
            SWP(g->deck[idxs[i]], g->deck[out-i-1]);
        }
        g->out -=3;
        out-=3;
    }
    else{
        if(g->dealt>78){
            // TODO: either include the cards in the set when shuffling, or come up with a good justification for why not (note that they are included when shuffled in add_cards )
            shuffle_from(g,g->out);
            g->dealt=g->out;
        }
        for(int i=0;i<3;i++){
            SWP(g->deck[idxs[i]], g->deck[g->dealt])
            g->dealt++;
        }
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
    addscore((game*)(((int*)g) + p), 0);
    endcompose();
    send_all(g, composedscript);
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
    if(memcmp(request,"inf/",4)==0){request+=4;}
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

#define SETUP \
    if (setup()){\
        puts("Setup failed");\
        return 1;\
    }\
    init_deck(&gm);


#include<main.c>
