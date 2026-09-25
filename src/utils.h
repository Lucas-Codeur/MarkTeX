/*
This file is part of MarkTeX which is released under the MIT liscence.
See file LISCENCE or go to https://github.com/Lucas-Codeur/MarkTeX/blob/main/LICENSE for full license details.
*/

#ifndef UTILS_H
#define UTILS_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// String view

typedef struct {
    const char* data;
    int length;
} string_view;

void trim(string_view* view);

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

// Output buffer

typedef struct {
    char* data;
    size_t pos;
    size_t capacity;
} OutputBuffer;

OutputBuffer newOutputBuffer(size_t capacity);

void obWrite(OutputBuffer* buffer, const char* data, size_t size);
void obWriteStr(OutputBuffer* buffer, const char* data);
void obWriteFormat(OutputBuffer* buffer, const char* data, ...);
void obWriteStrView(OutputBuffer* buffer, string_view* view);

bool obEnsureCapacity(OutputBuffer* buffer, size_t required);
void obFlush(OutputBuffer* buffer, FILE* file);

void deleteOutputBuffer(OutputBuffer* buffer);

#endif