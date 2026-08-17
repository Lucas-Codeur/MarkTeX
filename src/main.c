#include "latex.h"
#include "lexer.h"
#include "parser.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Warning: your duty to free the buffer
char* readFile(const char* path) {
    FILE* file = fopen("test.md", "r");
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

int main(int argc, char** argv) {
    printf("Using MarkTeX version 0.1.0\n");

    FILE* outFile = fopen("out.tex", "w");
    char* buf = readFile("test.md");

    Lexer lexer = newLexer(buf);
    tokenize(&lexer);

    Parser parser = {.lexer = &lexer, .pos = 0};

    AstNode* parsed = parseDocument(&parser);

    print(parsed, outFile);
    fclose(outFile);

    destroyNode(parsed);
    free(buf);
    return 0;
}