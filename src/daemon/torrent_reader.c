#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "bencode.h"


/* the "info" dictionary - Single mode for now */
struct torrent_info {

  int32_t piece_length;

  /* All 20 byte SHA1 hash values */
  char *pieces;

  /* optional - 0 or 1 */
  uint8_t private;

  char *name;

  int64_t length;

  char md5sum[32];


};

struct torrent_data {

  /* The url of the tracker */
  char *announce;

  /* optional - extension of spec */
  char **announce_list;

  /* creation date */
  int32_t created;

  /* optional comment */
  char *comment;

  /* optional name/version of prog */
  char *created_by;

  char *encoding;

  /* the info dictionary */
  struct info *info;

};

/* reading an integer */
int
hit_int(bencode_t *s, const char *dict_key, const long int val) {

  if (dict_key) {

    printf("%s\n", dict_key);

  }

  printf("Value is %ld\n", val);

  return 1;

}

int
do_nothing_str(bencode_t *s, const char *dict_key, unsigned int v_total_len, 
    const unsigned char *val, unsigned int v_len) {
  
  if (dict_key) {

    printf("%s\n", dict_key);

  }

  printf("%s\n", val);

  return 1;
}

int
do_nothing_enter_leave(bencode_t *s, const char *dict_key) {

  if (dict_key) {

    printf("%s\n", dict_key);

  }

  return 1;
}

int
do_nothing_next(bencode_t *s) {

  return 1;
}

void
set_callbacks(bencode_callbacks_t *cb) {

  cb->hit_int = hit_int;

  cb->hit_str = do_nothing_str;

  cb->dict_enter = do_nothing_enter_leave;

  cb->dict_leave = do_nothing_enter_leave;

  cb->list_enter = do_nothing_enter_leave;

  cb->list_leave = do_nothing_enter_leave;

  cb->list_next = do_nothing_next;

  cb->dict_next = do_nothing_next;

}


int
main(int argc, char *argv[]) {
  
  int fd;
  char buf[1024];

  char *file = argv[1];

  fd = open(file, O_RDONLY);

  if (fd == -1) {
    perror("Open failed");
    exit(1);
  }

  ssize_t recv;

  bencode_t *b_reader = malloc(sizeof(bencode_t));

  bencode_init(b_reader);

  bencode_callbacks_t cb;

  set_callbacks(&cb);

  struct torrent_data *tdata = malloc(sizeof(struct torrent_data));;

  b_reader = bencode_new(10, &cb, tdata);

  while(1) {

    recv = read(fd, buf, sizeof(buf));

    if (recv == -1) {
      perror("reading file");
      break;
    }

    if (recv == 0) { // EOF
      break;
    }

    if(!bencode_dispatch_from_buffer(b_reader, buf, recv)) {

      printf("bencode reading error\n");
      break;
    
    }

  }

  return 0;

}

