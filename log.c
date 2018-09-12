#include "log.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <syslog.h>
#include <sys/time.h>

static struct {
    const char *ident;
    int level_mask;
    FILE *file;
    log_sink_t sink;
} Config = { "?", LOG_DISABLED, NULL, LOG_SINK_UNSPECIFIED };

static const char *level_labels[] = {
    "DEBUG", "INFO", "WARN", "ERROR"
};

static pthread_mutex_t cfg_mtx = PTHREAD_MUTEX_INITIALIZER;

static void lock();
static void unlock();

static void log_to_file(int level, const char *file, int line, const char *fmt,
  va_list ap);

static void log_to_syslog(int level, const char *file, int line, const char *fmt,
  va_list ap);

// public

void log_get_sink(log_sink_t *sink, FILE **file) {
    lock();

    if (sink) {
        *sink = Config.sink;
    }

    if (file) {
        *file = Config.file;
    }

    unlock();
}

int log_get_level() {
    lock();
    int mask = Config.level_mask;
    unlock();

    return mask;
}

void log_set_sink(log_sink_t sink, FILE *file) {
    lock();

    if (sink == LOG_SINK_SYSLOG && Config.sink != LOG_SINK_SYSLOG) {
        openlog(Config.ident, LOG_CONS | LOG_PID, LOG_USER);
    } else if (sink != LOG_SINK_SYSLOG && Config.sink == LOG_SINK_SYSLOG) {
        closelog();
    }

    Config.sink = sink;
    Config.file = file;

    unlock();
}

void log_set_level(int mask) {
    lock();
    Config.level_mask = mask;
    unlock();
}

void log_set_ident(const char *ident) {
    lock();
    Config.ident = ident;
    unlock();
}

void log_log(int level, const char *file, int line, const char *fmt, ...) {
    lock();

    if (Config.sink == LOG_SINK_UNSPECIFIED
        || Config.level_mask == LOG_DISABLED
        || !(Config.level_mask & level)) {
        unlock();
        return;
    }

    if (Config.sink == LOG_SINK_FILE && !Config.file) {
        unlock();
        return;
    }

    va_list args;
    va_start(args, fmt);

    switch (Config.sink) {
    case LOG_SINK_FILE:
        log_to_file(level, file, line, fmt, args);
        break;

    case LOG_SINK_SYSLOG:
        log_to_syslog(level, file, line, fmt, args);
        break;

    default: break;
    }

    va_end(args);
    unlock();
}

// private

static
void get_time(char *buf, int len) {
    time_t t = time(NULL);
    char tmp[32];
    int rv = strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", localtime(&t));
    if (!rv) {
        tmp[0] = '\0';
    }

    // get milliseconds portion
    struct timeval usec_time;
    gettimeofday(&usec_time, NULL);
    int mseconds = usec_time.tv_usec / 1000;

    // put parts together to have "2007-01-01 00:00:00.000"
    snprintf(buf, len, "%s.%03d", tmp, mseconds);
}

static
const char *get_level_label(int level) {
    return level == LOG_LEVEL_DEBUG ? level_labels[0]
         : level == LOG_LEVEL_INFO  ? level_labels[1]
         : level == LOG_LEVEL_WARN  ? level_labels[2]
         : level == LOG_LEVEL_ERROR ? level_labels[3]
                                    : level_labels[0];
}

static
void log_to_syslog(int level, const char *file, int line, const char *fmt,
  va_list ap) {
    int priority = level == LOG_LEVEL_DEBUG ? LOG_DEBUG
                 : level == LOG_LEVEL_INFO  ? LOG_INFO
                 : level == LOG_LEVEL_WARN  ? LOG_WARNING
                 : level == LOG_LEVEL_ERROR ? LOG_ERR
                                            : LOG_DEBUG;

    char prefix[64];
    snprintf(prefix, sizeof(prefix), "%s:%d", file, line);

    char msg[256];
    vsnprintf(msg, sizeof(msg), fmt, ap);

    syslog(priority, "%s: %s", prefix, msg);
}

static
void log_to_file(int level, const char *file, int line, const char *fmt,
  va_list ap) {
    char time_str[32];
    get_time(time_str, sizeof(time_str));

    fprintf(Config.file, "%s [%-5s] [%s] %s:%d: ",
    time_str, get_level_label(level), Config.ident, file, line);

    vfprintf(Config.file, fmt, ap);
    fprintf(Config.file, "\n");
}

static
void lock() {
    pthread_mutex_lock(&cfg_mtx);
}

static
void unlock() {
    pthread_mutex_unlock(&cfg_mtx);
}
