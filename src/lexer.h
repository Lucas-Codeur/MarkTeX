#ifndef LEXER_H
#define LEXER_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    int line;
    int column;
    int offset;
} SourceLocation;

typedef struct {
    const char* data;
    int length;
} string_view;

void trim(string_view* view);

typedef enum {
    TOKEN_SOF, // Start Of File (so previous doesn't break)

    TOKEN_IDENTIFIER,
    TOKEN_TEXT,
    TOKEN_NEWLINE,

    TOKEN_HASH,
    TOKEN_DASH,
    TOKEN_AT,
    TOKEN_DOUBLE_AT,

    TOKEN_BOLD,
    TOKEN_ITALIC,

    TOKEN_EOF, // End Of File
} TokenType;

typedef struct {
    TokenType type;
    string_view text;
    SourceLocation location;
} Token;

typedef struct {
    int size;
    int capacity;
    Token* data;
} TokenList;

TokenList newTokenList();
void addToken(TokenList* list, TokenType type, string_view view, SourceLocation location);
void destroyTokenList(TokenList* list);

typedef struct {
    char* buffer;
    int length;
    TokenList tokens;

    int pos;
    int line;
    int column;
} Lexer;

Lexer newLexer(char* source);
void destroyLexer(Lexer* lexer);

char lPeek(Lexer* lexer);
char lPeekNext(Lexer* lexer);
Token lPeekLast(Lexer* lexer);

char lAdvance(Lexer* lexer);
bool lAtEnd(Lexer* lexer);

void lSkipSpaces(Lexer* lexer);

void tokenize(Lexer* lexer);

#endif