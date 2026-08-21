/*
This file is part of MarkTeX which is released under the MIT liscence.
See file LISCENCE or go to https://github.com/Lucas-Codeur/MarkTeX/blob/main/LICENSE for full license details.
*/

#include "parser.h"
#include "lexer.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define INITIAL_NODE_CAPACITY 4

static Token INVALID_TOKEN = {.type = TOKEN_INVALID};

NodeList newNodeList() {
    NodeList list;
    list.capacity = INITIAL_NODE_CAPACITY;
    list.size = 0;

    list.data = malloc(list.capacity * sizeof(AstNode*));
    if (list.data == NULL) {
        list.capacity = 0;
        marktexLog(LOG_ERROR, "Node list allocation failed.");
    }

    return list;
}

void nodeListAdd(NodeList* list, AstNode* node) {
    if (list->size >= list->capacity) {
        int newCapacity = list->capacity * 2;

        AstNode** newData = realloc(list->data, newCapacity * sizeof(AstNode*));

        if (newData == NULL) {
            marktexLog(LOG_ERROR, "Node list reallocation failed.");
            return;
        }

        list->data = newData;
        list->capacity = newCapacity;
    }

    list->data[list->size] = node;
    list->size++;
}

void destroyNode(AstNode* node) {
    if (node == NULL)
        return;

    if (node->type == NODE_ENVIRONMENT) {
        destroyNodeList(&node->environment.titleNodes);
    }

    destroyNodeList(&node->children);

    free(node);
}

void destroyNodeList(NodeList* list) {
    for (size_t i = 0; i < list->size; i++) {
        destroyNode(list->data[i]);
    }

    free(list->data);

    list->data = NULL;
    list->size = 0;
    list->capacity = 0;
}

void parserInterrupt(Parser* parser) {
    parser->interrupted = true;
}

Token pExpect(Parser* parser, TokenType type) {
    Token token = pPeek(parser);

    if (token.type != type) {
        marktexLog(LOG_WARNING, "Expected token %s, got %s at line %d.", tokenTypeName(type), tokenTypeName(token.type), token.location.line);

        return INVALID_TOKEN;
    }

    pAdvance(parser);
    return token;
}

AstNode* nodeCreate(NodeType type) {
    AstNode* node = malloc(sizeof(AstNode));
    node->type = type;
    node->children = newNodeList();
    return node;
}

Token pPeek(Parser* parser) {
    if (pAtEnd(parser))
        return parser->lexer->tokens.data[parser->lexer->tokens.size - 1];
    return parser->lexer->tokens.data[parser->pos];
}

Token pPeekNext(Parser* parser) {
    if (parser->pos + 1 >= parser->lexer->tokens.size) {
        return parser->lexer->tokens.data[parser->lexer->tokens.size - 1];
    }
    return parser->lexer->tokens.data[parser->pos + 1];
}

Token pAdvance(Parser* parser) {
    if (pAtEnd(parser))
        return parser->lexer->tokens.data[parser->lexer->tokens.size - 1];
    return parser->lexer->tokens.data[parser->pos++];
}

bool pAtEnd(Parser* parser) {
    if(parser->pos < parser->lexer->tokens.size) {
        return parser->lexer->tokens.data[parser->pos].type == TOKEN_EOF;
    }
    return true; 
}

void trimTextLeft(AstNode* node) {
    if (node->type == NODE_TEXT) {
        while (node->text.length > 0 && (node->text.data[0] == ' ' || node->text.data[0] == '\t' ||
                                         node->text.data[0] == '\n')) {
            node->text.data++;
            node->text.length--;
        }
    } else {
        if (node->children.size > 0) {
            trimTextLeft(node->children.data[0]);
        }
    }
}

void trimTextRight(AstNode* node) {
    if (node->type == NODE_TEXT) {
        while (node->text.length > 0 && (node->text.data[node->text.length - 1] == ' ' ||
                                         node->text.data[node->text.length - 1] == '\t' ||
                                         node->text.data[node->text.length - 1] == '\n')) {
            node->text.length--;
        }
    } else {
        if (node->children.size > 0) {
            trimTextRight(node->children.data[node->children.size - 1]);
        }
    }
}

void trimNodeListText(NodeList* list) {
    if (list == NULL || list->size == 0)
        return;

    trimTextLeft(list->data[0]);
    trimTextRight(list->data[list->size - 1]);
}

AstNode* parseDocument(Parser* parser) {
    AstNode* document = nodeCreate(NODE_DOCUMENT);

    while (!pAtEnd(parser) && !parser->interrupted) {
        if (pPeek(parser).type == TOKEN_HASH && pPeek(parser).location.column == 1) {
            nodeListAdd(&document->children, parseHeader(parser));
            continue;
        }

        if (pPeek(parser).type == TOKEN_AT) {
            nodeListAdd(&document->children, parseEnvironment(parser));
            continue;
        }

        if (pPeek(parser).type == TOKEN_TEXT) {
            nodeListAdd(&document->children, parseParagraph(parser));
            continue;
        }

        if (pPeek(parser).type == TOKEN_DASH && pPeek(parser).location.column == 1) {
            nodeListAdd(&document->children, parseList(parser));
            continue;
        } else if(pPeek(parser).type == TOKEN_DASH) {
            AstNode* node = nodeCreate(NODE_TEXT);
            node->text = pPeek(parser).text;
            nodeListAdd(&document->children, node);
            pAdvance(parser);
            continue;
        }

        if (pPeek(parser).type == TOKEN_NEWLINE) {
            AstNode* node = nodeCreate(NODE_NEWLINE);
            nodeListAdd(&document->children, node);
            pAdvance(parser);
            continue;
        }

        marktexLog(LOG_WARNING, "Parser error: found token (%s) at line %i.", tokenTypeName(pPeek(parser).type), pPeek(parser).location.line);
        parserInterrupt(parser);
    }

    return document;
}

