#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#include "aesdsocket.h"

bool write_records(const char *filename, const char *data) {
  log_debug("Writing records to data file '%s'", filename);
  bool status = true;

  FILE *file = fopen(filename, "a");
  if (file == NULL) {
    log_errno("Failed to open %s for recording", filename);
    return false;
  }

  if (fputs(data, file) == EOF) {
    log_error("Failed to write data to %s", filename);
    status = false;
  }

  if (fflush(file) == EOF) {
    log_errno("Failed to flush data");
    status = false;
  } else {
    log_debug("Wrote data to %s", filename);
  }

  if (fclose(file) == EOF) {
    log_errno("Failed to close %s", filename);
    status = false;
  }

  return status;
}

bool read_records(const char *filename, bool (*cb)(char *, void *),
                  void *cb_data) {
  log_debug("Reading records from %s", filename);

  char *buffer = NULL;
  size_t len = MAX_RECORD_SIZE;
  ssize_t nread;
  int status;
  FILE *file = fopen(filename, "r");
  if (file == NULL) {
    log_errno("Failed to open %s for recording", filename);
    return false;
  }

  while (feof(file) == 0) {
    nread = getline(&buffer, &len, file);
    if (nread < 0) {
      if (feof(file) != 0) break;
      log_errno("Failed reading data from %s", filename);
      return false;
    }
    log_debug("Passing record to handler");
    if (cb(buffer, cb_data) == false) {
      log_debug("Handler finished reading");
      break;
    }
  }
  free(buffer);
  buffer = NULL;

  if (fclose(file) == EOF) {
    log_errno("Failed to close %s", filename);
    return false;
  }

  log_debug("Read records from %s", filename);
  return true;
}
