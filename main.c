//
// Language spec: 
// 
// a bit like a calc that is less fancy :)

//
// Right now I will be working on a prefix implementation of a simple adder
//

#define DEBUG 1
#define internal static 

#if DEBUG
#define Assert(expression) if (!expression) { int expr = *(int *)0; }
#else
#define Assert(expression) 
#endif 



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

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32; 
typedef double f64;

#define true 1 
#define false 0.
typedef char bool;  // should this be a char??? 

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO(ziv): implement a real mini string minipulation lib for this compiler
char *slicecpy(char *dest, size_t dest_size, char *src, size_t src_size) {
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

#include "lexer.h"
#include "parser.h"



int main(int argc, char *argv[]) {
    
    // 
    // handle the compiler options
    // 
#if 0
    if (argc != 2) {
        fprintf(stderr, "Usage: <source>.gz\n"); 
        return 0; 
    }
    char *filename = argv[1];
#else 
    argc, argv;
    char *filename = "../tests/add.gzr";
#endif 
    
    
    // 
    // open the first file and read it's contents
    // 
    
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: failed to open file '%s'\n", filename);
    }
    
    fseek(file, 0, SEEK_END);
    int file_size = ftell(file);
    fseek(file, 0, SEEK_SET); 
    
    char *source_buff = (char*)malloc(file_size+1); 
    fread(source_buff, 1, file_size, file);
    source_buff[file_size] = '\0';
    
    fclose(file);
    
    
    //
    // Setup for compilation
    //
    
    init_tk_names();
    
    Lexer lexer = {0};
    lexer.code = source_buff;; 
    // most code editors begin from one and not zero e.g. row = 1, col = 1
    lexer.loc.ch   = 1; 
    lexer.loc.line = 1; 
    
    Parser parser = {0};
    parser.lexer = &lexer; 
    
    
    // 
    // Compilation/Simulation phase
    // 
    
    // parse_file(&parser);
    sim_file(&parser);
    
    
    
    free(source_buff); // I don't need to use this but whatever...
    return 0;
}