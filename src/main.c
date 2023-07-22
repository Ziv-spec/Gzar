
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

#include "lexer.c"
#include "parser.c"
#include "sema.c"
#include "x86_asm.c"// super old yet working x64 assmebly backend

#include "x64.c"    // new x64 emitter backend

#pragma warning(disable : 4431 4267 4456 4244 4189)
#define MICROSOFT_CRAZINESS_IMPLEMENTATION
#include "microsoft_crazyness.h"
#pragma warning(default: 4431 4267 4456 4244 4189)

#include "pe.c"

internal Value_Operand e_label(Builder *b, char *label) {
	Patch_Location *pl = &b->pls[PL_LABELS][b->pls_cnt[PL_LABELS]];
	*pl = (Patch_Location){ .name = label, .rva = b->bytes_count };
	return (Value_Operand){ .kind = LAYOUT_V, .imm = PL_LABELS << 28 | (b->pls_cnt[PL_LABELS]++) };
}

internal Value_Operand e_lit(Builder *b, char *literal) {
	Patch_Location *pl = &b->pls[PL_DATA_VARS][b->pls_cnt[PL_DATA_VARS]];
	*pl = (Patch_Location){ .name = literal, .rva = b->current_data_variable_location };
	b->current_data_variable_location += (int)(strlen(literal)+1);
	return (Value_Operand){ .kind = LAYOUT_V, .imm = PL_DATA_VARS << 28 | (b->pls_cnt[PL_DATA_VARS]++) };
}

internal Value_Operand e_cfunction(Builder *b, char *label) {
	Patch_Location *pl = &b->pls[PL_C_FUNCS][b->pls_cnt[PL_C_FUNCS]];
	*pl = (Patch_Location){ .name = label };
	return (Value_Operand){ .kind = PL_C_FUNCS, .imm = PL_C_FUNCS<< 28 | (b->pls_cnt[PL_C_FUNCS]++) };
}

int main() {
	
	char program[0x1000] = { 0 };
	Patch_Location labels[0x50] = {0}; 
	Patch_Location data_variables[0x50] = {0}; 
	Patch_Location c_funcs[0x50] = {0}; 
	
	Builder builder = { 
		.m = { .pool_size = 0x1000 },
		.pls = {
			[PL_LABELS]    = labels, 
			[PL_DATA_VARS] = data_variables, 
			[PL_C_FUNCS]   = c_funcs,
		},
		.code = program, // program buffer
	};
	pools_init(&builder.m);
	
	#if 0
	Inst inst = (Inst){ADD};
	Value_Operand v1 = { .kind = LAYOUT_R, .reg = RAX };
	Value_Operand v2 = { .kind = LAYOUT_R, .reg = R8  };
	inst2(&builder, &inst, &v1, &v2, 8);
	
	inst = (Inst){CALL};
	Value_Operand v = e_label(&builder, ".L0");
	inst1(&builder, &inst, &v, DT_DWORD);
	
	inst = (Inst){ADD};
	v1 = (Value_Operand){ .kind = LAYOUT_R, .reg = RAX };
	v2 = e_lit(&builder, "Hello World!");
	inst2(&builder, &inst, &v1, &v2, 8);
	
	
	
	// patching addresses
	Patch_Location *pls;
	
		pls = builder.pls[PL_LABELS];
	for (int i = 0; i < builder.pls_cnt[PL_LABELS]; i++) {
		Patch_Location pl = pls[i]; pl.rva += 0x1000;
		memcpy(&builder.code[pl.location], &pl.rva, sizeof(pl.rva)); 
	}
	
	pls = builder.pls[PL_DATA_VARS];
	for (int i = 0; i < builder.pls_cnt[PL_DATA_VARS]; i++) {
		Patch_Location pl = pls[i]; pl.rva += 0x3000;
		memcpy(&builder.code[pl.location], &pl.rva, sizeof(pl.rva)); 
	}
#endif 
	
	Inst inst = (Inst){MOV};
	Value_Operand v1 = { .kind = LAYOUT_R, .reg = RAX };
	Value_Operand v2 = { .kind = LAYOUT_I, .imm = 0x00 };
	inst2(&builder, &inst, &v1, &v2, 8);
	
	
	//test_x64_unary(&builder, NOT);
	//test_x64_binary(&builder, ADD);
	
	for (int i = 0; i < builder.bytes_count; i++) { printf("%02x", builder.code[i]&0xff); } printf("\n"); 
	
	
	
	#if 0
	// NOTE(ziv): THIS WORKS!!! The only feature I need to do is finish up with patching locations (which is easy) 
	
	//~
	// list of external library paths e.g. kernel32.lib (in linux libc.so)
	
	// TODO(ziv): remove this 
	char kernel32[] = "kernel32.lib";
	char user32[]   = "user32.lib";
	char *libs[] = { kernel32, user32 };
	
	write_pe_exe(&builder, "test.exe", libs, ArrayLength(libs)); 
	
#endif 
	
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
