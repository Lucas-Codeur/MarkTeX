cmake_minimum_required(VERSION 3.16)
project(marktex VERSION 1.0 LANGUAGES C CXX)

add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

add_executable(marktex
    src/main.c
    src/lang/lexer.c
    src/lang/parser.c
    src/lang/latex.c
)