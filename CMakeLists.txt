cmake_minimum_required(VERSION 3.20)
project(marktex VERSION 1.0 LANGUAGES C)

add_subdirectory(libs/libyaml)

# Embbed the default template

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/default.tex" TEMPLATE)

# Escape characters for a C string
string(REPLACE "\\" "\\\\" TEMPLATE "${TEMPLATE}")
string(REPLACE "\"" "\\\"" TEMPLATE "${TEMPLATE}")
string(REPLACE "\n" "\\n\"\n\"" TEMPLATE "${TEMPLATE}")
string(REPLACE "\r" "\\r" TEMPLATE "${TEMPLATE}")

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/src/default_template.c"
"const char default_template[] = \"${TEMPLATE}\";\n"
)

add_executable(marktex
    src/main.c
    src/lang/lexer.c
    src/lang/parser.c
    src/lang/latex.c
    src/utils.c
    src/config/frontmatter.c
    src/default_template.c
)

target_link_libraries(marktex PRIVATE
    yaml
)

# Debug config
target_compile_options(marktex PRIVATE
    $<$<CONFIG:Debug>:
        -Wall
        -Wextra
        -Wpedantic
        -g
        -fsanitize=address,undefined
    >
)

target_link_options(marktex PRIVATE
    $<$<CONFIG:Debug>:
        -fsanitize=address,undefined
    >
)

# Release config
target_compile_options(marktex PRIVATE
    $<$<CONFIG:Release>:
        -O2
        -DNDEBUG
    >
)