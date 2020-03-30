CC=gcc
CFLAGS=-I.

server: server.c cards.c requests.c
	$(CC) -o $@ $< $(CFLAGS)

server_infinite: server_infinite.c cards.c requests.c
	$(CC) -o $@ $< $(CFLAGS)

.PHONY: clean
clean:
	rm -f server server_infinite
