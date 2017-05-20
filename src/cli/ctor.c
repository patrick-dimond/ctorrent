#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>

// TODO  Needs to be read from a config file
char *socket_path = "/Users/psd/Documents/ctorrent/ctor.sock";

enum command_enum {
  add = 1,
  info,
  stop
};

/* How messages are generated
 * len command [torrent]
 *
 * len is the total length of the whole message string.
 *
 * command is a number that correseponds to the command enum
 *
 * [torrent] is the name of the torrent file
 *
 */

void
usage(void) {

  printf("Usage: ctor command [torrent]\n");
  exit(1);
}

void
generate_message(char *buf, int argc, char *argv[]) {

  char *file = NULL;
  enum command_enum command;


  if (strcmp(argv[1], "add") == 0 && argc == 3) {

    command = add;
    file = argv[2];

  } else if (strcmp(argv[1], "info") == 0) {
  
    if (argc > 2){
      file = argv[2];
    }
    command = info;

  } else if (strcmp(argv[1], "stop") == 0) {

    if (argc > 2){
      file = argv[2];
    }
    command = stop;

  } else {

    printf("%d -  %s\n", argc, argv[1]);
    usage();
  }

  uint32_t len = 0;
  int32_t result;
  if (file) {
    len = strlen(file) + 3 ;
    if (len < 9) {
      len += 1;
    } else if (len < 98) {
      len += 2;
    } else if (len < 997) {
      len += 3;
    } else {
      len += 4;
    }
    result = snprintf(buf, 1024, "%d %d %s", len, command, file);
    if (result > 1023) {
      printf("Filename too large\n");
      exit(1);
    }
  } else {
    len = 3;
    snprintf(buf, 1024, "%d %d", len, command);
  }

  return;
}

int main(int argc, char *argv[]) {

  char buf[1024];

  if (argc < 2) {
    usage();
  }

  generate_message(buf, argc, argv);

  printf("%s\n", buf);

  struct sockaddr_un addr;
  int fd;

  if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    perror("socket error");
    exit(1);
  }

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);

  if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    perror("connect error");
    exit(1);
  }

  size_t message_length = strlen(buf);
  ssize_t written = 0;
  ssize_t recv;
  do {
    written = write(fd, buf + written, message_length - written);
  
  } while(written < message_length);

  while( (recv = read(STDIN_FILENO, buf, 1023)) > 0) {
    buf[recv + 1] = '\0';
    printf("%s", buf);
  }

  return 0;
}
