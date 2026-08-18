#include "latex.h"
#include "parser.h"
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

    outputBufferWriteFormat(buffer, "\\%s{", command);

    for (int i = 0; i < node->children.size; i++) {
        printNode(node->children.data[i], buffer);
    }

    outputBufferWriteStr(buffer, "}\n");
}

void printEnvironment(AstNode* node, OutputBuffer* buffer) {
    if (node == NULL || node->type != NODE_ENVIRONMENT)
        return;

    outputBufferWriteStr(buffer, "\\begin{");
    outputBufferWriteStrView(buffer, &node->environment.name);
    outputBufferWriteStr(buffer, "}");

    NodeList* title = &node->environment.titleNodes;
    if (title->size > 0) {
        string_view text = title->data[0]->text;
        trim(&text);

        if (text.length != 0) {
            outputBufferWriteStr(buffer, "[");

            for (int i = 0; i < title->size; i++) {
                AstNode* node = title->data[i];
                printNode(title->data[i], buffer);
            }

            outputBufferWriteStr(buffer, "]");
        }
    }

    outputBufferWriteStr(buffer, "\n");

    for (int i = 0; i < node->children.size; i++) {
        printNode(node->children.data[i], buffer);
    }

    outputBufferWriteStr(buffer, "\\end{");
    outputBufferWriteStrView(buffer, &node->environment.name);
    outputBufferWriteStr(buffer, "}\n\n");
}

void printList(AstNode* node, OutputBuffer* buffer) {
    if (node->type != NODE_LIST || node->children.size == 0)
        return;
    outputBufferWriteStr(buffer, "\\begin{itemize}\n");

    for (int i = 0; i < node->children.size; i++) {
        outputBufferWriteStr(buffer, "\\item ");
        printNode(node->children.data[i], buffer);
        // '\n' character is inserted by the printNode paragraph
    }

    outputBufferWriteStr(buffer, "\\end{itemize}\n");
}

void printNode(AstNode* node, OutputBuffer* buffer) {
    if (node->type == NODE_HEADER) {
        printHeader(node, buffer);
    } else if (node->type == NODE_TEXT) {
        outputBufferWriteStrView(buffer, &node->text);
    } else if (node->type == NODE_ENVIRONMENT) {
        printEnvironment(node, buffer);
    } else if (node->type == NODE_LIST) {
        printList(node, buffer);
    } else if (node->type == NODE_PARAGRAPH) {
        for (int i = 0; i < node->children.size; i++) {
            printNode(node->children.data[i], buffer);
        }
        outputBufferWriteStr(buffer, "\n");
    } else if (node->type == NODE_BOLD) {
        outputBufferWriteStr(buffer, "\\textbf{");

        for (int i = 0; i < node->children.size; i++) {
            printNode(node->children.data[i], buffer);
        }

        outputBufferWriteStr(buffer, "}");
    }
}

void print(AstNode* root, OutputBuffer* buffer) {
    for (int i = 0; i < root->children.size; i++) {
        printNode(root->children.data[i], buffer);
    }
}