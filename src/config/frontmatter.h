/*
This file is part of MarkTeX which is released under the MIT liscence.
See file LISCENCE or go to https://github.com/Lucas-Codeur/MarkTeX/blob/main/LICENSE for full license details.
*/

#ifndef FRONTMATTER_H
#define FRONTMATTER_H

#include <stdio.h>
#include <yaml.h>

typedef struct {
    int start;
    int end;
} int_range;

// Returns start -1 and end -1 if no frontmatter
int_range getFrontmatterRange(const char* content);

#endif