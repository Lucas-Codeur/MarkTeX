cmake_minimum_required(VERSION 3.20)
project(marktex VERSION 1.0 LANGUAGES C)

add_executable(marktex
    src/main.c
    src/lang/lexer.c
    src/lang/parser.c
    src/lang/latex.c
    src/utils.c
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