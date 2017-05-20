PROG = ../../ctor_daemon

SRCS = ctor_daemon.c

CFLAGS+=-levent -Wno-deprecated-declarations

all: ctor_daemon

ctor_daemon: $(SRCS)
	$(CC) $(SRCS) -o $(PROG) $(CFLAGS)

.PHONY: clean
clean:
	rm $(PROG)

