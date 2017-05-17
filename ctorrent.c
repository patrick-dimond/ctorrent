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
#include <netdb.h>

#include <fcntl.h>


extern char *optarg;
extern int optind;
extern int optopt;
extern int opterr;
extern int optreset;

const uint8_t BASE_TEN = 0;
/* TODO Change log mode to be something better */
const mode_t LOG_MODE = S_IWUSR; 


struct fd_comm {

  /* http://www.wangafu.net/~nickm/libevent-book/Ref6_bufferevent.html 
   * Managing the bufferevent - important bits include
   * Changing the read / write event
   * Turning on / off the read/ write event
   */

  struct bufferevent *bufev;

  struct event *read_event;
  struct event *write_event;

};

void usage(void) {
  
  printf("usage: ctorrent [-d] [-p port] [-l logfile]\n");
  exit(1);

}

struct fd_comm *alloc_fd_comm(struct event_base *base, evutil_socket_fd fd) {

  struct fd_comm *comm = malloc(sizeof(struct fd_comm));

  if (comm == NULL) {
    return NULL;
  }

  comm->read = evbuffer_new();
  

  comm->write = evbuffer_new();




}

void
do_accept(evutil_socket_t listener, short event, void *arg) {

    struct event_base *base = arg;
    struct sockaddr_storage ss;
    socklen_t slen = sizeof(ss);
    evutil_socket_t fd = accept(listener, (struct sockaddr*)&ss, &slen);
    if (fd < 0) {
        perror("accept");
    } else if (fd > FD_SETSIZE) {
        evutil_closesocket(fd); // XXX replace all closes with EVUTIL_CLOSESOCKET */
    } else {
        struct fd_comm *comm;
        evutil_make_socket_nonblocking(fd);
        comm = alloc_fd_state(base, fd);
        assert(state); /*XXX err*/
        assert(state->write_event);
        event_add(state->read_event, NULL);
    }
}


void run(uint32_t port) {
  
  evutil_socket_t listener;
  struct sockaddr_un local;
  int32_t len;
  struct event_base *base;
  struct event *listener_event;

  base = event_base_new();
  if (base == NULL) {
      exit(1); 
  }

  /* TODO path needs to be generated - probably some checking to do too */
  strcpy(local.sun_path, "/Users/psd/Documents/ctorrent/ctorsock");
  if (unlink(local.sun_path) == -1) {
    err(errno, "unlink");
  }

  len = strnlen(local.sun_path, 108) + sizeof(local.sun_family);

  /* To be used when actually creating tcp/ip sockets
  sin.sin_family = AF_INET; // ipv4
  sin.sin_addr.s_addr = 0; // ??
  sin.sin_port = htons(port); // set preferred port - might have to make this dynamic
  */

  listener = socket(PF_LOCAL, SOCK_STREAM, 0); // 0 is ip protocol /etc/protocols
  evutil_make_socket_nonblocking(listener); // sets O_NONBLOCK

  /* TODO Lots of socket options - Will need to do some research */
  int one = 1;
  setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

  /* Not using len - is it needed? maybe remove after testing */
  if (bind(listener, (struct sockaddr*)&local, sizeof(local)) < 0) {
      err(errno, "bind");
      return;
  }
  /* TODO Allow a max of 16 pending connections - Is there an optimum number?*/
  if (listen(listener, 16) < 0) {
      err(errno, "listen");
      return;
  }

  listener_event = event_new(base, listener, EV_READ|EV_PERSIST, do_accept, (void*)base);
  /*XXX check it */
  event_add(listener_event, NULL);

  event_base_dispatch(base);

}


int main(int argc, char *argv[]) {

  
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

  run();

  return 0;
}


/* Thoughts
 * Do I care enought to only use err or perror? Does it matter?
 * I should make it work with ipv4 and ipv6 - welcome to the future
 */
