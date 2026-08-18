/*
This file is part of MarkTeX which is released under the MIT liscence.
See file LISCENCE or go to https://github.com/Lucas-Codeur/MarkTeX/blob/main/LICENSE for full license details.
*/

#ifndef LATEX_H
#define LATEX_H

#include "parser.h"

void printNode(AstNode* node, OutputBuffer* buffer);
void print(AstNode* root, OutputBuffer* buffer);

#endif