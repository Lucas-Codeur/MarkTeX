#include "lang/latex.h"
#include "lang/lexer.h"
#include "lang/parser.h"

#include <bits/getopt_core.h>
#include <getopt.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    const char* input;
    const char* output;
    const char* template;
    bool verbose;
    bool watch;
} Options;

void printUsage(const char* program) {
    printf(
        "Usage: %s -i <input> -o <output> [--verbose]\n"
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

// Warning: your duty to free the buffer
char* readFile(const char* path) {
    FILE* file = fopen(path, "r");
    if (!file) {
        printf("Could not open file\n");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);

#ifdef FS_DEBUG
    printf("File size: %i\n", size);
#endif

    char* buf = malloc(size * sizeof(char) + 1);
    fread(buf, 1, size, file);
    buf[size] = '\0';
    fclose(file);

    return buf;
}

void printCompiledContent(char* input, FILE* output) {
    Lexer lexer = newLexer(input);
    tokenize(&lexer);

    Parser parser = {.lexer = &lexer, .pos = 0};

    AstNode* parsed = parseDocument(&parser);

    print(parsed, output);

    destroyLexer(&lexer);
    destroyNode(parsed);
}

int main(int argc, char** argv) {
    Options options;
    if (!parseOptions(argc, argv, &options)) {
        printUsage(argv[0]);
        return EXIT_FAILURE;
    }

    if (access(options.input, F_OK) == -1) {
        printf("Error: input file (%s) not found\n", options.input);
        return EXIT_FAILURE;
    }

    if (access(options.template, F_OK) == -1) {
        printf("Error: template file (%s) not found\n", options.template);
        return EXIT_FAILURE;
    }

    char* input = readFile(options.input);
    char* template = readFile(options.template);

    FILE* outFile = fopen(options.output, "w");

    const char* contentPlaceholder = "{{MARKTEX_CONTENT}}";

    char* contentPos = strstr(template, contentPlaceholder);
    if (!contentPos) {
        free(input);
        free(template);
        printf("Could not find {{MARKTEX_CONTENT}} placeholder in the template\n");
        return EXIT_FAILURE;
    }

    fputs("% Built by MarkTeX 0.1.0\n", outFile);

    fwrite(template, 1, contentPos - template, outFile);

    printCompiledContent(input, outFile);

    fputs(contentPos + strlen(contentPlaceholder), outFile);

    fclose(outFile);

    free(input);
    free(template);
    return EXIT_SUCCESS;
}