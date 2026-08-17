#include "latex.h"
#include "lexer.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>

void printHeader(AstNode* node, FILE* file) {
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

    fprintf(file, "\\%s{", command);

    for (int i = 0; i < node->children.size; i++) {
        printNode(node->children.data[i], file);
    }

    fprintf(file, "}\n");
}

void printEnvironment(AstNode* node, FILE* file) {
    if (node == NULL || node->type != NODE_ENVIRONMENT)
        return;

    fprintf(file, "\\begin{");
    fwrite(node->environment.name.data, node->environment.name.length * sizeof(char), 1, file);
    fprintf(file, "}");

    NodeList* title = &node->environment.titleNodes;
    if (title->size > 0) {
        string_view text = title->data[0]->text;
        trim(&text);    

        if(text.length != 0) {
            fprintf(file, "[");
            
            for (int i = 0; i < title->size; i++) {
                AstNode* node = title->data[i];
                printNode(title->data[i], file);
            }

            fprintf(file, "]");
        }
    }

    fputc('\n', file);

    for (int i = 0; i < node->children.size; i++) {
        printNode(node->children.data[i], file);
    }

    fprintf(file, "\\end{");
    fwrite(node->environment.name.data, node->environment.name.length * sizeof(char), 1, file);
    fprintf(file, "}\n\n");
}

void printList(AstNode* node, FILE* file) {
    if(node->type != NODE_LIST || node->children.size == 0) return;
    fprintf(file, "\\begin{itemize}\n");

    for (int i = 0; i < node->children.size; i++) {
        fprintf(file, "\\item ");
        printNode(node->children.data[i], file);
        // '\n' character is inserted by the printNode paragraph
    }

    fprintf(file, "\\end{itemize}\n");
}

void printNode(AstNode* node, FILE* file) {
    if (node->type == NODE_HEADER) {
        printHeader(node, file);
    } else if (node->type == NODE_TEXT) {
        string_view content = node->text;
        fwrite(content.data, content.length * sizeof(char), 1, file);
    } else if (node->type == NODE_ENVIRONMENT) {
        printEnvironment(node, file);
    } else if(node->type == NODE_LIST) {
        printList(node, file);
    } else if (node->type == NODE_PARAGRAPH) {
        for (int i = 0; i < node->children.size; i++) {
            printNode(node->children.data[i], file);
        }
        fprintf(file, "\n");
    } else if(node->type == NODE_BOLD) {
        fprintf(file, "\\textbf{");

        for (int i = 0; i < node->children.size; i++) {
            printNode(node->children.data[i], file);
        }

        fprintf(file, "}");
    }
}

void print(AstNode* root, FILE* file) {
    for (int i = 0; i < root->children.size; i++) {
        printNode(root->children.data[i], file);
    }
}