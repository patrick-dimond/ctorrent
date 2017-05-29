#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/http.h>
#include <event2/util.h>

#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <errno.h>
#include <err.h>

#include <sys/queue.h>

#include <sys/stat.h>

#include <netinet/in.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>

#include <fcntl.h>

#include "local_comm.h"


extern char *optarg;
extern int optind;
extern int optopt;
extern int opterr;
extern int optreset;

const uint8_t BASE_TEN = 0;
/* TODO Change log mode to be something better */
const mode_t LOG_MODE = S_IWUSR; 

void 
usage(void) {
  
  printf("usage: ctorrent [-d] [-p port] [-l logfile]\n");
  exit(1);
}


void 
run(uint32_t port) {
  
  struct event_base *base;

  base = event_base_new();
  if (base == NULL) {
      exit(1); 
  }

  ud_setup(base, port);

  // set up to listen on another port for peer and tracker talking

  event_base_dispatch(base);

}


int 
main(int argc, char *argv[]) {

  
  uint8_t daemonize = 0;
  uint32_t port = 6881; // one of the default torrent ports
  int32_t option;
  int32_t log_fd = -1;


  while ((option = getopt(argc, argv, "dp:l:")) != -1) {
    switch (option) {
      case 'd':
        daemonize = 1;
        break;
      case 'p':
        port = strtol(optarg, NULL, BASE_TEN);
        if (port == 0) {
          if (errno == EINVAL || errno == ERANGE) {
            printf("Bad port\n");
            usage();
          }
        }
        break;
      case 'l':
        /* Does not following symlinks matter? What about using non block? */
        log_fd = open(optarg, O_WRONLY | O_APPEND | O_NOFOLLOW | O_CREAT |
            O_NONBLOCK, LOG_MODE);
        if (log_fd == -1) {
          err(errno, "Bad logfile - %s", optarg);
        }

        break;
      case '?':
      default:
        usage();
    }
  }
  argc -= optind;
  argv += optind;

  if (argc != 0) {
    usage();
  }

  if (log_fd != -1) {
    /* Set STDOUT and STDERR to now be the logfile */
    /* Leave STDIN for now - should that be closed? */
    if (dup2(log_fd, STDOUT_FILENO) == -1) {
        err(errno, "dup2");
      }
    if (dup2(log_fd, STDERR_FILENO) == -1) {
      err(errno, "dup2");
    }
  }
  /* TODO: confirm daemon works correctly with file descriptors */
  if (daemonize != 1) {
    if (daemon(1, 0) == -1 ) { 
      err(errno, "daemon error");
    }
  }

  run(port);

  return 0;
}


/* Thoughts
 * I should make it work with ipv4 and ipv6 - welcome to the future
 * Will need bufferevent timeouts !
 * void bufferevent_set_timeouts(struct bufferevent *bufev,
    const struct timeval *timeout_read, const struct timeval *timeout_write);
    The event callback is then invoked with either BEV_EVENT_TIMEOUT|BEV_EVENT_READING or BEV_EVENT_TIMEOUT|BEV_EVENT_WRITING.

    CLean up everything before starting to contact the torrent server
      - Start moving everything into separate c files
      - better make file
      - clean up all comments
    write tests + a design doc
 e*/
