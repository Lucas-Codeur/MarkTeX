cmake_minimum_required(VERSION 3.16)
project(marktex VERSION 1.0 LANGUAGES C CXX)

add_executable(marktex
    src/main.c
    src/lexer.c
    src/parser.c
    src/latex.c
)