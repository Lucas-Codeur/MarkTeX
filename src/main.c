#include "lang/latex.h"
#include "lang/lexer.h"
#include "lang/parser.h"
#include "utils.h"

#include <getopt.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

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
        "  -h, --help           Show this help\n",
        program);
}

bool parseOptions(int argc, char** argv, Options* options) {
    options->input = NULL;
    options->output = NULL;
    options->template = NULL;
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

    if (options->template == NULL) {
        options->template = "default.tex";
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

void printCompiledContent(const char* fileName, char* input, FILE* output) {
    marktexLog(LOG_VERBOSE_ONLY, "--- Compiling %s ---", fileName);

    int64_t start = timestamp_us();

    Lexer lexer = newLexer(input);
    tokenize(&lexer);

    marktexLog(LOG_VERBOSE_ONLY, "Generated %i tokens from input", lexer.tokens.size);

    Parser parser = {.lexer = &lexer, .pos = 0};
    AstNode* parsed = parseDocument(&parser);

    OutputBuffer outBuffer = newOutputBuffer(4096);

    marktexLog(LOG_VERBOSE_ONLY, "Parsed document");

    print(parsed, &outBuffer);

    marktexLog(LOG_VERBOSE_ONLY, "LaTeX generated");

    outputBufferFlush(&outBuffer, output);

    destroyLexer(&lexer);
    destroyNode(parsed);

    deleteOutputBuffer(&outBuffer);

    int64_t stop = timestamp_us();
    int64_t duration = stop - start;

    marktexLog(LOG_INFO, "Successfully compiled %s in %i µs", fileName, duration);
}

int main(int argc, char** argv) {
    Options options;
    
    if (!parseOptions(argc, argv, &options)) {
        printUsage(argv[0]);
        return EXIT_FAILURE;
    }

    if(options.verbose) setLogVerbose(true);

    
    if (access(options.input, F_OK) == -1) {
        marktexLog(LOG_ERROR, "input file (%s) not found", options.input);
        return EXIT_FAILURE;
    }

    if (access(options.template, F_OK) == -1) {
        marktexLog(LOG_ERROR, "template file (%s) not found", options.template);
        return EXIT_FAILURE;
    }

    int64_t start = timestamp_us();

    char* input = readFile(options.input);
    char* template = readFile(options.template);

    FILE* outFile = fopen(options.output, "w");

    const char* contentPlaceholder = "{{MARKTEX_CONTENT}}";

    char* contentPos = strstr(template, contentPlaceholder);
    if (!contentPos) {
        free(input);
        free(template);
        marktexLog(LOG_ERROR, "Could not find {{MARKTEX_CONTENT}} placeholder in the template, aborting");
        return EXIT_FAILURE;
    }

    fputs("% Built by MarkTeX 0.1.0\n", outFile);

    fwrite(template, 1, contentPos - template, outFile);

    printCompiledContent(options.input, input, outFile);

    fputs(contentPos + strlen(contentPlaceholder), outFile);

    fclose(outFile);

    int64_t stop = timestamp_us();
    int64_t duration = stop - start;

    marktexLog(LOG_INFO, "All tasks completed in %i µs", duration);

    free(input);
    free(template);
    return EXIT_SUCCESS;
}