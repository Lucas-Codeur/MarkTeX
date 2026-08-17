#ifndef LATEX_H
#define LATEX_H

#include "parser.h"
#include <stdio.h>

void printNode(AstNode* node, FILE* file);
void print(AstNode* root, FILE* file);

#endif