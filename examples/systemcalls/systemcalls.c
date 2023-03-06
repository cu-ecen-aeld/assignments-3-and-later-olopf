#include "systemcalls.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
 */
bool do_system(const char *cmd) {
  int returncode = system(cmd);
  return returncode == 0;
}

/**
 * @param count -The numbers of variables passed to the function. The variables
 * are command to execute. followed by arguments to pass to the command Since
 * exec() does not perform path expansion, the command to execute needs to be an
 * absolute path.
 * @param ... - A list of 1 or more arguments after the @param count argument.
 *   The first is always the full path to the command to execute with execv()
 *   The remaining arguments are a list of arguments to pass to the command in
 * execv()
 * @return true if the command @param ... with arguments @param arguments were
 * executed successfully using the execv() call, false if an error occurred,
 * either in invocation of the fork, waitpid, or execv() command, or if a
 * non-zero return value was returned by the command issued in @param arguments
 * with the specified arguments.
 */

bool do_exec(int count, ...) {
  bool result = false;
  va_list args;
  pid_t pid;
  int status;

  va_start(args, count);
  char *command[count + 1];
  for (int i = 0; i < count; i++) {
    command[i] = va_arg(args, char *);
    printf("Arg[%d]: %s\n", i, command[i]);
  }
  va_end(args);
  command[count] = NULL;

  pid = fork();
  if (pid == -1) {
    perror("Failed to fork process");
    result = false;
  } else if (pid == 0) {
    printf("Executing %s\n", command[0]);
    status = execv(command[0], command);
    perror("Failed to execute command");
    result = false;
  } else {
    printf("Forked process with pid %d\n", pid);
    if (waitpid(pid, &status, 0) == -1) {
      perror("Failed waiting on forked process");
      result = false;
    } else if (WIFEXITED(status)) {
      printf("Process %s (%d) completed with status %d\n", command[0], pid,
             WEXITSTATUS(status));
      result = WEXITSTATUS(status) == 0;
    } else {
      perror("Forked process did not exit normally");
      result = false;
    }
  }
  printf("do_exec result: %i\n", result);
  return result;
}

/**
 * @param outputfile - The full path to the file to write with command output.
 *   This file will be closed at completion of the function call.
 * All other parameters, see do_exec above
 */
bool do_exec_redirect(const char *outputfile, int count, ...) {
  bool result = false;
  va_list args;
  pid_t pid;
  int status;
  int fd;

  va_start(args, count);
  char *command[count + 1];
  for (int i = 0; i < count; i++) {
    command[i] = va_arg(args, char *);
    printf("Arg[%d]: %s\n", i, command[i]);
  }
  va_end(args);
  command[count] = NULL;

  fd = open(outputfile, O_WRONLY | O_TRUNC | O_CREAT, 0644);
  if (fd < 0) {
    perror("Failed to open file");
    return false;
  }

  pid = fork();
  if (pid == -1) {
    perror("Failed to fork process");
    result = false;
  } else if (pid == 0) {
    printf("Executing %s\n", command[0]);
    if (dup2(fd, 1) < 0) {
      perror("Failed to redirect stdout to file");
    } else {
      status = execv(command[0], command);
      perror("Failed to execute command");
    }
    result = false;
  } else if (pid > 0) {
    printf("Forked process with pid %d\n", pid);
    if (waitpid(pid, &status, 0) == -1) {
      perror("Failed waiting on forked process");
      result = false;
    } else if (WIFEXITED(status)) {
      printf("Process %s (%d) completed with status %d\n", command[0], pid,
             WEXITSTATUS(status));
      result = WEXITSTATUS(status) == 0;
    } else {
      perror("Forked process did not exit normally");
      result = false;
    }
  }
  close(fd);
  printf("do_exec_redirect result: %i\n", result);
  return result;
}
