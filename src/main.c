
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

#pragma warning(disable : 4431 4267 4456 4244 4189)
#define MICROSOFT_CRAZINESS_IMPLEMENTATION
#include "microsoft_crazyness.h"
#pragma warning(default: 4431 4267 4456 4244 4189)

#include "x86.c"    // last version of the x64 backend (which will eventually get phased out)
#include "x64.c"    // new backend 
#include "pe.c"


#if 0 
internal Value_Operand lk_c_function(Builder *builder, char *function) {
	Operand result = { Val, .value = builder->jumpinstructions_count | (JUMPINSTRUCTIONS_TABLE_KIND << 28)};
	Name_Location *v = &builder->jumpinstructions[builder->jumpinstructions_count++];
	v->location = 0; // NOTE(ziv): @Incomplete the location should get reset here at a later date
	v->name = function;
	return result;
}

internal Value_Operand lk_label(Builder *builder, const char *label) {
	Operand result = { Val, .value = builder->labels_count | (LABELS_TABLE_KIND << (32-4)) };
	Name_Location *v = &builder->labels[builder->labels_count++];
	v->location = builder->bytes_count;
	v->name = (char *)label;
	return result;
}

internal Value_Operand lk_lit(Builder *builder, const char *literal) {
	Operand result = { Val, .value = builder->data_variables_count | (DATA_VARIABLES_TABLE_KIND << 28) };
	Name_Location *v = &builder->data_variables[builder->data_variables_count++];
	v->location = builder->current_data_variable_location;
	builder->current_data_variable_location += (int)strlen(literal)+1;
	v->name = (char *)literal;
	return result;
}

internal Name_Location *lk_get_name_location_from_value(Builder *builder, Value v) {
	Name_Location *nl_table[4] = { builder->labels , builder->jumpinstructions, builder->data_variables };
	int idx  = v.idx & ((1<<28)-1);
	int type = v.idx >> 28;
	return &nl_table[type][idx];
}

internal Value_Operand lk_function(Builder *builder, const char *module, const char *function) {
	Operand result = { Val, .value = builder->jumpinstructions_count | (JUMPINSTRUCTIONS_TABLE_KIND << (32-4)) };
	
	Name_Location *v = &builder->jumpinstructions[builder->jumpinstructions_count++];
	v->location = 0;
	v->name = (char *)function;
	
	module; function;
	return result;
}
#endif 


#if 1
int main() {
	
	char program[0x1000] = { 0 };
	Name_Location labels[0x50] = {0}; 
	Name_Location data_variables[0x50] = {0}; 
	Name_Location jumpinstructions[0x50] = {0}; 
	
	Builder builder = { 
		.code = program, 
		
		// init tables
		.labels = labels,
		.data_variables = data_variables,
		.jumpinstructions = jumpinstructions,
		
		.m = { .pool_size = 0x1000 }
	};
	pools_init(&builder.m);
	
	
	
	
	/* 
		Inst inst;
		inst = (Inst){RET};
		inst0(&builder, &inst); 
		*/
	
	 //test_x64_unary(&builder, NOT);
	test_x64_binary(&builder, ADD);
	
	for (int i = 0; i < builder.bytes_count; i++) { printf("%02x", builder.code[i]&0xff); } printf("\n"); 
	
	
	
	#if 0
	// NOTE(ziv): THIS WORKS!!! The only feature I need to do is finish up with patching locations (which is easy) 
	Operand pe_exe_lit      = x86_lit(&builder, "a simple 64b PE executable");
	Operand hello_world_lit = x86_lit(&builder, "Hello World!");
	
	Operand message_box_a = x86_c_function(&builder, "MessageBoxA");
	Operand exit_process  = x86_c_function(&builder, "ExitProcess");
	
	x86_encode(&builder, sub,  REG(RSP), IMM(0x28));
	x86_encode(&builder, mov,  REG(R9D), IMM(0));
	x86_encode(&builder, mov,  REG(R8D), IMM(0x403000));
	x86_encode(&builder, mov,  REG(EDX), IMM(0x40301b));
	x86_encode(&builder, mov,  REG(ECX), IMM(0));
	x86_encode(&builder, call, MEM(0,0,0,0x402088,0), NO_OPERAND);
	x86_encode(&builder, mov,  REG(ECX), IMM(0));
	x86_encode(&builder, call, MEM(0,0,0,0x402078,0), NO_OPERAND);
	
	// TODO(ziv): remove this 
	char kernel32[] =  "kernel32.lib";
	char user32[]   =  "user32.lib";
	char *libs[] = { kernel32, user32 };
	builder.external_library_paths = libs; 
	builder.external_library_paths_count = ArrayLength(libs);
	
	write_pe_exe(&builder, "test.exe"); 
	
	#endif 
	
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
