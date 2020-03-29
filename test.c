#include<stdio.h>
#include<string.h>

char s[] = "hello world";

int main(){
    sprintf(s,"%05x",512);
    puts (s);
    return 0;
}
