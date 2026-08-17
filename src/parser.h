#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

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
    int capacity;
    int size;
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

        struct {
        } paragraph;
    };

    string_view text;
    NodeType type;
    NodeList children;
};

typedef struct {
    Lexer* lexer;
    int pos;
} Parser;

Token pExpect(Parser* parser, TokenType type);

AstNode* nodeCreate(NodeType type);
void destroyNode(AstNode* node);

Token pPeek(Parser* parser);
Token pPeekNext(Parser* parser);

Token pAdvance(Parser* parser);
bool pAtEnd(Parser* parser);

void trimTextLeft(AstNode* node);
void trimTextRight(AstNode* node);

AstNode* parseDocument(Parser* parser);
AstNode* parseHeader(Parser* parser);
AstNode* parseEnvironment(Parser* parser);
AstNode* parseParagraph(Parser* parser);
AstNode* parseList(Parser* parser);
AstNode* parseInline(Parser* parser);

AstNode* parseBold(Parser* parser);

#endif