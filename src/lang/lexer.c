/*
This file is part of MarkTeX which is released under the MIT liscence.
See file LISCENCE or go to https://github.com/Lucas-Codeur/MarkTeX/blob/main/LICENSE for full license details.
*/

#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_TOKEN_CAPACITY 128

static string_view EMPTY_STR_VIEW = {0};

TokenList newTokenList() {
    TokenList list;
    list.capacity = INITIAL_TOKEN_CAPACITY;
    list.size = 0;

    list.data = malloc(list.capacity * sizeof(Token));
    if (list.data == NULL) {
        list.capacity = 0;
        printf("Token list allocation failed\n");
    }

    return list;
}

void addToken(TokenList* list, TokenType type, string_view view, SourceLocation location) {
    if (list->size >= list->capacity) {
        int newCapacity = list->capacity * 2;

        Token* newData = realloc(list->data, newCapacity * sizeof(Token));

        if (newData == NULL) {
            printf("Token list reallocation failed\n");
            return;
        }

        list->data = newData;
        list->capacity = newCapacity;
    }

    Token* token = (Token*)(list->data + list->size);
    token->type = type;
    token->text = view;
    token->location = location;
    list->size++;
}

void destroyTokenList(TokenList* list) {
    free(list->data);
    
    list->data = NULL;
    list->capacity = 0;
    list->size = 0;
}

Lexer newLexer(char* source) {
    Lexer lexer;
    lexer.line = 1;
    lexer.pos = 0;
    lexer.column = 1;
    lexer.buffer = source;
    lexer.tokens = newTokenList();
    lexer.length = strlen(source);
    return lexer;
}

void destroyLexer(Lexer* lexer) {
    destroyTokenList(&lexer->tokens);
    lexer->length = 0;
    lexer->pos = 0;
    lexer->buffer = NULL;
    lexer->column = 0;
    lexer->line = 1;
}

char lPeek(Lexer* lexer) {
    if (lAtEnd(lexer))
        return '\0';
    return lexer->buffer[lexer->pos];
}

char lPeekNext(Lexer* lexer) {
    if (lexer->pos + 1 >= lexer->length)
        return '\0';
    return lexer->buffer[lexer->pos + 1];
}

Token lPeekLast(Lexer* lexer) {
    if (lexer->tokens.size == 0) {
        Token empty = {0};
        empty.type = TOKEN_SOF;
        return empty;
    }
    return lexer->tokens.data[lexer->tokens.size - 1];
}

char lAdvance(Lexer* lexer) {
    if (lAtEnd(lexer))
        return '\0';

    if (lexer->buffer[lexer->pos] == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    return lexer->buffer[lexer->pos++];
}

bool lAtEnd(Lexer* lexer) { return lexer->pos >= lexer->length; }

void lSkipSpaces(Lexer* lexer) {
    while (lPeek(lexer) == ' ' || lPeek(lexer) == '\t' || lPeek(lexer) == '\n') {
        lAdvance(lexer);
    }
}

void lSkipLine(Lexer* lexer) {
    while (lPeek(lexer) != '\n') {
        lAdvance(lexer);
    }

    // Consume the '\n'
    lAdvance(lexer);
}

void tokenize(Lexer* lexer) {
    // For texts currently being assembled
    int textStart = 0;
    int textLength = 0;
    SourceLocation textLocation;

#define END_TEXT                                                                                   \
    {                                                                                              \
        if (textLength != 0) {                                                                     \
            string_view view = {.data = lexer->buffer + textStart, .length = textLength};          \
            addToken(&lexer->tokens, TOKEN_TEXT, view, textLocation);                              \
            textLength = 0;                                                                        \
        }                                                                                          \
    };

    while (!lAtEnd(lexer)) {
        SourceLocation srcLoc = {
            .column = lexer->column, .line = lexer->line, .offset = lexer->pos};

        // Parse block start / ending
        if (lPeek(lexer) == '@') {
            END_TEXT;

            if (lPeekNext(lexer) == '@') {
                addToken(&lexer->tokens, TOKEN_DOUBLE_AT, EMPTY_STR_VIEW, srcLoc);
                lAdvance(lexer);
                lAdvance(lexer);

            } else {
                addToken(&lexer->tokens, TOKEN_AT, EMPTY_STR_VIEW, srcLoc);
                lAdvance(lexer);

                // Parse next word as a text
                lSkipSpaces(lexer);

                textLength = 0;
                textStart = lexer->pos;
                while (!lAtEnd(lexer) && lPeek(lexer) != ' ' && lPeek(lexer) != '\t' &&
                       lPeek(lexer) != '\n') {
                    textLength++;
                    lAdvance(lexer);
                }

                string_view view = {.data = lexer->buffer + textStart, .length = textLength};
                addToken(&lexer->tokens, TOKEN_IDENTIFIER, view, srcLoc);
                textLength = 0;
            }
            continue;
        }

        // Parse title
        if (lPeek(lexer) == '#') {
            END_TEXT;
            string_view view = {.data = lexer->buffer + lexer->pos, .length = 1};

            addToken(&lexer->tokens, TOKEN_HASH, view, srcLoc);
            lAdvance(lexer);
            continue;
        }

        // Parse dash
        if (lPeek(lexer) == '-') {
            END_TEXT;
            string_view view = {.data = lexer->buffer + lexer->pos, .length = 1};

            addToken(&lexer->tokens, TOKEN_DASH, view, srcLoc);
            lAdvance(lexer);
            continue;
        }

        // Parse bold text
        if (lPeek(lexer) == '*' && lPeekNext(lexer) == '*') {
            END_TEXT;

            addToken(&lexer->tokens, TOKEN_BOLD, EMPTY_STR_VIEW, srcLoc);
            lAdvance(lexer);
            lAdvance(lexer);
            continue;
        }

        // Parse italic text
        if (lPeek(lexer) == '_' && lPeekNext(lexer) == '_') {
            END_TEXT;

            addToken(&lexer->tokens, TOKEN_ITALIC, EMPTY_STR_VIEW, srcLoc);
            lAdvance(lexer);
            lAdvance(lexer);
            continue;
        }

        // Parse newline
        if (lPeek(lexer) == '\n') {
            END_TEXT;
            string_view view = {.data = lexer->buffer + lexer->pos, .length = 1};

            addToken(&lexer->tokens, TOKEN_NEWLINE, view, srcLoc);
            lAdvance(lexer);
            continue;
        }

        // Everything else should be text
        if (textLength == 0) {
            textLocation.line = lexer->line;
            textLocation.column = lexer->column;
            textLocation.offset = lexer->pos;

            textStart = lexer->pos;
            textLength = 1;
        } else {
            textLength++;
        }
        lAdvance(lexer);
    }
}