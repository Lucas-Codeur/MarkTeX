#ifndef UTILS_H
#define UTILS_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

// Time

int64_t timestamp_us();

// Logging

typedef enum {
    LOG_VERBOSE_ONLY,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
} LogLevel;

void marktexLog(LogLevel level, const char* format, ...);

void setLogVerbose(bool log);

// File utils

char* readFile(const char* path);

// String view

typedef struct {
    const char* data;
    int length;
} string_view;

void trim(string_view* view);

#endif