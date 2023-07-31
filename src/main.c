
/* 

 create optmizer!!!!! This will be crazy if I could manage it. 
although I want an optimizer, I will have to look into the following too: 

TODO(ziv): 
1. finish x64 backend
2. finish pe.c 
3. reform the microsoft_crazyness.h to be msvc.c without the header thingy and so on...
4. create a linker which will accept multiple platforms/formats
5. create a better base layer, I want to review the design to see how I can improve and make my life going forward easier. 
6. make lexer slightly faster/better
7. recreate parser with new design

that said, I plan on doing the optimizer pretty early since I want to have a proven method before I incorprate it furthur into the design of my compiler. 

*/

//#pragma comment(lib, "user32.lib")
#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "OleAut32.lib")

#define _CRT_SECURE_NO_WARNINGS 1
#include <stdio.h>  // sprintf, fopen, fclose
#include <stdlib.h> // malloc realloc and free
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

#define TIMINGS 0


#define VC_EXTRALEAN
#include <windows.h>
//#include <winnt.h>

#include "base.h"
#include "lexer.h"
#include "sema.h"
#include "parser.h"

// frontend
#include "lexer.c"
#include "parser.c"
#include "sema.c"

// backend
#include "x86_asm.c"
#include "x64.c"
#pragma warning(disable : 4431 4267 4456 4244 4189)
#define MICROSOFT_CRAZINESS_IMPLEMENTATION
#include "microsoft_crazyness.h"
#pragma warning(default: 4431 4267 4456 4244 4189)

#include "pe.c"


int main() {
	
	char program[0x1000] = { 0 };
	
	Builder builder = { 
		.m = { .pool_size = 0x4000 },
		.code = program, // program buffer
	};
	
	// TODO(ziv): maybe put initialization into a function instead?
	pools_init(&builder.m);
	
	builder.pls_sz = 0x100; 
	builder.pls = pool_alloc(&builder.m, builder.pls_sz* sizeof(Patch_Locations));
	
	// NOTE(ziv): I don't have a better way of doing this since I am bad at tooling
	for (int i = 0; i < PL_COUNT; i++) {
		builder.pls_maps[i] = (Map){
			.buckets = pool_alloc(&builder.m, DEFAULT_MAP_SIZE*sizeof(Bucket)), 
			.capacity = DEFAULT_MAP_SIZE,
		};
	}
	
	
	
	
	
	
	Value_Operand l0 = e_label(&builder, ".L0");
	
	Inst inst = (Inst){MOV};
	Value_Operand v1 = { .kind = LAYOUT_R, .reg = RAX };
	Value_Operand v2 = { .kind = LAYOUT_I, .imm = 0x00 };
	inst2(&builder, &inst, &v1, &v2, 8);
	
	inst = (Inst){JMP};
	v1 = (Value_Operand){ .kind = LAYOUT_R, .reg = RAX };
	inst1(&builder, &inst, &l0, 4);
	
	Value_Operand l1 = e_label(&builder, ".L1");
	inst = (Inst){MOV};
	inst2(&builder, &inst, &v1, &v2, 8);
	
	inst = (Inst){JMP};
	v1 = (Value_Operand){ .kind = LAYOUT_R, .reg = RAX };
	inst1(&builder, &inst, &l1, 4);
	
	inst = (Inst){MOV};
	Value_Operand lit1 = e_lit(&builder, "Hello World!");
	inst2(&builder, &inst, &v1, &lit1, 4);
	Value_Operand lit2 = e_lit(&builder, "This is a 64b PE executable!");
	inst2(&builder, &inst, &v1, &lit2, 4);
	
	// TODO(ziv): add cfunc patching 
	// TODO(ziv): create a general function for patching addresses for the 'linker'
	
	// patching the label's addresses
	Map_Iterator it = map_iterator(&builder.pls_maps[PL_LABELS]);
	while (map_next(&it)) {
		Patch_Locations *pls = it.value;
		for (int i = 0; i<  pls->loc_cnt; i++) {
			int loc = pls->loc[i], rva = pls->rva - (loc+4);
			memcpy(&builder.code[loc], &rva, sizeof(rva));
		}
	}
	
	// patching the global variables addresses (in this case mostly strings I guess)
	it = map_iterator(&builder.pls_maps[PL_DATA_VARS]);
	while (map_next(&it)) {
		Patch_Locations *pls = it.value;
		for (int i = 0; i<  pls->loc_cnt; i++) {
			int loc = pls->loc[i], rva = pls->rva + 0x3000;
			memcpy(&builder.code[loc], &rva, sizeof(rva));
		}
	}
	
	// patching the global variables addresses (in this case mostly strings I guess)
	it = map_iterator(&builder.pls_maps[PL_C_FUNCS]);
	while (map_next(&it)) {
		Patch_Locations *pls = it.value;
		for (int i = 0; i<  pls->loc_cnt; i++) {
			int loc = pls->loc[i], rva = pls->rva + 0x4000;
			memcpy(&builder.code[loc], &rva, sizeof(rva));
		}
	}
	
	
	
	
	
	
	//test_x64_unary(&builder, NOT);
	//test_x64_binary(&builder, ADD);
	
	
	
	#if 1
	// NOTE(ziv): THIS WORKS!!! The only feature I need to do is finish up with patching locations (which is easy) 
	
	//~
	// list of external library paths e.g. kernel32.lib (in linux libc.so)
	
	// TODO(ziv): remove this 
	char kernel32[] = "kernel32.lib";
	char user32[]   = "user32.lib";
	char *libs[] = { kernel32, user32 };
	
	write_pe_exe(&builder, "test.exe", libs, ArrayLength(libs)); 
	
#endif 
	
	for (int i = 0; i < builder.bytes_count; i++) { printf("%02x", builder.code[i]&0xff); } printf("\n"); 
	
	pool_free(&builder.m);
	
	return 0;
}




#if 0
int main(int argc, char *argv[]) {
	//~
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
		return 0;
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
	
	Token_Stream token_stream = {
		.capacity = 0x1000, // some default value for the time being a page size
		.s = realloc(NULL, 0x1000*sizeof(Token)), 
		.start = source_buff, 
	};
	
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
	if (success == false)
		return 0;
	CLOCK_END(SEMA);
	
	// NOTE(ziv): Here I am outputing assembly, for which I am using sprintf. This is a
	// very slow function. It takes up around 70% of total runtime cost for this whole
	// function. This is after I ran it on a test with 181 total calls for sprintf.
	
	CLOCK_START(CODEGEN);
	gen_translation_unit(&tu, "test.asm");
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
	
#if TIMINGS
	
	LARGE_INTEGER _freq; 
	QueryPerformanceFrequency(&_freq);
	
	float freq = (float)_freq.QuadPart;
	for (int i = 0; i < C_COUNT; i++) {
		printf("%s:    \t%fs\n", clock_names[i] , clock_counters[i]/freq);
	}
	
	float total = 0.f; 
	for (int i = 0; i < C_COUNT; i++) total += clock_counters[i];
	printf("total:    \t%fs\n", total/freq);
	
#endif
	
	free(source_buff); // I don't have to use this but whatever...
	
	return 0;
}
#endif 
