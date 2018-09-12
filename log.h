#ifndef LOG_H_INCLUDED
#define LOG_H_INCLUDED

#include <stdio.h>
#include <stdarg.h>

/*
    This library is thread-safe, but it's achieved by rather simple approach:
    entire logging function is guarded with a mutex. Certainly, this approach
    provides suboptimal performance, but for now it's okay. If someday logging
    will become bottleneck, consider architecture where most important bits
    of code are executed in a dedicated logging thread. Another approach is to
    allow to eliminate logging in compile time:

    #ifdef NDEBUG
    #define LOGD(...)
    #else
    #define LOGD(...) log_log(...)
    #endif

    Before logging messages you probably want to configure logger with
    log_set_*() functions. Logging is disabled by default.
*/

enum {
    LOG_DISABLED = 0,
    LOG_LEVEL_DEBUG = 1 << 0,
    LOG_LEVEL_INFO  = 1 << 1,
    LOG_LEVEL_WARN  = 1 << 2,
    LOG_LEVEL_ERROR = 1 << 3,
};

typedef enum {
    LOG_SINK_UNSPECIFIED,
    LOG_SINK_FILE,
    LOG_SINK_SYSLOG,
} log_sink_t;

/*
    Print message to log. Use them like printf(): LOGD("fmt", args).
    If you're trying to print message that exceeds 255 characters to syslog,
    it will be truncated.
*/
#define LOGD(...) log_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOGI(...) log_log(LOG_LEVEL_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define LOGW(...) log_log(LOG_LEVEL_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define LOGE(...) log_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__)

#define LOG(log_level, ...) log_log(log_level, __FILE__, __LINE__, __VA_ARGS__)

/*
    Specify where to print log messages. You can print your messages to a file
    or to a syslog, but not to both.

    If 'sink' is LOG_SINK_SYSLOG, value of 'file' is not used, so you may pass
    NULL. If 'sink' is LOG_SINK_FILE, you should pass valid pointer to FILE or
    otherwise no logging will be done. If 'sink' is LOG_SINK_SYSLOG, logging
    is disabled.
*/
void log_set_sink(log_sink_t sink, FILE *file);

/*
    Set log level. Logger will print message only if corresponding bit in 'mask'
    is set.

    'mask' is a bitmask. Allowed flags are any of LOG_LEVEL_*. 'mask' can contain
    any combination of these flags, like this:
    <code>
        mask = LOG_DEBUG | LOG_INFO | LOG_WARN;
    </code>

    Pass LOG_DISABLED to disable logging.
*/
void log_set_level(int mask);

/*
    Set program identifier that prepends every log message. Usually you want
    to set it to program name. When printing to syslog, you have to set ident
    before calling log_set_sink() abd trying to log something.

    'ident' must be a valid string (non-null).
*/
void log_set_ident(const char *ident);

/*
    Get current settings of log sink. If you don't need some value, just pass
    NULL as a corresponding parameter. If 'sink' != LOG_SINK_FILE, 'file' will be
    set to NULL.
*/
void log_get_sink(log_sink_t *sink, FILE **file);

/*
    Get current log level mask.
*/
int log_get_level();

/*
    Low-level logging call.
*/
void log_log(int level, const char *file, int line, const char *fmt, ...);

#endif // LOG_H_INCLUDED
