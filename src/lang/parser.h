/*
This file is part of MarkTeX which is released under the MIT liscence.
See file LISCENCE or go to https://github.com/Lucas-Codeur/MarkTeX/blob/main/LICENSE for full license details.
*/

#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include <stddef.h>

typedef struct AstNode AstNode;

typedef enum {
    NODE_DOCUMENT,
    NODE_HEADER,

    NODE_ENVIRONMENT,
    NODE_PARAGRAPH,
    NODE_LIST,

    NODE_TEXT,
    NODE_BOLD,
    NODE_ITALIC,

    NODE_NEWLINE,
    NODE_EOF
} NodeType;

typedef struct {
    AstNode** data;
    size_t capacity;
    size_t size;
} NodeList;

NodeList newNodeList();
void nodeListAdd(NodeList* list, AstNode* node);
void destroyNodeList(NodeList* list);

struct AstNode {
    union {
        struct {
            int level;
        } header;

        struct {
            string_view name;
            NodeList titleNodes;
        } environment;
    };

    string_view text;
    NodeType type;
    NodeList children;
};

typedef struct {
    Lexer* lexer;
    size_t pos;
    bool interrupted;
} Parser;

Token pExpect(Parser* parser, TokenType type);
void parserInterrupt(Parser* parser);

AstNode* nodeCreate(NodeType type);
void destroyNode(AstNode* node);

Token pPeek(Parser* parser);
Token pPeekNext(Parser* parser);

Token pAdvance(Parser* parser);
bool pAtEnd(Parser* parser);
bool pAtLineStart(Parser* parser);

void trimTextLeft(AstNode* node);
void trimTextRight(AstNode* node);

AstNode* parseDocument(Parser* parser);

AstNode* parseHeader(Parser* parser);
AstNode* parseEnvironment(Parser* parser, int depth);
AstNode* parseParagraph(Parser* parser);
AstNode* parseList(Parser* parser);
AstNode* parseInline(Parser* parser);

AstNode* parseBold(Parser* parser);

#endif