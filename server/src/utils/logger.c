#include "utils/logger.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

static const char* level_strings[] = {
    "DEBUG", "INFO", "WARN", "ERROR"
};

static const char* level_colors[] = {
    "\x1b[90m", // DEBUG - Gray
    "\x1b[32m", // INFO  - Green
    "\x1b[33m", // WARN  - Yellow
    "\x1b[31m"  // ERROR - Red
};

void log_message(log_level_t level, const char* file, int line, const char* fmt, ...) {
    // Get current time
    time_t now;
    time(&now);
    struct tm* tm_info = localtime(&now);
    char time_buffer[26];
    strftime(time_buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);

    // Print color + header
    printf("%s[%s] [%s] ", level_colors[level], time_buffer, level_strings[level]);

    // Print message
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    // Reset color
    printf("\x1b[0m\n");
}
