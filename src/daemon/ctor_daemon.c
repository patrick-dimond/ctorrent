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

#include "torrent_reader.h"


extern char *optarg;
extern int optind;
extern int optopt;
extern int opterr;
extern int optreset;

const uint8_t BASE_TEN = 0;
/* TODO Change log mode to be something better */
const mode_t LOG_MODE = S_IWUSR; 

/* handle parsing commands from local socket */
struct message {
  char buf[1024]; // build the command
  char *torrent_file; // torrent file
  uint8_t command;
  int32_t msg_len; // reported length
  int32_t recv_len; // length received
};

struct ud_comm {

  struct bufferevent *bufev;

  struct message *msg;

  struct event *read_event;
  struct event *write_event;

};

void free_ud_comm(struct ud_comm *);

void 
usage(void) {
  
  printf("usage: ctorrent [-d] [-p port] [-l logfile]\n");
  exit(1);

}

void 
local_command(struct bufferevent *bufev, void *ctx) {
  struct ud_comm *comm = ctx;
  struct message *msg = comm->msg;
  int32_t recv;
  uint32_t comm_offset = 0;


  while ((recv = bufferevent_read(bufev, msg->buf + msg->recv_len, 
          sizeof(msg->buf) - msg->recv_len - 1)) > 0) {

    msg->recv_len += recv;
    msg->buf[msg->recv_len] = '\0';

    printf("total recv: %d\n", msg->recv_len);

    for (uint32_t i = 0; i < msg->recv_len && msg->msg_len == 0; i++) {
      if (msg->buf[i] == ' ') { //got at least the length
        msg->buf[i] = '\0';
        msg->msg_len = strtol(msg->buf, NULL, BASE_TEN);
        if (msg->msg_len == 0) {
          if (errno == EINVAL || errno == ERANGE) {
            printf("Bad message length\n"); // Reply that it was a bad message
          }
        }

      }
    }   

    if (msg->recv_len == msg->msg_len) { // got whole message
      for (uint32_t i = 0; i < msg->recv_len; i++) {
        if (msg->buf[i] == '\0') {
          comm_offset = i + 1;
        }
        else if (msg->buf[i] == ' ') {
          msg->buf[i] = '\0';
          msg->torrent_file = msg->buf + i + 1;

        }
      }

      msg->command =  strtol(msg->buf + comm_offset, NULL, BASE_TEN);

      if (msg->command == 0) {
        if (errno == EINVAL || errno == ERANGE) {
          printf("Bad Command\n"); // Reply that it was a bad message
        }
      }

      printf("%d %d", msg->msg_len, msg->command);
      if (msg->torrent_file) {
        printf(" %s", msg->torrent_file);

      }
      printf("\n");
    }
  }
}

void 
local_event(struct bufferevent *bev, short events, void *ctx) {

    struct ud_comm *comm = ctx;
    int finished = 0;

    if (events & BEV_EVENT_EOF) {
      printf("EOF received from local sock\n");
      finished = 1;
    }
    if (events & BEV_EVENT_ERROR) {
      printf("Error on local sock\n");
      finished = 1;
    }
    if (events & BEV_EVENT_TIMEOUT) {
      printf("Timeout on local sock\n");
      finished = 1;
    }
    if (finished) {
        free_ud_comm(comm);
    }
}

struct ud_comm *
alloc_ud_comm(struct event_base *base, evutil_socket_t fd) {

  struct ud_comm *comm = malloc(sizeof(struct ud_comm));

  if (comm == NULL) {
    return NULL;
  }

  comm->msg = malloc(sizeof(struct message));

  if (comm->msg == NULL) {
    free(comm);
    return NULL;
  }

  comm->msg->recv_len = 0;
  comm->msg->msg_len = 0;
  comm->msg->torrent_file = NULL;

  comm->bufev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

  if (comm->bufev == NULL) {
    free(comm);
    return NULL;
  }

  bufferevent_setcb(comm->bufev, local_command, NULL, local_event, comm);

  bufferevent_enable(comm->bufev, EV_READ);

  return comm;

}

void
free_ud_comm(struct ud_comm *comm) {

  /* Remove all callbacks to free safely */
  bufferevent_setcb(comm->bufev, NULL, NULL, NULL, NULL);

  bufferevent_free(comm->bufev);

  free(comm->msg);

  free(comm);

  return;

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
        struct ud_comm *comm;
        evutil_make_socket_nonblocking(fd);
        comm = alloc_ud_comm(base, fd);
    }
}


void 
run(uint32_t port) {
  
  evutil_socket_t listener;
  struct sockaddr_un local;
  int32_t len;
  struct event_base *base;
  struct event *listener_event;

  base = event_base_new();
  if (base == NULL) {
      exit(1); 
  }

  /* TODO path needs to be read from config */
  strcpy(local.sun_path, "/Users/psd/Documents/ctorrent/ctor.sock");
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
