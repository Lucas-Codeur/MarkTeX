/*
This file is part of MarkTeX which is released under the MIT liscence.
See file LISCENCE or go to https://github.com/Lucas-Codeur/MarkTeX/blob/main/LICENSE for full license details.
*/

#ifndef LEXER_H
#define LEXER_H

#include <stdbool.h>
#include <stddef.h>
#include "../utils.h"

typedef struct {
    int line;
    int column;
    int offset;
} SourceLocation;

// Remember to add the type name in tokenTypeName();
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
    TOKEN_UNDERLINE,

    TOKEN_INVALID,
    TOKEN_EOF, // End Of File
} TokenType;

const char* tokenTypeName(TokenType type);

typedef struct {
    TokenType type;
    string_view text;
    SourceLocation location;
} Token;

typedef struct {
    size_t size;
    size_t capacity;
    Token* data;
} TokenList;

TokenList newTokenList();
void addToken(TokenList* list, TokenType type, string_view view, SourceLocation location);
void destroyTokenList(TokenList* list);

typedef struct {
    char* buffer;
    size_t length;
    TokenList tokens;

    size_t pos;
    size_t line;
    size_t column;
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