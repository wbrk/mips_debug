#include "measure.h"

#include "log.h"

#include <errno.h>
#include <stdio.h>

#define ARRAY_SIZE 16

static struct timespec start[ARRAY_SIZE];
static int start_ptr = 0;

static int is_log_available();

// public

struct timespec *measure_diff(const struct timespec *start,
  struct timespec *end, struct timespec *result) {
    result->tv_sec  = end->tv_sec - start->tv_sec;
    result->tv_nsec = end->tv_nsec - start->tv_nsec;

    if (result->tv_nsec < 0) {
        result->tv_nsec += 1000000000;
        result->tv_sec  -= 1;
    }

    return result;
}

void measure_start() {
    if (start_ptr >= ARRAY_SIZE) {
        printf("DM: measure_start(): can't store time: too much calls!\n");
        return;
    }

    if (clock_gettime(CLOCK_MONOTONIC, &start[start_ptr++])) {
        perror("DM: measure_start(): clock_gettime()");
    }
}

struct timespec *measure_get(struct timespec *diff) {
    if (clock_gettime(CLOCK_MONOTONIC, diff)) {
        perror("DM: measure_get(): clock_gettime()");
        return diff;
    }

    if(--start_ptr < 0) {
        start_ptr = 0;
    }

    return measure_diff(&start[start_ptr], diff, diff);
}

void measure_print(const char *comment) {
    struct timespec diff;
    measure_get(&diff);

    if (is_log_available()) {
        LOGD("%s took %ld.%09ld seconds", comment, diff.tv_sec, diff.tv_nsec);
    } else {
        printf("DM: %s took %ld.%09ld seconds\n", comment, diff.tv_sec, diff.tv_nsec);
    }
}

// private

static
int is_log_available() {
    log_sink_t sink;
    log_get_sink(&sink, NULL);

    return log_get_level() != LOG_DISABLED
        && sink != LOG_SINK_UNSPECIFIED;
}
