#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <errno.h>
#include <err.h>

#include <netinet/in.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
//#include <netdb.h>

#include <fcntl.h>

#include "local_comm.h"


/* handle parsing commands from local socket */
struct message {
  char buf[1024]; // build the command
  char *torrent_file; // torrent file
  uint8_t command;
  int32_t msg_len; // reported length
  int32_t recv_len; // length received
  uint8_t bad;
};

struct ud_comm {
  struct bufferevent *bufev;
  struct message *msg;
  struct event *read_event;
  struct event *write_event;
};

void free_ud_comm(struct ud_comm *);
void ud_command(struct bufferevent *, void *);
void ud_write(struct bufferevent *, void *);
void ud_event(struct bufferevent *, short, void *);
struct ud_comm *alloc_ud_comm(struct event_base *, evutil_socket_t);
void ud_accept(evutil_socket_t, short, void *);
void ud_response(struct bufferevent *, void*);

void
ud_response(struct bufferevent *bufev, void *ctx) {
  //struct ud_comm *comm = ctx;


  printf("Write triggered\n");

}


void 
ud_command(struct bufferevent *bufev, void *ctx) {
  struct ud_comm *comm = ctx;
  struct message *msg = comm->msg;
  int32_t recv;
  uint32_t comm_offset = 0;
  uint8_t finished = 0;

  printf("read triggered\n");


  while ((recv = bufferevent_read(bufev, msg->buf + msg->recv_len, 
          sizeof(msg->buf) - msg->recv_len - 1)) > 0) {

    msg->recv_len += recv;
    msg->buf[msg->recv_len] = '\0';

    printf("total recv: %d\n", msg->recv_len);

    for (uint32_t i = 0; i < msg->recv_len && msg->msg_len == 0; i++) {
      if (msg->buf[i] == ' ') { //got at least the length
        msg->buf[i] = '\0';
        msg->msg_len = strtol(msg->buf, NULL, 0); //BASE_TEN);
        if (msg->msg_len == 0) {
          if (errno == EINVAL || errno == ERANGE) {
            msg->bad = 1;
          }
        }

      }
    }   

    if (msg->recv_len == msg->msg_len) { // got whole message
      finished = 1;
      for (uint32_t i = 0; i < msg->recv_len; i++) {
        if (msg->buf[i] == '\0') {
          comm_offset = i + 1;
        }
        else if (msg->buf[i] == ' ') {
          msg->buf[i] = '\0';
          msg->torrent_file = msg->buf + i + 1;

        }
      }

      msg->command =  strtol(msg->buf + comm_offset, NULL, 0); //BASE_TEN);

      if (msg->command == 0) {
        if (errno == EINVAL || errno == ERANGE) {
          msg->bad = 1;
        }
      }

      printf("%d %d", msg->msg_len, msg->command);
      if (msg->torrent_file) {
        printf(" %s", msg->torrent_file);

      }
      printf("\n");
    }
  }
  if (finished == 1 || msg->bad == 1) {
     char *reply = "Message recv\n";
     bufferevent_write(bufev, reply, strlen(reply));
  }
}

void 
ud_event(struct bufferevent *bev, short events, void *ctx) {

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
  comm->msg->bad = 0;

  comm->bufev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

  if (comm->bufev == NULL) {
    free(comm);
    return NULL;
  }

  bufferevent_setcb(comm->bufev, ud_command, ud_response, ud_event, comm);

  bufferevent_enable(comm->bufev, EV_READ | EV_WRITE);

  return comm;

}

void
free_ud_comm(struct ud_comm *comm) {

  printf("Freeing\n");

  /* Remove all callbacks to free safely */
  bufferevent_setcb(comm->bufev, NULL, NULL, NULL, NULL);

  bufferevent_free(comm->bufev);

  free(comm->msg);

  free(comm);

  return;

}

void
ud_accept(evutil_socket_t listener, short event, void *arg) {

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
ud_setup(struct event_base *base, uint32_t port) {
  
  evutil_socket_t listener;
  struct sockaddr_un local;
  int32_t len;
  struct event *listener_event;

  /* TODO path needs to be read from config */
  strcpy(local.sun_path, "/Users/psd/Documents/ctorrent/ctor.sock");
  if (unlink(local.sun_path) == -1) {
    printf("socket unlink failed\n");
  }
  local.sun_family = AF_UNIX;

  len = strnlen(local.sun_path, 108) + sizeof(local.sun_family);

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

  listener_event = event_new(base, listener, EV_READ|EV_PERSIST, ud_accept, (void*)base);
  /*XXX check it */
  event_add(listener_event, NULL);

}
