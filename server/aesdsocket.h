#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#ifndef USAGE
#define USAGE "aesdsocket [-d -i ADDRESS -p PORT -f FILE]"
#define MAX_EVENTS 32
#define MAX_CLIENTS 10
#define MAX_HANDLERS MAX_CLIENTS + 1
#define CLIENT_BACKLOG 10
#define MAX_RECORD_SIZE 4096
#define DATA_FILE "/var/tmp/aesdsocketdata"
#define ADDRESS "0.0.0.0"
#define PORT "9000"
#endif // Default values

#ifndef log_errno
#define log_errno(msg, ...)                                                    \
  syslog(LOG_ERR, "ERR(%d): " msg, errno, ##__VA_ARGS__)
#define log_error(msg, ...) syslog(LOG_ERR, msg, ##__VA_ARGS__)
#define log_warning(msg, ...) syslog(LOG_WARNING, msg, ##__VA_ARGS__)
#define log_info(msg, ...) syslog(LOG_INFO, msg, ##__VA_ARGS__)
#define log_debug(msg, ...) syslog(LOG_DEBUG, msg, ##__VA_ARGS__)
#endif // Macros

struct Config {
  bool daemon;
  char *address;
  char *port;
  char *data_file;
};

struct EventHandler {
  void (*callback)(struct epoll_event *event);
  void *data;
  int fd;
};

struct EventLoop {
  int _epfd;
  struct EventHandler *_handlers[MAX_HANDLERS];
  int _nhandlers;
};

struct AppState {
  struct Config *config;
  struct EventLoop *loop;
  int server_sock_fd;
  bool *stop;
};

struct EventLoop *create_loop();
bool destroy_loop(struct EventLoop *loop);
bool run(struct EventLoop *loop, bool *stop);
bool register_event(struct EventLoop *loop, int fd, uint32_t events,
                                    void (*cb)(struct epoll_event *event),
                                    void *cb_data);
bool deregister_event(struct EventLoop *loop, struct EventHandler *handler);

int create_server(const char *address, const char *port);
void handle_client_connection(struct epoll_event *event);

bool recv_data(FILE *stream, char **buffer);
bool send_data(FILE *stream, char *data);
void handle_client_msg(struct epoll_event *event);

bool write_records(const char *filename, const char *data);
bool read_records(const char *filename, bool (*cb)(char *data, void *),
                  void *cb_data);
