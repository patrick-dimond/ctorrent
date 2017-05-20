CC=gcc
CFLAGS+=-Wall -Werror 

export CC CFLAGS 

all: 
	@$(MAKE) -c src/daemon
	@$(MAKE) -c src/cli

.PHONY: clean
clean:	
	@$(MAKE) -c src/daemon clean
	@$(MAKE) -c src/cli clean

