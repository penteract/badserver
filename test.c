#include<stdio.h>
#include<string.h>

char s[] = "hello world";
#define A 6

#define STR(x) #x" x"   

int main(){
    puts(STR( hello  world));
    sprintf(s,"%05x  %d A ",512,strcmp("hello","hello "));
    puts (s);
    sprintf(s,"%05x  %d",512,strcmp("hello ","hello"));
    puts (s);
    sprintf(s,"%05x  %4d",512,strcmp("hello","hwllo"));
    puts (s);
    return 0;
}
