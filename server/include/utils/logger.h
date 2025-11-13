#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <time.h>
#include <stdarg.h>

typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} log_level_t;

void log_message(log_level_t level, const char* file, int line, const char* fmt, ...);

#define log_debug(...) log_message(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define log_info(...)  log_message(LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(...)  log_message(LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...) log_message(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)

#endif // LOGGER_H