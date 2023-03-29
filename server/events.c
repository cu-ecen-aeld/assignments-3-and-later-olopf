#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <syslog.h>
#include <unistd.h>

#include "aesdsocket.h"

bool _deregister_event(struct EventLoop *loop, int idx);

struct EventLoop *create_loop() {
  struct EventLoop *loop = malloc(sizeof(struct EventLoop));
  if (loop == NULL) {
    log_error("Failed to allocate memory for event loop");
    return NULL;
  }
  
  loop->_epfd = epoll_create1(0);
  if (loop->_epfd == -1) {
    log_errno("Failed to create event loop");
    free(loop);
    return NULL;
  }
  for (int i = 0; i < MAX_HANDLERS; i++) loop->_handlers[i] = NULL;
  
  loop->_nhandlers = 0;
  return loop;
}

bool destroy_loop(struct EventLoop *loop) {
  log_debug("Deregistering all handlers");
  for (int i = 0; i < MAX_HANDLERS; i++) {
    if (loop->_handlers[i] != NULL) {
      _deregister_event(loop, i);
    }
  }

  log_debug("Closing event loop");
  close(loop->_epfd);
  
  log_debug("Freeing memory");
  free(loop);

  log_debug("Loop destroyed");
  return true;
}

bool _run(struct EventLoop *loop, bool *stop, struct epoll_event *events) {
  int nevents, i;
  bool status;
  struct EventHandler *handler;

  nevents = epoll_wait(loop->_epfd, events, MAX_EVENTS, -1);
  if (*stop == true) {
    log_info("Stop flag set");
    return true;
  }
  if (nevents == -1) {
    log_errno("Failed waiting for events");
    return false;
  }

  for (i = 0; i < nevents; i++) {
    if (*stop == true) break;
    handler = (struct EventHandler *)events[i].data.ptr;
    log_debug("Got event for fd %d", handler->fd);
    handler->callback(&events[i]);
  }
  return true;
}

bool run(struct EventLoop *loop, bool *stop) {
  struct epoll_event *events;
  bool status;

  events = malloc(sizeof(struct epoll_event) * MAX_EVENTS);
  if (events == NULL) {
    log_errno("Failed to allocate memory for events");
    return false;
  }

  while (*stop == false) {
    status = _run(loop, stop, events);
    if (status == false) {
      break;
    }
  }

  free(events);
  return status;
}

bool register_event(struct EventLoop *loop, int fd, uint32_t events,
                    void (*cb)(struct epoll_event *event),
                    void *cb_data) {
  int status;
  struct epoll_event event;
  struct EventHandler *handler;

  log_debug("Registering event for fd %d", fd);
  if (loop->_nhandlers == MAX_HANDLERS) {
    log_warning("Cannot add another event handler, already at maximum");
    return false;
  }

  handler = malloc(sizeof(struct EventHandler));
  if (handler == NULL) {
    log_errno("Failed to allocate memory for event handler data");
    return false;
  }

  handler->callback = cb;
  handler->data = cb_data;
  handler->fd = fd;

  event.events = events;
  event.data.ptr = handler;

  status = epoll_ctl(loop->_epfd, EPOLL_CTL_ADD, fd, &event);
  if (status == -1) {
    log_errno("Failed to register event for fd %d", fd);
    free(handler);
    return false;
  }

  for (int i = 0; i < MAX_HANDLERS; i++) {
    if (loop->_handlers[i] == NULL) {
      loop->_handlers[i] = handler;
      break;
    }
  }
  loop->_nhandlers += 1;

  log_debug("Registered event for fd %d", fd);
  return true;
}

bool deregister_event(struct EventLoop *loop, struct EventHandler *handler) {
  bool status;
  int idx;

  log_debug("Deregistering event for fd %d", handler->fd);
  if (loop->_nhandlers == 0) {
    log_warning("No handlers have been registered");
    return false;
  }
  
  status = false;
  for (idx = 0; idx < MAX_HANDLERS; idx++) {
    if (loop->_handlers[idx] == handler) {
      status = _deregister_event(loop, idx);
      break;
    }
  }

  if (idx == MAX_HANDLERS) {
    log_warning("Handler is not registered with loop");
    return false;
  } else {
    return status;
  }
}

bool _deregister_event(struct EventLoop *loop, int idx) {
  int status;
  struct EventHandler *handler = loop->_handlers[idx];

  status = epoll_ctl(loop->_epfd, EPOLL_CTL_DEL, handler->fd, NULL);
  if (status == -1) {
    log_errno("Failed to deregister event for fd %d", handler->fd);
    status = false;
  } else {
    log_debug("Deregistered event for fd %d", handler->fd);
    status = true;
  }

  free(handler);
  loop->_handlers[idx] = NULL;
  loop->_nhandlers -= 1;
  return status;
}
