//
// Language spec (BNF - I need to do this): 
//
// program -> stmt*
// 
// stmt -> expression
//     | decloration
//     | function
//     | while 
//     | if
//     | for 
// 
// expression -> literal 
//     | unary
//     | binary 
//     | grouping
//     | assignment
// 
// literal -> NUMBER | STRING | "true" | "false" | "nil" 
// grouping -> "(" expression ")"
// unary -> ("-" | "!") expression
// binary -> expression operator expression 
// operator -> "==" | "!=" | ">=" | "<=" | "<" |
//            ">" | "+" | "-" | "*" | "/"
// 
// decloration -> identifier ":" type  ":" statement ";"  
// function -> identifier "::" grouping "->" type block
// 
// identifier -> "A-Z" | "a-z" | "_"
// 
// NOTE(ziv): For the time being I will now support 16 bit integers in any way
// type -> "int" | "u32" | "u8" |
//                 "s32" | "s8"
// block -> stmt
//
// if -> "if" "(" expression ")" block
// while -> "while" "(" expression ")" block

#define DEBUG 1
#define internal static 

#if DEBUG
#define Assert(cond) do { if (!(cond)) { __debugbreak(); } } while(0)
#else
#define Assert(cond) 
#endif

// TODO(ziv): restrucutre the lexer & parser such that I will not need this. 
typedef struct Location Location; 
struct Location { 
    int line; 
    int ch;
    int index; 
};

// 
// Some language definitions: 
// 

#include <stdint.h>

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float  f32; 
typedef double f64;

typedef unsigned char bool;

#define true 1 
#define false 0

#define is_digit(ch) ('0'<=(ch) && (ch)<='9')
#define is_alpha(ch) (('A'<=(ch) && (ch)<='Z') || ('a'<=(ch) && (ch)<='z'))
#define is_alphanumeric(ch) (is_digit(ch) || is_alpha(ch) || ch == '_')

#define ArrayLength(arr) (sizeof(arr)/sizeof(arr[0]))

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h> 

// TODO(ziv): implement a real mini string minipulation lib for this compiler

// NOTE(ziv): Deprecated
internal char *slicecpy(char *dest, size_t dest_size, char *src, size_t src_size) {
    Assert(src && dest);
    char *tdest = dest; 
    if (src_size > dest_size) return NULL;
    
    while (src_size) {
        *tdest++ = *src++; 
        src_size--;
    }
    *tdest = '\0';
    return dest;
}

internal int my_strcmp(char *str1, char *str2) {
    char *temp1 = str1, *temp2 = str2; 
    while (*temp1 && *temp2 && *temp1 == *temp2 && *temp1++ == *temp2++); 
    return *temp1 && *temp2;
    
}

#include "lexer.h"
#include "parser.h"
#include "codegen.h"

#include "lexer.c"
#include "parser.c"
#include "codegen.c"


int main(int argc, char *argv[]) {
    
    // 
    // handle the compiler options
    // 
    
#if 0
    if (argc != 2) {
        fprintf(stdout, "Usage: <source>.gz\n"); 
        return 0; 
    }
    char *filename = argv[1];
#else 
    argc, argv;
    
    char *filename = "C:/dev/HW/toylang/tests/test2.gzr";
    
#endif 
    
    
    // 
    // open the first file and read it's contents
    // 
    
    FILE *file = fopen(filename, "rt");
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
    
    init_tk_names();
    init_keywords();
    
    Lexer lexer = {0};
    lexer.code = source_buff;
    // most code editors begin from one and not zero e.g. row = 1, col = 1
    lexer.loc.ch   = 1; 
    lexer.loc.line = 1; 
    
    bool success = lex_file(&lexer);
    if (success == false) return 0;
    
    parse_file(); 
    
    // gen();
    
    
    free(source_buff); // I don't need to use this but whatever...
    return 0;
}