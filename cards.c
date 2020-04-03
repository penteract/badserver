// 01 10 11
// 1  2  3
// R  G  P
// E  H  F
// D  O  S
char names[] = "123RGPEHFDOS";
// cards are bytes represented by the bitpattern #NNCCFFSS
typedef unsigned char card;

card thrd(card a, card b){
    card c = a^b;
    //isolate the fields which are identical in a and b (pairs where a.part^b.part==0)
    card f = (c|(c<<1))&0xAA;
    // turn this into a mask
    f=~(f|(f>>1));
    //printf("%02X\n",f);
    c=c|(a&f);
    return c;
}
bool isTriple(card a,card b,card c){
    return c==thrd(a,b);
}
void pr(card a,card b){
    printf("%02x %02x %02x\n",a,b,thrd(a,b));
}
void test(){
    pr(0b11011011,0b01111011);
    pr(0b11111111,0b11111111);
    pr(0b11111111,0b10101101);
    pr(0b11010101,0b10101101);
    pr(0b01111110,0b11101101);
    pr(189,123);
}

void toStr(char* s, card c){
    for(int i=0; i<4; i++){
        s[i]=names[i*3+(c&3)-1];
        c>>=2;
    }
}
#define GAME
