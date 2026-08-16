#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct {
    const char* data;
    size_t pos;
    size_t length;
    int line;
    int linePos;
} Parser;

bool atEnd(Parser* parser) {
    return parser->pos >= parser->length;
}

char peek(Parser* parser) {
    if(atEnd(parser)) {
        return '\0';
    }

    return parser->data[parser->pos];
}

char peekNext(Parser* parser) {
    if(parser->pos + 1 >= parser->length) {
        return '\0';
    }

    return parser->data[parser->pos + 1];
}


char advance(Parser* parser) {
    if(atEnd(parser)) {
        return '\0';
    }

    char c = parser->data[parser->pos++];
    parser->linePos++;

    if(c == '\n') {
        parser->linePos = 0;
        parser->line++;
    }

    return c;
}

void skipSpaces(Parser* parser) {
    while (!atEnd(parser) && (peek(parser) == ' ' || peek(parser) == '\t')) {
        advance(parser);
    }
}

// Warning: your duty to free the buffer
char* readFile(const char* path) {
    FILE* file = fopen("test.md", "r");
    if(!file) {
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

void parseHeader(Parser* parser, FILE* file) {
    int level = 0;

    while (!atEnd(parser) && peek(parser) == '#') {
        advance(parser);
        level++;
    }
    
    skipSpaces(parser);

    if(level == 0 || level > 3) return;

    const char* command;
    switch (level) {
        case 1:
            command = "section";
            break;
        case 2:
            command = "subsection";
            break;
        case 3:
            command = "subsubsection";
            break;
        default:
            return;
    }

    fprintf(file, "\\%s{", command);

    while (!atEnd(parser) && peek(parser) != '\n') {
        fputc(peek(parser), file);
        advance(parser);
    }

    fprintf(file, "}\n");

    //if(!atEnd(parser)) advance(parser);
}

void parseBlock(Parser* parser, FILE* file) {
    if(peekNext(parser) == '@') {
        printf("Unexpected @@ bloc termination at line %i\n", parser->line);
        return;
    }

    advance(parser);
    skipSpaces(parser);
    //printf("Character: %c.\n", peek(parser));

    char command[256] = {0};

    char word[256] = {0};
    int wordSize = 0;

    // Parse command environment

    while (!atEnd(parser) && (peek(parser) != ' ' && peek(parser) != '\t' && peek(parser) != '\n')) {
        if(wordSize > 255) {
            printf("Too much characters as environment, at line %i, maximum is 256\n", parser->line);
            return;
        }
        word[wordSize] = peek(parser);
        wordSize++;

        advance(parser);
    }
    word[wordSize] = '\0';

    fprintf(file, "\\begin{%s}", word);
    strcpy(command, word);

    // Parse title
    skipSpaces(parser);
    if(peek(parser) != '\n') {
        wordSize = 0;
        while (!atEnd(parser) && peek(parser) != '\n') {
            if(wordSize > 255) {
                printf("Too much characters as environment, at line %i, maximum is 256\n", parser->line);
                return;
            }
            word[wordSize] = peek(parser);
            wordSize++;

            advance(parser);
        }
        word[wordSize] = '\0';

        fprintf(file, "[%s]", word);
    }

    fputc('\n', file);

    // Parse content
    advance(parser);
    while (!atEnd(parser)) {
        // Detect block ending
        if(peek(parser) == '@' && peekNext(parser) == '@') {
            advance(parser);
            advance(parser);

            fprintf(file, "\\end{%s}\n", command);

            while (!atEnd(parser) && peek(parser) != '\n') {
                advance(parser);
            }

            if(!atEnd(parser)) advance(parser);

            return;
        }

        fputc(advance(parser), file);
    }

}

int main(int argc, char** argv) {
    printf("Using MarkTeX version 0.1.0\n");

    FILE* outFile = fopen("out.tex", "w");
    char* buf = readFile("test.md");

    Parser parser = {
        .data = buf,
        .line = 1,
        .linePos = 0,
        .pos = 0,
        .length = strlen(buf)
    };
    
    while (!atEnd(&parser)) {
        char c = peek(&parser);

        if(parser.linePos == 0 && c == '#') {
            parseHeader(&parser, outFile);
            continue;
        } else if(parser.linePos == 0 && c == '@') {
            parseBlock(&parser, outFile);
            continue;
        }

        fputc(advance(&parser), outFile);
    }

    fclose(outFile);
    free(buf);
    return 0;
}