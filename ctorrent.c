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


extern char *optarg;
extern int optind;
extern int optopt;
extern int opterr;
extern int optreset;

void usage(void) {
  
  printf("usage: ctorrent [-d] [-p port] [-l logfile] directory\n");
  exit(1);

}

int main(int argc, char *argv[]) {

  
  int daemonize = 0;
  int port = 0;
  int option;

  if (argc < 2) {
    usage();
  }

  while ((option = getopt(argc, argv, "dp:l:")) != -1) {
    switch (option) {
      case 'd':
        daemonize = 1;
        break;
      case 'p':
        /* convert optarg to a integer - check for errors and save it as the
         * port
         */
        port = 1;
        break;
      case 'l':
        /* Set STDOUT and STDERR to now be the logfile? */
        break;
      case '?':
      default:
        usage();
    }
  }

  /* Check for remaining options - should just be the directory. The directory
   * could also probably just be a default
   */

  return 0;
}
