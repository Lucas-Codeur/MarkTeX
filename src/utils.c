#include "utils.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static bool verbose = false;

void setLogVerbose(bool log) {
    verbose = true;
}

OutputBuffer newOutputBuffer(size_t capacity) {
    OutputBuffer buffer;
    buffer.data = NULL;
    buffer.pos = 0;
    buffer.capacity = 0;

    if (capacity == 0)
        capacity = 64;

    char* data = malloc(capacity * sizeof(char));
    if(!data) {
        marktexLog(LOG_ERROR, "Allocation of %zu bytes for output buffer failed", sizeof(char) * capacity);
        return buffer;
    }

    buffer.data = data;
    buffer.capacity = capacity;

    return buffer;
}

void outputBufferWrite(OutputBuffer* buffer, const char* data, size_t size) {
    if(!data) return;

    if(!outputBufferEnsureCapacity(buffer, buffer->pos + size)) return;

    memcpy(buffer->data + buffer->pos, data, size);
    buffer->pos += size;
}

void outputBufferWriteStr(OutputBuffer* buffer, const char* str) {
    if (!str) return;

    outputBufferWrite(buffer, str, strlen(str) * sizeof(char));
}

void outputBufferWriteFormat(
    OutputBuffer* buffer,
    const char* format,
    ...
) {
    va_list args;

    va_start(args, format);

    va_list argsCopy;
    va_copy(argsCopy, args);

    int required = vsnprintf(
        NULL,
        0,
        format,
        argsCopy
    );

    va_end(argsCopy);

    if (required < 0) {
        va_end(args);
        marktexLog(LOG_ERROR, "Formatting output failed");
        return;
    }

    if(!outputBufferEnsureCapacity(buffer, (size_t)required)) return;

    vsnprintf(
        buffer->data + buffer->pos,
        required + 1,
        format,
        args
    );

    buffer->pos += required;

    va_end(args);
}

void outputBufferWriteStrView(OutputBuffer* buffer, string_view* view) {
    outputBufferWrite(buffer, view->data, view->length * sizeof(char));
}

void outputBufferFlush(OutputBuffer* buffer, FILE* file) {
    fwrite(buffer->data, buffer->pos, 1, file);
}

void deleteOutputBuffer(OutputBuffer* buffer) {
    free(buffer->data);
    buffer->data = NULL;
    buffer->pos = 0;
    buffer->capacity = 0;
}

bool outputBufferEnsureCapacity(OutputBuffer* buffer, size_t required)
{
    if (required <= buffer->capacity)
        return true;

    size_t newCapacity = buffer->capacity;

    if (newCapacity == 0) newCapacity = 64;

    while (newCapacity < required) {
        newCapacity *= 2;
    }

    char* newData = realloc(buffer->data, newCapacity);

    if (!newData) {
        marktexLog(
            LOG_ERROR,
            "Reallocation of %zu bytes for output buffer failed",
            newCapacity
        );
        return false;
    }

    buffer->data = newData;
    buffer->capacity = newCapacity;

    return true;
}

void marktexLog(LogLevel level, const char* format, ...) {
    if(!verbose && level == LOG_VERBOSE_ONLY) return;
    const char* prefix;

    switch (level) {
        case LOG_VERBOSE_ONLY: prefix = "";   break;
        case LOG_INFO:    prefix = "";    break;
        case LOG_WARNING: prefix = "\x1b[33mWarning: "; break;
        case LOG_ERROR:   prefix = "\x1b[31mError: ";   break;
        default:          prefix = "";     break;
    }

    printf("%s", prefix);

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    puts("\x1b[0m");
}


int64_t timestamp_us() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    return (int64_t)ts.tv_sec * 1000000
         + ts.tv_nsec / 1000;
}

void trim(string_view* view) {
    while (view->length > 0 && (view->data[0] == ' ' || view->data[0] == '\t')) {
        view->data++;
        view->length--;
    }

    while (view->length > 0 && view->data[view->length - 1] == ' ' ||
           view->data[view->length - 1] == '\t') {
        view->length--;
    }
}

// Warning: your duty to free the buffer
char* readFile(const char* path) {
    FILE* file = fopen(path, "r");
    if (!file) {
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    marktexLog(LOG_VERBOSE_ONLY, "Reading %i bytes from %s", size, path);

    char* buf = malloc(size * sizeof(char) + 1);
    fread(buf, 1, size, file);
    buf[size] = '\0';
    fclose(file);

    return buf;
}

