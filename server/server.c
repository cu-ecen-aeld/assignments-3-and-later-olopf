#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <syslog.h>

#include "aesdsocket.h"

int create_server(const char *address, const char *port) {
  struct addrinfo hints, *res;
  int server_sock_fd;
  int opt = 1;
  int status;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if (getaddrinfo(address, port, &hints, &res) != 0) {
    log_error("Failed to get server details");
    return -1;
  }

  server_sock_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (server_sock_fd == -1) {
    log_errno("Failed to create server socket");
    return -1;
  }

  status =
      setsockopt(server_sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  if (status == -1) {
    log_errno("Failed to set socket options");
    return -1;
  }

  if (bind(server_sock_fd, res->ai_addr, res->ai_addrlen) == -1) {
    log_errno("Failed to bind server socket");
    return -1;
  }

  if (listen(server_sock_fd, CLIENT_BACKLOG) == -1) {
    log_errno("Failed to listen for client connection requests");
    return -1;
  }

  free(res);
  return server_sock_fd;
}

bool _handle_client_connection(struct AppState *state) {
  struct sockaddr_storage client_addr;
  socklen_t addr_size;
  int client_sock_fd;
  bool status;
  char peername[INET6_ADDRSTRLEN];

  addr_size = sizeof(client_addr);
  client_sock_fd = accept(state->server_sock_fd,
                          (struct sockaddr *)&client_addr, &addr_size);
  if (client_sock_fd == -1) {
    log_errno("Failed to accept client connection");
    return false;
  }

  getpeername(client_sock_fd, (struct sockaddr *)&client_addr, &addr_size);

  if (client_addr.ss_family == AF_INET) {
    struct sockaddr_in *s = (struct sockaddr_in *)&client_addr;
    inet_ntop(AF_INET, &s->sin_addr, peername, sizeof(peername));
  } else {
    struct sockaddr_in6 *s = (struct sockaddr_in6 *)&client_addr;
    inet_ntop(AF_INET6, &s->sin6_addr, peername, sizeof(peername));
  }
  log_info("Accepted connection from %s", peername);

  status = register_event(state->loop, client_sock_fd, EPOLLIN,
                          handle_client_msg, state);
  if (status == false) {
    return false;
  }

  return true;
}

void handle_client_connection(struct epoll_event *event) {
  log_debug("Handling client connect event");
  bool status;
  struct EventHandler *handler = (struct EventHandler *)event->data.ptr;
  struct AppState *state = (struct AppState *)handler->data;

  status = _handle_client_connection(state);
  if (status == false) {
    log_info("Setting stop flag to signal shutdown");
    deregister_event(state->loop, handler);
    *(state->stop) = true;
  }
  log_debug("Handled client connect event");
}
