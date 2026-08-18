#include "utils.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static bool verbose = false;

void setLogVerbose(bool log) {
    verbose = true;
}

void marktexLog(LogLevel level, const char* format, ...) {
    if(!verbose && level == LOG_VERBOSE_ONLY) return;
    const char* prefix;

    switch (level) {
        case LOG_VERBOSE_ONLY: prefix = "Trace: ";   break;
        case LOG_INFO:    prefix = "Info: ";    break;
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

