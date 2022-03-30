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
// statement-> expression
//     | while 
//     | if
//     | for 
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
//            ">" | "+" | "-" | "*" | "/"
// 
// variable_decl -> identifier ":" type ( ("=" expression) | ";" )
// function_decl -> identifier "::" grouping "->" type block
// 
// identifier -> "A-Z" | "a-z" | "_"
// 
// type -> "*"? ( "u64" | "u32" | "u8" |
//                "s64" | "s32" | "s8" |
//                "int" | "string" | "bool" |
//                "void") 
// 
// TODO(ziv): implement these
// if -> "if" "(" expression ")" block
// while -> "while" "(" expression ")" block

#define _CRT_SECURE_NO_WARNINGS 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h> 

/* TODO(ziv):
    - Use a better algorithem for code generation (aka don't do the stupid things you are trying to do) 
    - Try to stay away from CRT stuff as much as possible or maybe even remove it alltogether (totaly possible but that would not be as convenient)
    - Create tests for the language 
    - Create a minimal standard libray (a super small one) for showcasing what the language is able to do (hopefully when it is working)
    - Create a programming enviorment where you can link to the compiler from a dll in which you would have one function which will generate a exe?
- More features? This could be seperated into a couple of sections where I could talk about it for a long ass time. Because I don't want to do 
      that, what I will do is specify a minimum feature set for the language where I know that if I implement those they would be enough for most 
      problems that are sort of crucial in a language to have. That said, this is a toy-language in which I don't plat to have anything close to 
      a fully featured or working compiler so I am not going to include advanced features like generics/macros/function pointers/
      compilated operations that will make my compiler more complex than it absolutely needs to be.
    - finish!!! This seems like an obvious one but I need to strees this out to myself that I want to at some point have a finished prodcut that 
      could use as such this is a requirement to not go down the rabbit hole of implementing things to obivion. I don't want to work on this 
      for a long time. It is not even supposed to be a show of skill just a project that I will do as a learning exercise. 
*/

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
        fprintf(stdout, "Usage: <source>.zr\n");
        return 0; 
    }
    char *filename = argv[1];
    
    // 
    // open the first file and read it's contents
    // 
    
    FILE *file = fopen(filename, "rb"); // I wnat to read text but if I do then the fread would not read the correct amount of bytes as it should. To fix this I am using "rb" instead of "rt" 
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
    
    token_stream.capacity = 1;
    token_stream.s = realloc(NULL, sizeof(Token));
    token_stream.start = source_buff;
    token_stream.current = 0;
    
    bool success = lex_file(&token_stream);
    if (success == false) return 0;
    
    Translation_Unit tu = {0}; 
    tu.block_stack.blocks[0] = realloc(NULL, sizeof(Block)); // init global scope 
    tu.block_stack.blocks[0]->locals = init_map(sizeof(Symbol));
    tu.block_stack.blocks[0]->statements = init_vec();
    tu.block_stack.index = 1;
    
    parse_translation_unit(&tu, &token_stream);
    sema_translation_unit(&tu);
    x86gen_translation_unit(&tu); 
    
    
    free(source_buff); // I don't need to use this but whatever...
    return 0;
}
