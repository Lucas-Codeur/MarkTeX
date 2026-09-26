#include "frontmatter.h"
#include <stdbool.h>
#include <string.h>
#include "../utils.h"

int_range getFrontmatterRange(const char* content) {
    int_range range = {
        .start = -1,
        .end = -1,
    };

    // If there is no front matter delimiter
    if(strncmp(content, "---\n", 4) != 0) return range;
    int i = 0;
    while (content[i + 5] != '\0') {        
        if(strncmp(content + i, "\n---\n", 5) == 0) {
            range.start = 4;
            range.end = i;
            break;
        }

        i++;
    }

    return range;
}