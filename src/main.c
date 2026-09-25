/*
This file is part of MarkTeX which is released under the MIT liscence.
See file LISCENCE or go to https://github.com/Lucas-Codeur/MarkTeX/blob/main/LICENSE for full license details.
*/

#include "lang/latex.h"
#include "lang/lexer.h"
#include "lang/parser.h"
#include "utils.h"

#include <asm-generic/errno-base.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/inotify.h>
#include <limits.h>

#define EVENT_SIZE  (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + NAME_MAX + 1))

#define DEFAULT_TEMPLATE NULL

extern const char default_template[];

static volatile sig_atomic_t running = 1;

typedef struct {
    const char* input;
    const char* output;
    const char* template;
    bool verbose;
    bool watch;
} Options;

void printUsage(const char* program) {
    printf(
        "Usage: %s -i <input> -o <output> [--verbose] [--watch]\n"
        "\n"
        "Options:\n"
        "  -i, --input FILE     Input Markdown file\n"
        "  -o, --output FILE    Output LaTeX file\n"
        "  -t, --template FILE  Template file\n"
        "      --verbose        Enable verbose output\n"
        "  -h, --help           Show this help\n"
        "  -w, --watch          Automatically recompile upon update\n",
        program);
}

bool parseOptions(int argc, char** argv, Options* options) {
    options->input = NULL;
    options->output = NULL;
    options->template = DEFAULT_TEMPLATE;
    options->verbose = false;
    options->watch = false;

    static struct option longOptions[] = {
        {"input",    required_argument, 0, 'i'},
        {"output",   required_argument, 0, 'o'},
        {"template", required_argument, 0, 't'},
        {"watch",    no_argument,       0, 'w'},
        {"verbose",  no_argument,       0, 'v'},
        {"help",     no_argument,       0, 'h'}
    };

    int option;
    while ((option = getopt_long(argc, argv, "i:o:t:wvh", longOptions, NULL)) != -1) {
        switch (option) {
        case 'i':
            options->input = optarg;
            break;

        case 'o':
            options->output = optarg;
            break;

        case 't':
            options->template = optarg;
            break;

        case 'w':
            options->watch = true;
            break;

        case 'v':
            options->verbose = true;
            break;

        case 'h':
            printUsage(argv[0]);
            exit(EXIT_SUCCESS);
        case '?':
            return false;
        }
    }

    if (options->input == NULL) {
        fprintf(stderr, "Error: no input file specified\n");
        return 0;
    }

    if (options->output == NULL) {
        fprintf(stderr, "Error: no output file specified\n");
        return 0;
    }

    return true;
}

bool compile(const char* inputPath, const char* outputPath, char* template) {
    if (access(inputPath, F_OK) == -1) {
        marktexLog(LOG_ERROR, "input file (%s) not found", inputPath);
        return false;
    }

    FILE* outFile = fopen(outputPath, "w");
    if(!outFile) {
        marktexLog(LOG_ERROR, "could not access the ouput file (%s)", outputPath);
        return false;
    }

    char* input = readFile(inputPath);

    marktexLog(LOG_VERBOSE_ONLY, "--- Compiling %s ---", inputPath);

    const char* contentPlaceholder = "{{MARKTEX_CONTENT}}";

    char* contentPos = strstr(template, contentPlaceholder);
    if (!contentPos) {
        free(input);
        fclose(outFile);

        marktexLog(LOG_ERROR, "Could not find {{MARKTEX_CONTENT}} placeholder in the template, aborting...");
        return EXIT_FAILURE;
    }

    fputs("% Built by MarkTeX 0.1.0\n", outFile);

    fwrite(template, 1, contentPos - template, outFile);

    /*
    * ##################################
    */

    int64_t start = timestamp_us();

    Lexer lexer = newLexer(input);
    tokenize(&lexer);

    marktexLog(LOG_VERBOSE_ONLY, "Generated %i tokens from input.", lexer.tokens.size);

    Parser parser = {.lexer = &lexer, .pos = 0, .interrupted = false};
    AstNode* parsed = parseDocument(&parser);

    if(parser.interrupted || parsed == NULL) {
        marktexLog(LOG_ERROR, "Parser error encountered while processing %s, please check your syntax.", inputPath);
        destroyLexer(&lexer);
        destroyNode(parsed);
        free(input);
        fclose(outFile);
        return EXIT_FAILURE;
    }

    OutputBuffer outBuffer = newOutputBuffer(4096);

    marktexLog(LOG_VERBOSE_ONLY, "Parsed document");

    print(parsed, &outBuffer);

    marktexLog(LOG_VERBOSE_ONLY, "LaTeX generated");

    obFlush(&outBuffer, outFile);

    destroyLexer(&lexer);
    destroyNode(parsed);

    deleteOutputBuffer(&outBuffer);

    int64_t stop = timestamp_us();
    int64_t duration = stop - start;

    marktexLog(LOG_INFO, "Successfully compiled %s in %i µs", inputPath, duration);

    /*
    * ##################################
    */

    fputs(contentPos + strlen(contentPlaceholder), outFile);

    free(input);
    fclose(outFile);
    
    return true;
}

