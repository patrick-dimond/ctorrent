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

#include <sys/stat.h>

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

const uint8_t BASE_TEN = 0;
/* Change log mode to be something better */
const mode_t LOG_MODE = S_IWUSR; 

void usage(void) {
  
  printf("usage: ctorrent [-d] [-p port] [-l logfile]\n");
  exit(1);

}

int main(int argc, char *argv[]) {

  
  uint8_t daemonize = 0;
  uint32_t port = 0;
  int32_t option;
  int32_t log_fd = -1;

  if (argc < 2) {
    usage();
  }

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
  if (daemon(1, 0) == -1 ) { 
    err(errno, "daemon error");
  }

  return 0;
}
