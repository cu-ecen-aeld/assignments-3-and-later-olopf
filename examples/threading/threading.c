#include "threading.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Optional: use these functions to add debug or error prints to your
// application
#define DEBUG_LOG(msg,...) printf("DEBUG: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg, ...) printf("ERROR: " msg "\n", ##__VA_ARGS__)

void *threadfunc(void *thread_param) {
  struct thread_data *args = (struct thread_data *)thread_param;
  args->thread_complete_success = false;

  DEBUG_LOG("Waiting %d ms to acquire lock", args->wait_to_obtain_ms);
  usleep(1000 * args->wait_to_obtain_ms);
  DEBUG_LOG("Acquiring lock");
  if (pthread_mutex_lock(args->mutex) != 0) {
    ERROR_LOG("Failed to acquire log");
    return thread_param;
  };

  DEBUG_LOG("Waiting %d ms to release lock", args->wait_to_release_ms);
  usleep(1000 * args->wait_to_release_ms);
  DEBUG_LOG("Releasing lock");
  if (pthread_mutex_unlock(args->mutex) != 0) {
    ERROR_LOG("Failed to release log");
    return thread_param;
  };

  DEBUG_LOG("Thread completed");
  args->thread_complete_success = true;
  return thread_param;
}

bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,
                                  int wait_to_obtain_ms,
                                  int wait_to_release_ms) {
  DEBUG_LOG("Setting up thread data");
  struct thread_data *args = malloc(sizeof(struct thread_data));
  *args = (struct thread_data){.mutex = mutex,
                               .wait_to_obtain_ms = wait_to_obtain_ms,
                               .wait_to_release_ms = wait_to_release_ms};

  DEBUG_LOG("Creating thread");
  int rc = pthread_create(thread, NULL, threadfunc, args);
  if (rc != 0) {
    ERROR_LOG("Failed to create thread");
    return false;
  }

  DEBUG_LOG("Thread created");
  return true;
}