int watchCompile(const char* inputPath, const char* outputPath, char* template) {
    marktexLog(LOG_INFO, "Watching %s, waiting for changes...", inputPath);
    char path[PATH_MAX];
    char filename[NAME_MAX];

    strncpy(path, inputPath, sizeof(path));
    path[sizeof(path) - 1] = '\0';

    char *slash = strrchr(path, '/');

    if (slash) {
        strcpy(filename, slash + 1);

        if (slash == path)
            slash[1] = '\0';
        else
            *slash = '\0';
    } else {
        strcpy(filename, path);
        strcpy(path, ".");
    }

    int fd = inotify_init1(IN_CLOEXEC);
    if(fd == -1) {
        marktexLog(LOG_ERROR, "could not initialize inotify");
        return EXIT_FAILURE;
    }

    int wd = inotify_add_watch(fd, path, IN_MODIFY | IN_CLOSE_WRITE | IN_MOVED_FROM | IN_MOVED_TO | IN_DELETE);
    if(wd == -1) {
        marktexLog(LOG_ERROR, "cannot create watch on file %s", inputPath);
        return EXIT_FAILURE;
    }

    if (wd == -1) {
        perror("inotify_add_watch");
        close(fd);
        return EXIT_FAILURE;
    }

    char buffer[EVENT_BUF_LEN];
    while(running) {
        ssize_t length = read(fd, buffer, sizeof(buffer));
        if(length == -1) {
            if(errno == EINTR) continue;
            marktexLog(LOG_ERROR, "could not read from inotify buffer");
            break;
        }

        for(char *ptr = buffer; ptr < buffer + length;) {
            struct inotify_event* event = (struct inotify_event*) ptr;

            if(event->len > 0 && strcmp(event->name, filename) == 0) {
                if(event->mask & IN_CLOSE_WRITE) {
                    compile(inputPath, outputPath, template);
                }

                if (event->mask & IN_MOVED_FROM) {
                    running = false;
                }

                if (event->mask & IN_MOVED_TO) {
                    compile(inputPath, outputPath, template);
                }
                
                if (event->mask & IN_DELETE) {
                    running = false;
                }
            }

            ptr += sizeof(struct inotify_event) + event->len;
        }
    }
    
    inotify_rm_watch(fd, wd);
    close(fd);

    marktexLog(LOG_VERBOSE_ONLY, "Watch mode stopped");
    return EXIT_SUCCESS;
}

void exitCallback(int signal) {
    (void) signal;
    running = 0;

    marktexLog(LOG_INFO, "Stopping watch and exiting");
}

int main(int argc, char** argv) {
    Options options;
    
    if (!parseOptions(argc, argv, &options)) {
        printUsage(argv[0]);
        return EXIT_FAILURE;
    }

    if(options.verbose) setLogVerbose(true);

    // Todo: make this security work with different relative paths
    if(strcmp(options.input, options.output) == 0) {
        marktexLog(LOG_ERROR, "please specify different input and output files");
        return EXIT_FAILURE;
    }
    
    if (options.template != DEFAULT_TEMPLATE && access(options.template, F_OK) == -1) {
        marktexLog(LOG_ERROR, "template file (%s) not found", options.template);
        return EXIT_FAILURE;
    }

    char* template = NULL;

    if(options.template != DEFAULT_TEMPLATE) {
        template = readFile(options.template);
    } else {
        template = (char*) default_template;
    }

    int status = EXIT_SUCCESS;
    if(options.watch) {
        struct sigaction sa = {0};

        sa.sa_handler = exitCallback;
        sigemptyset(&sa.sa_mask);

        sigaction(SIGINT,  &sa, NULL); // Ctrl+C
        sigaction(SIGTERM, &sa, NULL); // kill <pid>

        status = watchCompile(options.input, options.output, template);
    } else {
        compile(options.input, options.output, template);
    }

    if(options.template != DEFAULT_TEMPLATE) free(template);
    return status;
}