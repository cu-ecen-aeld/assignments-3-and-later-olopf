#include <arpa/inet.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <syslog.h>
#include <unistd.h>

#include "aesdsocket.h"

bool recv_data(FILE *stream, char **buffer) {
  log_debug("Receiving data from %d", stream->_fileno);

  size_t len = MAX_RECORD_SIZE;
  ssize_t nread;

  nread = getline(buffer, &len, stream);
  if (nread < 0) {
    log_error("Failed reading record from socket %d", stream->_fileno);
    return false;
  } else {
    log_debug("Received line from %d", stream->_fileno);
    return true;
  }
}

bool send_data(FILE *stream, char *data) {
  log_debug("Sending data to client");
  if (fputs(data, stream) == EOF) {
    log_error("Failed to write data to %d", stream->_fileno);
    return false;
  } else {
    log_debug("Sent data to client");
    return true;
  }
}

bool pipe_records(char *data, void *stream) {
  return send_data((FILE *)stream, data);
}

bool _handle_client_msg(FILE *stream, struct AppState *state) {
  bool status = false;
  char *buffer = NULL;

  if (recv_data(stream, &buffer) == true) {
    if (write_records(state->config->data_file, buffer) == true) {
      if (read_records(state->config->data_file, pipe_records, stream) ==
          true) {
        status = true;
      }
    }
  }
  free(buffer);
  buffer = NULL;

  return status;
}

void close_client_socket(int fd) {
  char peername[INET6_ADDRSTRLEN];
  struct sockaddr_storage client_addr;
  socklen_t addr_size;

  addr_size = sizeof(client_addr);
  getpeername(fd, (struct sockaddr *)&client_addr, &addr_size);

  if (client_addr.ss_family == AF_INET) {
    struct sockaddr_in *s = (struct sockaddr_in *)&client_addr;
    inet_ntop(AF_INET, &s->sin_addr, peername, sizeof(peername));
  } else {
    struct sockaddr_in6 *s = (struct sockaddr_in6 *)&client_addr;
    inet_ntop(AF_INET6, &s->sin6_addr, peername, sizeof(peername));
  }
  log_debug("Closing client socket %d", fd);
  if (shutdown(fd, SHUT_RDWR) == -1) {
    log_errno("Failed to close client socket %d", fd);
  } else {
    log_info("Closed connection from %s", peername);
  }
}

void handle_client_msg(struct epoll_event *event) {
  log_debug("Handling client msg event");
  struct EventHandler *handler = (struct EventHandler *)event->data.ptr;
  struct AppState *state = (struct AppState *)handler->data;

  FILE *stream = fdopen(dup(handler->fd), "r+b");
  if (stream == NULL) {
    log_errno("Failed to open %d as a rw stream", event->data.fd);
  } else {
    _handle_client_msg(stream, state);
    fflush(stream);
    fclose(stream);
  }
  
  close_client_socket(handler->fd);
  deregister_event(state->loop, handler);

  log_debug("Handled client msg event");
}
