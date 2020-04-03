// A single never stopping game with up to MAXCONNS players.

#define PORT 8082

#include<headers.c>

#include<cards.c>
#include<requests.c>

#define MAXCONNS 500
struct game{
    //sockets TODO: make this an array of players
    int players[MAXCONNS];
    int curr; // number of connections
    //Invariant: players[i] is nonzero for exactly 'live' indicies i
    
    card deck[81];
    int dealt; //number of cards dealt
    int out; //number of cards currently in play
};
typedef struct game game;


#define SWP(a,b) \
    if ((a)!=(b)){\
        a ^= b;\
        b ^= a;\
        a ^= b;\
    }


// How not to make a webserver


struct game gm;


int new_conn(game* g, int sock){
    if (g->curr>=MAXCONNS) return -1;
    g->players[g->curr++]=sock;
    return (g->curr-1);
}

int remove_conn(game* g, int idx){
    if (idx<0 || idx>=g->curr){
        puts("Bad connection removal");
        return -1;
    }
    g->players[idx]=g->players[gm.curr-1];
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

bool add_3(game* g){
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

int send_all(game* g, char* msg){
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

int new_player(int sock){
    game* g = &gm;
    int idx = new_conn(g,sock);
    if (send_init(sock)){
        remove_conn(g,idx);
        return -1;
    }
    
    // modify startscript
    sprintf(gamenum,"%4d",0);
    gamenum[4]=' ';
    if (snd(sock, startscript)){
        remove_conn(g,idx);
        return -1;
    }
    //modify blankscript
    sprintf(blanknum,"%2d",g->out);
    blanknum[2]=' ';
    if (snd(sock, blankscript)){
        remove_conn(g,idx);
        return -1;
    }
    
    for(int i=0; i< g->out; i+=3){
        //modify addscript
        for(int j=0;j<3;j++){
            *(addids[j])= 'a'+i+j;
            toStr(addvals[j], g->deck[i+j]);
        }
        if (snd(sock, addscript)){
            remove_conn(g,idx);
            return -1;
        }
    }
    return 0;
}

//Errors: 1 - not a game in progress
//        2 - not 3 distinct dealt cards (should never come from a 0-latency well behaved client)
//        3 - not a set
#define SWPGT(a,b) \
    if ((a)>(b)){\
        a ^= b;\
        b ^= a;\
        a ^= b;\
    }

int play_move(unsigned int idx, int* set){
    printf("game %d, set %d %d %d\n",idx,set[0],set[1],set[2]);
    struct game* g = &gm;
    char s[] = "XXXX XXXX XXXX";
    for(int i=0;i<3;i++) toStr(s+i*5,g->deck[set[i]]);
    puts(s);
    printf("game %d, set %d %d %d\n",idx,g->deck[set[0]],g->deck[set[1]],g->deck[set[2]]);
    //sort set
    SWPGT(set[0],set[1])
    SWPGT(set[1],set[2])
    SWPGT(set[0],set[1])
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
    if(out > 12){
        // If set[i] contains one of [out-3 .. out), this will overwrite deck[set[i]]
        // before it matters
        for(int i=2; i>=0; i--){
            SWP(g->deck[set[i]], g->deck[out-i-1]);
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
            SWP(g->deck[set[i]], g->deck[g->dealt])
            g->dealt++;
        }
    }
    startcompose();
    //modify addscript
    addadd(g->deck, set);
    //See if more need to be dealt

    bool finished = add_cards(g);
    printf("out: %d; dealt: %d g1: %X",g->out,g->dealt, g->deck[1]);
    for(int i=out;i< g->out;i+=3){
        //modify addscript
        addadd(g->deck, nats+i);
    }
    addblank(g->out);
    //sprintf(blanknum,"%2d",g->out);
    //blanknum[2]=' ';
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
    bool b = true;
    b = b && *(request++) == 'G';
    b = b && *(request++) == 'E';
    b = b && *(request++) == 'T';
    b = b && *(request++) == ' '; //TODO: check if RFC allows more/other whitespace
    b = b && *(request++) == '/';
    if(memcmp(request,"inf/",4)==0){request+=4;}
    int c;
    CHECK(!b)
    if (memcmp(request," ",1)==0){
        new_player(sock);
    }
    else { // Assume it is a move
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

#define SETUP \
    if (setup()){\
        puts("Setup failed");\
        return 1;\
    }\
    init_deck(&gm);


#include<main.c>
