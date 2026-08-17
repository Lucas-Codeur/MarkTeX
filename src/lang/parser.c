#include "parser.h"
#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>

#define INITIAL_NODE_CAPACITY 4

static Token EMPTY_TOKEN = {.type = TOKEN_EOF};

NodeList newNodeList() {
    NodeList list;
    list.capacity = INITIAL_NODE_CAPACITY;
    list.size = 0;

    list.data = malloc(list.capacity * sizeof(AstNode*));
    if (list.data == NULL) {
        list.capacity = 0;
        printf("Node list allocation failed\n");
    }

    return list;
}

void nodeListAdd(NodeList* list, AstNode* node) {
    if (list->size >= list->capacity) {
        int newCapacity = list->capacity * 2;

        AstNode** newData = realloc(list->data, newCapacity * sizeof(AstNode*));

        if (newData == NULL) {
            printf("Node list reallocation failed\n");
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

Token pExpect(Parser* parser, TokenType type) {
    Token token = pPeek(parser);

    if (token.type != type) {
        printf("Expected token %d, got %d at line %d\n", type, token.type, token.location.line);

        return EMPTY_TOKEN;
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
        return EMPTY_TOKEN;
    return parser->lexer->tokens.data[parser->pos];
}

Token pPeekNext(Parser* parser) {
    if (parser->pos + 1 >= parser->lexer->tokens.size)
        return EMPTY_TOKEN;
    return parser->lexer->tokens.data[parser->pos + 1];
}

Token pAdvance(Parser* parser) {
    if (pAtEnd(parser))
        return EMPTY_TOKEN;
    return parser->lexer->tokens.data[parser->pos++];
}

bool pAtEnd(Parser* parser) { return parser->pos >= parser->lexer->tokens.size; }

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

    while (!pAtEnd(parser)) {
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
            printf("List found\n");
            nodeListAdd(&document->children, parseList(parser));
            continue;
        }

        if (pPeek(parser).type == TOKEN_NEWLINE) {
            AstNode* node = nodeCreate(NODE_NEWLINE);
            nodeListAdd(&document->children, node);
            pAdvance(parser);
            continue;
        }

        printf(
            "Parser error: found token (%i) at line %i.\n", pPeek(parser).type,
            pPeek(parser).location.line);
    }

    return document;
}

AstNode* parseHeader(Parser* parser) {
    int level = 0;

    // Consume the hashtag
    while (pPeek(parser).type == TOKEN_HASH) {
        pAdvance(parser);
        level++;
    }

    if (level == 0 || level > 3) {
        printf("Invalid header level\n");
        return NULL;
    }

    AstNode* header = nodeCreate(NODE_HEADER);
    header->header.level = level;

    // Rest of line is title
    while (pPeek(parser).type != TOKEN_NEWLINE && !pAtEnd(parser)) {
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

    while (pPeek(parser).type != TOKEN_NEWLINE) {
        AstNode* node = parseInline(parser);

        if (node == NULL) {
            printf("parseInline returned NULL\n");
            return NULL;
        }

        nodeListAdd(&environment->environment.titleNodes, node);
    }
    trimNodeListText(&environment->environment.titleNodes);

    pExpect(parser, TOKEN_NEWLINE);

    while (pPeek(parser).type != TOKEN_DOUBLE_AT) {
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

    while (pPeek(parser).type != TOKEN_NEWLINE && pPeek(parser).type != TOKEN_DOUBLE_AT) {
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
    AstNode* list = nodeCreate(NODE_LIST);

    while (pPeek(parser).type == TOKEN_DASH && pPeek(parser).location.column == 1) {
        pAdvance(parser);

        AstNode* paragraph = parseParagraph(parser);

        if (paragraph == NULL) {
            destroyNode(paragraph);
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

    if (pPeek(parser).type == TOKEN_TEXT || pPeek(parser).type == TOKEN_DASH ||
        pPeek(parser).type == TOKEN_HASH) {
        pAdvance(parser);

        AstNode* node = nodeCreate(NODE_TEXT);
        node->text = token.text;

        return node;
    }

    if (pPeek(parser).type == TOKEN_BOLD) {
        return parseBold(parser);
    }

    printf("Wrong token (%i) in inline content at line %i\n", token.type, token.location.line);
    return NULL;
}

AstNode* parseBold(Parser* parser) {
    AstNode* node = nodeCreate(NODE_BOLD);

    pExpect(parser, TOKEN_BOLD);

    while (pPeek(parser).type != TOKEN_BOLD && !pAtEnd(parser)) {
        AstNode* child = parseInline(parser);

        if (child == NULL)
            return NULL;

        nodeListAdd(&node->children, child);
    }

    if (pAtEnd(parser)) {
        printf("Unterminated bold expression\n");
        return NULL;
    }

    pExpect(parser, TOKEN_BOLD);
    return node;
}