CC=gcc
CFLAGS=-I.

server: $(wildcard *.c)
  $(CC) -o $@ server.c $(CFLAGS)
	
.PHONY: clean
clean:
	rm -f server
