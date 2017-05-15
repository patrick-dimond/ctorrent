#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/http.h>

#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <errno.h>
#include <err.h>

#include <sys/queue.h>

#include <netinet/in.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <fcntl.h>

void usage(void) {
  
  printf("usage: ctorrent [-d] [-p port] directory\n");
  exit(1);

}

int main(int argc, char *argv[]) {

  usage();
  
  return 0;
}
