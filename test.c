#include<stdio.h>
#include<string.h>

char s[] = "hello world";

int main(){
    sprintf(s,"%05x  %d",512,strcmp("hello","hello "));
    puts (s);
    sprintf(s,"%05x  %d",512,strcmp("hello ","hello"));
    puts (s);
    sprintf(s,"%05x  %d",512,strcmp("hello","hwllo"));
    puts (s);
    return 0;
}
