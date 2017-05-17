PROG=ctorrent
CC=gcc
SRCS=ctorrent.c
CFLAGS+=-Wall -Werror -levent -Wno-deprecated-declarations
MAN=


ctorrent:
	$(CC) $(SRCS) -o $(PROG) $(CFLAGS)

clean:
	rm $(PROG)




