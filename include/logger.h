#ifndef PET_LOGGER_H
#define PET_LOGGER_H

#ifdef PET_USE_EWTS
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// Using EWTS
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

#include "ewts/module_constants.h"
#include "ewts/logger.h"
#include "ewts/log_levels.h"

#define PET_MODULE_ID EWTS_ID_PET

#define Log(level, ...) EwtsLogModule(PET_MODULE_ID, (level), __VA_ARGS__)
#define LOG(level, ...) EwtsLogModule(PET_MODULE_ID, (level), __VA_ARGS__)
#define GetLogLevel() EwtsGetLogLevelModule(PET_MODULE_ID)
#define IsLoggingEnabled() EwtsIsLoggingEnabledModule(PET_MODULE_ID)

#else

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// Log messages written to STDOUT
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#define PET_MODULE_ID "PET"
typedef enum {
    NOTSET  = 0,
    DEBUG   = 10,
    PERFORM = 15,
    INFO    = 20,
    WARNING = 30,
    SEVERE  = 40,
    FATAL   = 50
} LogLevel;

#define PET_FALLBACK_LOGGING_ENABLED   1
#define PET_FALLBACK_LOG_LEVEL         INFO

static inline bool IsLoggingEnabled(void) {
    return PET_FALLBACK_LOGGING_ENABLED;
}

static inline LogLevel GetLogLevel(void) {
    return PET_FALLBACK_LOG_LEVEL;
}

static inline const char* pet_level_to_string(LogLevel level) {
    switch (level) {
        case DEBUG:   return "DEBUG";
        case PERFORM: return "PERFORM";
        case INFO:    return "INFO";
        case WARNING: return "WARN";
        case SEVERE:  return "ERROR";
        case FATAL:   return "FATAL";
        default:      return "LOG";
    }
}

static inline void pet_utc_timestamp(char* buffer, size_t size) {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    struct tm tm_utc;
    gmtime_r(&tv.tv_sec, &tm_utc);

    char base[32];
    strftime(base, sizeof(base), "%Y-%m-%dT%H:%M:%S", &tm_utc);

    snprintf(buffer, size, "%s.%03ldZ", base, tv.tv_usec / 1000);
}

static inline void pet_strip_trailing_newlines(char* s) {
    if (!s) return;

    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[--len] = '\0';
    }
}

static inline void Log(LogLevel level, const char* fmt, ...) {
    if (!fmt) return;
    if (!IsLoggingEnabled()) return;
    if (level < GetLogLevel()) return;

    char message[4096];

    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    pet_strip_trailing_newlines(message);

    char timestamp[64];
    pet_utc_timestamp(timestamp, sizeof(timestamp));

    char line[8192];
    snprintf(line, sizeof(line), "%s %s %s %s\n",
             timestamp,
             PET_MODULE_ID,
             pet_level_to_string(level),
             message);

    fputs(line, stdout);
    fflush(stdout);
}

#define LOG(level, ...) Log((level), __VA_ARGS__)
#endif
#endif /* PET_LOGGER_H */
