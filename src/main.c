/* 
NOTE: Reflection on the project 12/9/23

# Pain points working on the project
I do see that I am suffering greatly from a poor base layer since 
everything else is being built on top of it. For that reason, it might make sense 
to advance the base layer before continuing on with the rest of the compiler infestructure. 

This might include introducing stb arrays since I have a great need for arrays and C arrays 
are kind of hard to work with (This is of course if I don't at some point switch to CPP).
 Of course threading will be crucial in the future, depending on the stage that I would want 
to introduce threads to the compiler. 

I also need a "platform" layer. I began noticing that certain os operations have become 
more and more freqently used. These operantions should not be platform specific as they 
are currently. For this reason I have concluded the need for a basic platform api. 
This is not without it's challenges. Especially because of my unfamiliarity with linux api.

# Changes base on pain
All these changes basically mean a complete rewrite of the compiler as a whole. It should 
also allow me to get a really solid design since I now understand the problem better.
This rewrite will use many of the currently existing components and pieces of code I have 
already written but should take a new refreshed form. It will include all my insights 
for when creating the current testing langauge "GZAR".
*/

//#pragma comment(lib, "user32.lib")
#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "OleAut32.lib")

// TODO(ziv): REMOVE BORING DEPENDENCIES (for release, debug is fine)
#define _CRT_SECURE_NO_WARNINGS 1
#include <stdio.h>  // sprintf, fopen, fclose
#include <stdlib.h> // malloc realloc and free
#include <stdarg.h>

// TODO(ziv): Clearly bad design here
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

#include "lexer.c"
#include "parser.c"
#include "sema.c"
#include "ir.c"
//#include "bb.c"
#include "x86_asm.c"
#include "x64.c"
#include "microsoft_crazyness.h" // TODO(ziv): rename
#include "pe.c"

#if 1
#pragma warning(disable: 4702) // TODO(ziv): remove this
int main() {
	

/* 

	// open the file and read it's contents
	char *filename = "../tests/test3.gzr";
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
	fclose(file);
	source_buff[file_size] = '\0';
	
	printf("%s\n\n", source_buff);
	
	// Setup for compilation
	Translation_Unit tu = {0};
	Token_Stream token_stream = {
		.capacity = 0x1000, // some default value for the time being a page size
		.s = realloc(NULL, 0x1000*sizeof(Token)), 
		.start = source_buff, 
	};
	
	bool success = lex_file(&token_stream);
	if (success == false) return 0;
	parse_translation_unit(&tu, &token_stream);
	success = sema_translation_unit(&tu);
	if (success == false) return 0;
	
	IR_Buffer ir_buffer = { &tu.m };
	 Vector *functions = convert_ast_to_ir(&tu, &ir_buffer);
	print_ir(functions);
	
	
	return 0;
 */
	 

	//
	// Initialization
	//
	
	char program[0x1000] = { 0 };
	Builder builder = { 
		.m = { .pool_size = 0x4000 },
		.code = program, // program buffer
	};
	
	// TODO(ziv): maybe put initialization into a function instead?
	pools_init(&builder.m);
	
	// NOTE(ziv): I don't have a better way of doing this since I am bad at tooling
	for (int i = 0; i < PL_COUNT; i++) {
		builder.pls_maps[i] = (Map){
			.buckets = pool_alloc(&builder.m, DEFAULT_MAP_SIZE*sizeof(Bucket)), 
			.capacity = DEFAULT_MAP_SIZE,
		};
	}
	
	
	
	
	// 
	// Codegen 
	//

/* 	
	Value_Operand l0 = e_label(&builder, ".L0");
	
	Inst inst = (Inst){MOV};
	Value_Operand v1 = { .kind = LAYOUT_R, .reg = RAX };
	Value_Operand v2 = { .kind = LAYOUT_I, .imm = 0x00 };
	inst2(&builder, &inst, &v1, &v2, 8);
	
	inst = (Inst){JMP};
	inst1(&builder, &inst, &l0, 4);
	
	Value_Operand l1 = e_label(&builder, ".L1");
	inst = (Inst){MOV};
	inst2(&builder, &inst, &v1, &v2, 8);
	
	inst = (Inst){JMP};
	 inst1(&builder, &inst, &l1, 4);
	 */

	Inst inst;
	
	Value_Operand rcx = { .kind = LAYOUT_R, .reg = RCX };
	Value_Operand rdx = { .kind = LAYOUT_R, .reg = RDX };
	Value_Operand r8  = { .kind = LAYOUT_R, .reg = R8 };
	Value_Operand r9  = { .kind = LAYOUT_R, .reg = R9 };
	Value_Operand zero = { .kind = LAYOUT_I, .imm = 0 };
	
	Value_Operand lit1 = e_lit(&builder, "Hello World!");
	Value_Operand lit2 = e_lit(&builder, "This is a 64b PE executable!");
	
	inst = (Inst){SUB};
		Value_Operand rsp = { .kind = LAYOUT_R, .reg = RSP }; 
	Value_Operand v28 = { .kind = LAYOUT_I, .imm = 0x28}; 
	inst2(&builder, &inst, &rsp, &v28, 8);
	
	inst = (Inst){MOV};
	inst2(&builder, &inst, &r9,  &zero, 4); 
	inst2(&builder, &inst, &r8,  &lit2, 4); 
	inst2(&builder, &inst, &rdx, &lit1, 4); 
	inst2(&builder, &inst, &rcx, &zero, 4); 
	
	inst = (Inst){CALL};
	Value_Operand messageboxa = e_cfunction(&builder, "MessageBoxA");
	inst1(&builder, &inst, &messageboxa, 4);
	
	inst = (Inst){MOV}; 
	inst2(&builder, &inst, &rcx, &zero, 4);
	
	inst = (Inst){CALL};
	Value_Operand exit_process = e_cfunction(&builder, "ExitProcess");
	inst1(&builder, &inst, &exit_process, 4);
	
	
	//
	// gen .data buffer 
	// 
	
	// TODO(ziv): don't allocate with pool_alloc, this could be a large allocation!
	builder.data = pool_alloc(&builder.m, builder.current_data_variable_location); 
	char *data = builder.data; 
	for (int i = 0; i < builder.data_vars_cnt; i++) {
		String8 lit = builder.data_vars[i];
		memcpy(data, lit.str, lit.size+1);
		data += lit.size+1;
	}
	
	// TODO(ziv): create a general function for patching addresses for the 'linker' since this is it's job for the most part
	
	// 
	// Address patching 
	//
	
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
		for (int i = 0; i < pls->loc_cnt; i++) {
			int loc = pls->loc[i], rva = pls->rva + 0x3000 + 0x400000;
			memcpy(&builder.code[loc], &rva, sizeof(rva));
		}
	}
	
	//test_x64_unary(&builder, NOT);
	//test_x64_binary(&builder, ADD);
	
	
	
	
#if 1
	// NOTE(ziv): THIS WORKS!!! The only feature I need to do is finish up with patching locations (which is easy) 
	
	//~
	// TODO(ziv): remove this and make automatic
	char kernel32[] = "kernel32.lib";
	char user32[]   = "user32.lib";
	char *libs[] = { kernel32, user32 };
	
	write_pe_exe(&builder, "test.exe", libs, ArrayLength(libs)); 
	
#endif 
	
	for (int i = 0; i < builder.bytes_count; i++) { printf("%02x", builder.code[i]&0xff); } printf("\n"); 
	
	pool_free(&builder.m);
	
	return 0;
}
#endif 



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
