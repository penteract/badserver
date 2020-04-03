CC=gcc
CFLAGS=-I.
COMMON=cards.c requests.c headers.c main.c

.PHONY: all
all: server_duel server_infinite


server_duel: server_duel.c $(COMMON)
	$(CC) -o $@ $< $(CFLAGS)

server_infinite: server_infinite.c $(COMMON)
	$(CC) -o $@ $< $(CFLAGS)

.PHONY: clean
clean:
	rm -f server server_infinite
