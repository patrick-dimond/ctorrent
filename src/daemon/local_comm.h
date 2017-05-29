#ifndef LOCAL_COMM_H
#define LOCAL_COMM_H

#include <stdint.h>
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/util.h>

void ud_setup(struct event_base *, uint32_t);

#endif /* LOCAL_COMM_H */
