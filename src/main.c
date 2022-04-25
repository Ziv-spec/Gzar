//
// Language spec
// *  - at least one
// ?  - 0 or more times
// |  - expands the possible space
// () - groups
// "" - string which represents the 
//      combination of charatcters 
//      needed for a certain token 
//      in the language
//
// program -> decloration*
// 
// block -> "{" statement* "}"
//
// decloration -> variable_decl
//     | function_decl
//     | statement
// 
// statement -> expression
//     | while 
//     | if
//     | block
// 
// expression -> expression
//     | call
//     | unary
//     | binary
//     | grouping
//     | arguments
//     | assignment
//     | literal
// 
// literal -> number | string | "true" | "false" | "nil" 
// cast -> "cast" "(" type ")" expression
// call -> identifier "(" arguments ")"
// unary -> ("-" | "!" | cast ) expression
// grouping -> "(" expression ")"
// binary -> expression operator expression 
// operator -> "==" | "!=" | ">=" | "<=" | "<" |
//            ">" | "+" | "-" | "*" | "/" | "&" | 
//            "|" | "^" | "&&" | "||" | "%"
// 
// number -> "0-9"*
// string -> "
// variable_decl -> identifier ":" type ( ("=" expression) | ";" )
// function_decl -> identifier "::" grouping "->" return_type block
// 
// identifier -> "A-Z" | "a-z" | "_"
// 
// type -> "*"? ( "u64" | "u32" | "u8" |
//                "s64" | "s32" | "s8" |
//                "int" | "string" | "bool" |
//                "*void") 
// 
// return_type type | "void"
// 
// if -> ("if" expression block ) | ("if" expression block "else" block )
// while -> "while" expression block
//

#define _CRT_SECURE_NO_WARNINGS 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // probably I don't really need to use this and can have my own implementations for all the of the functions i use in here
#include <stdarg.h>

// TODO(ziv): maybe don't store the register in the expression node? 
// I think that I can just return the register itself and not have 
// conflicts with nodes type and more, which will allow me to have 
// a faster compiler and not need this bad decloration of the register 
// which I don't like. 

typedef struct Register Register; 
struct Register {
    int r; // register index itself
    int size; // register size in bytes
}; 

#include "base.h"
#include "lexer.h"
#include "sema.h"
#include "parser.h"

#include "lexer.c"
#include "sema.c"
#include "parser.c"
#include "x86.c"

int main(int argc, char *argv[]) {
    
    // 
    // handle the compiler options
    // 
    
    if (argc != 2) {
        fprintf(stdout, "Usage: <source>.gzr\n");
        return 0; 
    }
    
    char *filename = argv[1];
    
    // 
    // open the file and read it's contents
    // 
    
    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Error: failed to open file '%s'\n", filename);
    }
    
    fseek(file, 0, SEEK_END);
    int file_size = ftell(file);
    fseek(file, 0, SEEK_SET); 
    
    char *source_buff = (char*)calloc(file_size+1, sizeof(char));
    fread(source_buff, 1, file_size, file);
    source_buff[file_size] = '\0';
    
    fclose(file);
    
    
    //
    // Setup for compilation
    //
    
    Token_Stream token_stream;
    token_stream.capacity = 4*1024;
    token_stream.s = realloc(NULL, token_stream.capacity*sizeof(Token));
    token_stream.start = source_buff;
    token_stream.current = 0;
    
    CLOCK_START(LEXER);
    bool success = lex_file(&token_stream);
    if (success == false) 
        return 0;
    CLOCK_END(LEXER);
    
    CLOCK_START(PARSER);
    Translation_Unit tu = {0}; 
    parse_translation_unit(&tu, &token_stream);
    CLOCK_END(PARSER);
    
    CLOCK_START(SEMA);
    success = sema_translation_unit(&tu);
    CLOCK_END(SEMA);
    
    if (!success) {
        return 0;
    }
    
#if 0 
    char assembly_file_name[100] = { 0 }; 
    char *filename_start = filename; 
    bool dot_exists = false; 
    
    while (*filename) {
        if (filename[0] == '.' && filename[1]) {
            dot_exists = true; 
        }
        filename++; 
    }
    
    char *filename_end = filename; 
    u64 extention_len = 0;
    if (dot_exists) {
        while (*--filename != '.');
        extention_len = filename_end - filename;
        while (*--filename != '/' && filename > filename_start);
        if (*filename == '/')  filename++; 
        u64 filename_len = filename_end - filename;
        
        strncpy(assembly_file_name, filename, filename_len - extention_len);
        strcpy(assembly_file_name + filename_len-extention_len-1, ".asm");
        printf("\n");
        printf(assembly_file_name);
        printf("\n");
    }
    else {
        fprintf(stderr, "Error: invalid file extention please enter <source>.gzr\n");
    }
#endif 
    
    CLOCK_START(CODEGEN);
    x86gen_translation_unit(&tu, "test.asm"); 
    CLOCK_END(CODEGEN);
    
    // NOTE(ziv): calling the assembler and the linker for final assembly of the executable
    // This is very slow, and for the time being can not be avoided. One way to speed things 
    // up would be to not need assembler. This would require me to ouput x86-64 machine code
    // for the time being I am not doing that, if I see that it is worth it, I might try.
    
    char buff[255]; 
    sprintf(buff, "ml64 -nologo /c %s >nul && cl /nologo %s /link kernel32.lib msvcrt.lib", "test.asm", "test.obj");
    
    CLOCK_START(FINAL);
    system(buff);
    CLOCK_END(FINAL);
    
#if 1
    for (int i = 0; i < C_COUNT; i++) {
        printf("%s:    \t%lld\n", clock_names[i] , clock_counters[i]);
    }
#endif 
    
    free(source_buff); // I don't have to use this but whatever...
    
    return 0;
}