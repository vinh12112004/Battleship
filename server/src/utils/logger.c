#include "utils/logger.h"
#include <windows.h>

static const char* level_strings[] = {
    "DEBUG", "INFO", "WARN", "ERROR"
};

static WORD level_colors[] = {
    7,  // DEBUG - Gray
    10, // INFO - Green
    14, // WARN - Yellow
    12  // ERROR - Red
};

void log_message(log_level_t level, const char* file, int line, const char* fmt, ...) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    
    // Get current time
    time_t now;
    time(&now);
    struct tm* tm_info = localtime(&now);
    char time_buffer[26];
    strftime(time_buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    
    // Set color
    SetConsoleTextAttribute(hConsole, level_colors[level]);
    
    // Print log header
    printf("[%s] [%s] ", time_buffer, level_strings[level]);
    
    // Print message
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    
    printf("\n");
    
    // Reset color
    SetConsoleTextAttribute(hConsole, 7);
}