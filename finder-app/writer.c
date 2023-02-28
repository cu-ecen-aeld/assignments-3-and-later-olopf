#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syslog.h>

const char usage[] = "writer PATH CONTENT";

int main(int argc, char *argv[]) {
  FILE *file;
  openlog(NULL, LOG_PERROR | LOG_PID, LOG_USER);
  if (argc != 3) {
    syslog(LOG_ERR, "Invalid number of arguments: %d", argc);
    printf("Usage: %s\n", usage);
    return 1;
  }

  file = fopen(argv[1], "w");
  if (!file) {
    syslog(LOG_ERR, "ERR(%d): Failed to open %s for writing", errno, argv[1]);
    return 1;
  }

  syslog(LOG_USER, "Writing %s to %s", argv[2], argv[1]);
  // if (!fwrite(argv[2], 1, sizeof(argv[2]), file)) {
  if (fputs(argv[2], file) == EOF) {
    syslog(LOG_ERR, "ERR(%d): Failed to write %s to %s", errno, argv[2],
           argv[1]);
    return 1;
  } else if (fclose(file)) {
    syslog(LOG_ERR, "ERR(%d): Failed to close %s", errno, argv[1]);
    return 1;
  } else {
    return 0;
  }
}