AstNode* parseHeader(Parser* parser) {
    int level = 0;

    // Consume the hashtag
    while (pPeek(parser).type == TOKEN_HASH && !parser->interrupted) {
        pAdvance(parser);
        level++;
    }

    if (level == 0 || level > 3) {
        marktexLog(LOG_WARNING, "Found invalid header level at line %i, should be between 1 and 3.", pPeek(parser).location.line);
        return NULL;
    }

    AstNode* header = nodeCreate(NODE_HEADER);
    header->header.level = level;

    // Rest of line is title
    while (pPeek(parser).type != TOKEN_NEWLINE && !pAtEnd(parser) && !parser->interrupted) {
        AstNode* child = parseInline(parser);

        if (child == NULL)
            return NULL;

        nodeListAdd(&header->children, child);
    }
    trimNodeListText(&header->children);

    // Consume \n
    if (pPeek(parser).type == TOKEN_NEWLINE)
        pAdvance(parser);

    return header;
}

AstNode* parseEnvironment(Parser* parser) {
    pExpect(parser, TOKEN_AT);

    Token name = pExpect(parser, TOKEN_IDENTIFIER);
    trim(&name.text);

    AstNode* environment = nodeCreate(NODE_ENVIRONMENT);
    environment->environment.name = name.text;
    environment->environment.titleNodes = newNodeList();

    while (pPeek(parser).type != TOKEN_NEWLINE && !parser->interrupted) {
        AstNode* node = parseInline(parser);

        if (node == NULL) {
            return NULL;
        }

        nodeListAdd(&environment->environment.titleNodes, node);
    }
    trimNodeListText(&environment->environment.titleNodes);

    pExpect(parser, TOKEN_NEWLINE);

    while (pPeek(parser).type != TOKEN_DOUBLE_AT && 
           !parser->interrupted) {
        if (pPeek(parser).type == TOKEN_DASH) {
            nodeListAdd(&environment->children, parseList(parser));
        } else {
            nodeListAdd(&environment->children, parseParagraph(parser));
        }
    }

    pExpect(parser, TOKEN_DOUBLE_AT);

    return environment;
}

AstNode* parseParagraph(Parser* parser) {
    AstNode* paragraph = nodeCreate(NODE_PARAGRAPH);

    while (pPeek(parser).type != TOKEN_NEWLINE && 
           pPeek(parser).type != TOKEN_DOUBLE_AT &&
           !pAtEnd(parser) &&
           !parser->interrupted) {
        AstNode* child = parseInline(parser);

        if (child == NULL) {
            destroyNode(paragraph);
            return NULL;
        }

        nodeListAdd(&paragraph->children, child);
    }

    if (pPeek(parser).type == TOKEN_NEWLINE)
        pAdvance(parser);

    return paragraph;
}

AstNode* parseList(Parser* parser) {
    // Should not process two consecutive dash as a list
    if(pPeek(parser).type == TOKEN_DASH && pPeekNext(parser).type == TOKEN_DASH) {
        AstNode* node = nodeCreate(NODE_TEXT);

        node->text = pPeek(parser).text;
        pAdvance(parser);
        return node;
    }

    AstNode* list = nodeCreate(NODE_LIST);

    while (pPeek(parser).type == TOKEN_DASH && pPeek(parser).location.column == 1 && !parser->interrupted) {
        pAdvance(parser);

        AstNode* paragraph = parseParagraph(parser);

        if (paragraph == NULL) {
            destroyNode(paragraph);
            destroyNode(list);
            return NULL;
        }

        trimTextLeft(paragraph);
        trimTextRight(paragraph);

        nodeListAdd(&list->children, paragraph);
    }

    return list;
}

AstNode* parseInline(Parser* parser) {
    Token token = pPeek(parser);

    if (pPeek(parser).type == TOKEN_TEXT || pPeek(parser).type == TOKEN_DASH || pPeek(parser).type == TOKEN_HASH) {
        pAdvance(parser);

        AstNode* node = nodeCreate(NODE_TEXT);
        node->text = token.text;

        return node;
    }

    if (pPeek(parser).type == TOKEN_BOLD) {
        return parseBold(parser);
    }

    marktexLog(LOG_WARNING, "Wrong token (%s) in inline content at line %i.", tokenTypeName(token.type), token.location.line);
    parserInterrupt(parser);
    return NULL;
}

AstNode* parseBold(Parser* parser) {
    AstNode* node = nodeCreate(NODE_BOLD);

    Token start = pExpect(parser, TOKEN_BOLD);

    while (pPeek(parser).type != TOKEN_BOLD && !pAtEnd(parser) && !parser->interrupted) {
        AstNode* child = parseInline(parser);

        if (child == NULL)
            return NULL;

        nodeListAdd(&node->children, child);
    }

    if (pAtEnd(parser)) {
        marktexLog(LOG_WARNING, "Unterminated bold expression starting at line %i.", start.location.line);
        return NULL;
    }

    pExpect(parser, TOKEN_BOLD);
    return node;
}