PROG=ctorrent
CC=gcc
SRCS=ctorrent.c
CFLAGS+=-Wall -Werror -levent
MAN=


ctorrent:
	$(CC) $(SRCS) -o $(PROG) $(CFLAGS)

clean:
	rm $(PROG)




