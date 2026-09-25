/*
This file is part of MarkTeX which is released under the MIT liscence.
See file LISCENCE or go to https://github.com/Lucas-Codeur/MarkTeX/blob/main/LICENSE for full license details.
*/

#include "latex.h"
#include "parser.h"
#include <stddef.h>
#include <stdlib.h>

void printHeader(AstNode* node, OutputBuffer* buffer) {
    if (node->type != NODE_HEADER)
        return;

    const char* command;

    switch (node->header.level) {
    case 1:
        command = "section";
        break;
    case 2:
        command = "subsection";
        break;
    case 3:
    default:
        command = "subsubsection";
        break;
    }

    obWriteFormat(buffer, "\\%s{", command);

    for (size_t i = 0; i < node->children.size; i++) {
        printNode(node->children.data[i], buffer);
    }

    obWriteStr(buffer, "}\n");
}

void printEnvironment(AstNode* node, OutputBuffer* buffer) {
    if (node == NULL || node->type != NODE_ENVIRONMENT)
        return;

    obWriteStr(buffer, "\\begin{");
    obWriteStrView(buffer, &node->environment.name);
    obWriteStr(buffer, "}");

    NodeList* title = &node->environment.titleNodes;
    if (title->size > 0) {
        string_view text = title->data[0]->text;
        trim(&text);

        if (text.length != 0) {
            obWriteStr(buffer, "[");

            for (size_t i = 0; i < title->size; i++) {
                printNode(title->data[i], buffer);
            }

            obWriteStr(buffer, "]");
        }
    }

    obWriteStr(buffer, "\n");

    for (size_t i = 0; i < node->children.size; i++) {
        printNode(node->children.data[i], buffer);
    }

    obWriteStr(buffer, "\\end{");
    obWriteStrView(buffer, &node->environment.name);
    obWriteStr(buffer, "}");
}

void printList(AstNode* node, OutputBuffer* buffer) {
    if (node->type != NODE_LIST || node->children.size == 0)
        return;
    obWriteStr(buffer, "\\begin{itemize}\n");

    for (size_t i = 0; i < node->children.size; i++) {
        obWriteStr(buffer, "\\item ");
        printNode(node->children.data[i], buffer);
        // '\n' character is inserted by the printNode paragraph
    }

    obWriteStr(buffer, "\\end{itemize}\n");
}

void printNode(AstNode* node, OutputBuffer* buffer) {
    if (node->type == NODE_HEADER) {
        printHeader(node, buffer);
    } else if (node->type == NODE_TEXT) {
        obWriteStrView(buffer, &node->text);
    } else if (node->type == NODE_ENVIRONMENT) {
        printEnvironment(node, buffer);
    } else if (node->type == NODE_LIST) {
        printList(node, buffer);
    } else if (node->type == NODE_PARAGRAPH) {
        for (size_t i = 0; i < node->children.size; i++) {
            printNode(node->children.data[i], buffer);
        }
        obWriteStr(buffer, "\n");
    } else if (node->type == NODE_BOLD) {
        obWriteStr(buffer, "\\textbf{");

        for (size_t i = 0; i < node->children.size; i++) {
            printNode(node->children.data[i], buffer);
        }

        obWriteStr(buffer, "}");
    } else if(node->type == NODE_NEWLINE) {
        obWriteStr(buffer, "\n");
    }
}

void print(AstNode* root, OutputBuffer* buffer) {
    for (size_t i = 0; i < root->children.size; i++) {
        printNode(root->children.data[i], buffer);
    }
}