//
// Language spec (I might remake this in the BNF): 
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
//            ">" | "+" | "-" | "*" | "/" | "&" | 
//            "|" | "^" | "&&" | "||"
// 
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
// if -> "if" expression block
// while -> "while" expression block

#define _CRT_SECURE_NO_WARNINGS 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // probably I don't really need to use this and can have my own implementations for all the of the functions i use in here
#include <stdarg.h> 
#include <time.h>

/* TODO(ziv):
    - Use a better algorithem for code generation (aka don't do the stupid things you are trying to do) 
    - Try to stay away from CRT stuff as much as possible or maybe even remove it alltogether (totaly possible but that would not be as convenient)
    - Create a minimal standard libray (a super small one) for showcasing what the language is able to do (hopefully when it is working)
- More features? This could be seperated into a couple of sections where I could talk about it for a long ass time. Because I don't want to do 
      that, what I will do is specify a minimum feature set for the language where I know that if I implement those they would be enough for most 
      problems that are sort of crucial in a language to have. That said, this is a toy-language in which I don't plat to have anything close to 
      a fully featured or working compiler so I am not going to include advanced features like generics/macros/function pointers/
      compilated operations that will make my compiler more complex than it absolutely needs to be.
    - finish!!! This seems like an obvious one but I need to strees this out to myself that I want to at some point have a finished prodcut that 
      could use as such this is a requirement to not go down the rabbit hole of implementing things to obivion. I don't want to work on this 
      for a long time. It is not even supposed to be a show of skill just a project that I will do as a learning exercise. 
*/

typedef struct Register Register; 
struct Register {
    int r; // register index itself
    int size; // register size in bytes
}; 

// NOTE(ziv): This is only getting used
#define STB_SPRINTF_IMPLEMENTATION
#define STB_SPRINTF_NOFLOAT

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
    // open the first file and read it's contents
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
    
    
    // NOTE(ziv): just as a note, this compiler does not free any of it's memory 
    // it instead lets the operating system free all of the resoruces of the process 
    // this is because I expect the compiler to be running for a short time and do only 
    // what it needs to do. 
    // This shall change in the future once I implement my unified-memory-pool 
    // implementation of a custom allocator. Which will allocate in pools of memory
    
    //
    // Setup for compilation
    //
    
    // TODO(ziv): Make this nicer
    
    Token_Stream token_stream;
    token_stream.capacity = 1;
    token_stream.s = realloc(NULL, sizeof(Token));
    token_stream.start = source_buff;
    token_stream.current = 0;
    
    //u64 begin = __rdtsc();
    bool success = lex_file(&token_stream);
    if (success == false) 
        return 0;
    //printf("lexing: \t%lld\n", __rdtsc()-begin);
    
    //begin = __rdtsc();
    Translation_Unit tu = {0}; 
    parse_translation_unit(&tu, &token_stream);
    //printf("parsing: \t%lld\n", __rdtsc()-begin);
    
    //begin = __rdtsc();
    success = sema_translation_unit(&tu);
    //printf("sema:     \t%lld\n", __rdtsc()-begin);
    
    if (!success) {
        return 0;
    }
    //begin = __rdtsc();
    x86gen_translation_unit(&tu, "test.asm"); 
    //printf("codegen: \t%lld\n", __rdtsc()-begin);
    
    
    // NOTE(ziv): calling the assembler and the linker for final assembly of the executable
    // This is very slow, and for the time being can not be avoided. One way to speed things 
    // up would be to not need assembler. This would require me to ouput x86-64 machine code
    // for the time being I am not doing that, if I see that it is worth it, I might try.
    //begin = __rdtsc();
    system("ml64 -nologo /c test.asm >nul && cl /nologo test.obj /link kernel32.lib msvcrt.lib"); 
    //printf("final:    \t%lld\n", __rdtsc()-begin);
    free(source_buff); // I don't have to use this but whatever...
    return 0;
}
