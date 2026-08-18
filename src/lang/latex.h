#ifndef LATEX_H
#define LATEX_H

#include "parser.h"

void printNode(AstNode* node, OutputBuffer* buffer);
void print(AstNode* root, OutputBuffer* buffer);

#endif