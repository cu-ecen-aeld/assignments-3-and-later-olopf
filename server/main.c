#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <syslog.h>
#include <unistd.h>

#include "aesdsocket.h"

bool STOP = false;
int SIGNAL = -1;

static void signal_handler(int signal);
bool parse_cli(int argc, char *argv[], struct Config *config);
pid_t daemonise(void);
int _main(struct AppState *state);

int main(int argc, char *argv[]) {
  bool status;
  pid_t pid;
  struct Config *config = &(struct Config){.daemon = false,
                                           .address = ADDRESS,
                                           .port = PORT,
                                           .data_file = DATA_FILE};
  struct AppState *state = &(struct AppState){
      .config = config,
      .server_sock_fd = -1,
      .stop = &STOP,
  };

  openlog(NULL, LOG_PERROR | LOG_PID, LOG_USER);

  log_debug("Parsing CLI");
  if (parse_cli(argc, argv, config) == false) {
    fputs("Usage: " USAGE "\n", stdout);
    return -1;
  }

  log_info("Creating server socket");
  state->server_sock_fd = create_server(config->address, config->port);
  if (state->server_sock_fd == -1) {
    return -1;
  }

  pid = 0;
  if (config->daemon == true) {
    log_info("Forking process");
    pid = daemonise();
  }

  if (pid == -1) {
    shutdown(state->server_sock_fd, SHUT_RDWR);
    status = false;
  } else if (pid > 0) {
    log_info("Server started with pid %d", pid);
    status = true;
  } else {
    status = _main(state);
    log_info("Shutting down server socket");
    shutdown(state->server_sock_fd, SHUT_RDWR);
  }

  if (pid < 1) {
    log_info("Deleting data file - %d", pid);
    if (remove(config->data_file) == -1) {
      log_errno("Failed to delete data file '%s'", config->data_file);
      status = false;
    }
  }

  log_info("Closing log");
  closelog();

  return status == true ? 0 : -1;
}

int _main(struct AppState *state) {
  struct sigaction action;
  bool status;
  log_info("Creating event loop");
  state->loop = create_loop();
  if (state->loop == NULL) {
    return -1;
  }

  log_info("Registering signal handlers");
  memset(&action, 0, sizeof(struct sigaction));
  action.sa_handler = *signal_handler;
  if (sigaction(SIGTERM, &action, NULL) == -1) {
    log_errno("Failed to register signal handler for SIGTERM");
    return -1;
  }
  if (sigaction(SIGINT, &action, NULL) == -1) {
    log_errno("Failed to register signal handler for SIGINT");
    return -1;
  }

  log_info("Registering server with event loop");
  status = register_event(state->loop, state->server_sock_fd, EPOLLIN,
                          *handle_client_connection, state);
  if (status == true) {
    log_info("Looping");
    status = run(state->loop, state->stop);
    if (SIGNAL != -1)
      log_info("Caught signal, exiting");
  }
  log_info("Stopping event loop");
  destroy_loop(state->loop);

  return status;
}

bool parse_cli(int argc, char *argv[], struct Config *config) {
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-d") == 0) {
      config->daemon = true;
    } else if (strcmp(argv[i], "-i") == 0) {
      i += 1;
      if (i < argc) {
        config->address = argv[i];
      } else {
        fputs("-i missing argument\n", stderr);
        return false;
      }
    } else if (strcmp(argv[i], "-p") == 0) {
      i += 1;
      if (i < argc) {
        config->port = argv[i];
      } else {
        fputs("-p missing argument\n", stderr);
        return false;
      }
    } else if (strcmp(argv[i], "-f") == 0) {
      i += 1;
      if (i < argc) {
        config->data_file = argv[i];
      } else {
        fputs("-f missing argument\n", stderr);
        return false;
      }
    } else {
      fprintf(stderr, "Unrecognized option '%s'\n", argv[i]);
      return false;
    }
  }
  return true;
}

pid_t daemonise() {
  pid_t pid;
  int fd;

  pid = fork();
  if (pid != 0) {
    return pid;
  }

  if (setsid() == -1) {
    log_errno("Failed to create new session");
    return -1;
  }

  if (chdir("/") == -1) {
    log_errno("Failed to change directory");
    return -1;
  }

  fd = open("/dev/null", O_RDWR);
  if (fd == -1) {
    log_errno("Failed to open /dev/null");
    return -1;
  }
  for (int i = 0; i < 3; i++) {
    if (close(i) == -1) {
      log_errno("Failed to close fd %d", i);
      return -1;
    }
    if (dup2(fd, i) == -1) {
      log_errno("Failed to redirect fd %d to /dev/null", i);
      return -1;
    }
  }

  return pid;
}

static void signal_handler(int signal) {
  char *alert = "!!! Stop signal received !!!\n";
  write(1, alert, strlen(alert));
  STOP = true;
  SIGNAL = signal;
}
