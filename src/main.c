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

#define TIMINGS 1


#define VC_EXTRALEAN
#include <windows.h>

#include "base.h"
#include "lexer.h"
#include "sema.h"
#include "parser.h"

#include "lexer.c"
#include "parser.c"
#include "sema.c"

#define MICROSOFT_CRAZINESS_IMPLEMENTATION
#include "microsoft_crazyness.h"

#include "x86_asm.c"
#include "x86.c"

#include "pe.c"



#if 1
int main() {
	
	
	
	
	
	
	
	
	
	//~
	// Generate Code
	//
	
	
	/* template for cool api for encoding instructions regardless of backend type
Value lab = label(".L1"); 
encode(builder, jmp, lab, NO_OPERAND); 

Value func = function("Function");
		encode(builder, call, func, NIL);
		
Value exit_process = c_function("kernel32.dll", "ExitProcess"); 
		encode(builder, call, exit_process, NO_OPERAND);

Value str = lit("some string");
encode(builder, mov, lit("some string"), NO_OPERAND); 

encode(builder, add, REG(RAX), IMM(0x10));
		 */
	
	
	char program[0x60] = { 0 };

	Name_Location labels[0x50] = {0}; 
	Name_Location data_variables[0x50] = {0}; 
	Name_Location jumpinstructions[0x50] = {0}; 

	Builder builder = { 
		.code = program, 
		.current_instruction = program,
		
		.jumpinstructions = jumpinstructions,
		.labels = labels,
		.data_variables = data_variables,
		
		.vs_sdk = find_visual_studio_and_windows_sdk()
		
	};
	
	
	char *dll_name = is_function_in_module(&builder, "kernel32.lib", "ExitProcess");
	printf("dll name: %s\n", dll_name);
	free(dll_name); 
	
	
/* 	
	encode(&builder, sub,  REG(RSP), IMM(0x28));
	encode(&builder, mov,  REG(R9D), IMM(0));
	encode(&builder, mov,  REG(R8D), IMM(0x403000));
	encode(&builder, mov,  REG(EDX), IMM(0x40301b));
	encode(&builder, mov,  REG(ECX), IMM(0));
	encode(&builder, call, MEM(0,0,0,0x402088,0), NO_OPERAND);
	encode(&builder, mov,  REG(ECX), IMM(0));
	encode(&builder, call, MEM(0,0,0,0x402078,0), NO_OPERAND);
	 */

	// I believe this works? 

	/* 	
		Value v = x86_label(&builder, ".L1");
		v = x86_lit(&builder, "some data thingy that I might want to put there"); 
		v = x86_lit(&builder, "one more string");
		 
	// this is a more internal thingy 
	Name_Location *name = x86_get_name_location_from_value(&builder, v);
	*/
	
	x86_encode(&builder, mov, REG(R8D), x86_lit(&builder, "some string"));
	x86_encode(&builder, mov, REG(R9),  x86_label(&builder, ".L1"));
	x86_encode(&builder, mov, REG(R8D), x86_lit(&builder, "some"));

/* 	
	Operand op = x86_lit(&builder, "some string");
	encode(&builder, mov, REG(R8D), op);
	 */

	
	// patch all the paths inside the program using whatever is needed 
	//int base = 0; // base address to be added to all relative locations? 
	// or maybe I should just use relative locations for all of those things? 
	// this is a good question to be asked of how to handle it.

/* 	
	printf("Before: ");  
	for (int i = 0; i < bytes_count; i++) { printf("%02x", builder.code[i]&0xff); } printf("\n"); 
	 */

	// maybe I should just use relative locations for all of those things? 
	// this is a good question to be asked of how to handle it.
	int base = 0x4000;
	for (int i = 0; i < builder.data_variables_count; i++) {
		Name_Location nl = builder.data_variables[i];
		int location = nl.location + base;
		memcpy(&program[nl.location_to_patch], &location, sizeof(nl.location));
	}
	
	 base = 0x1000;
	for (int i = 0; i < builder.labels_count; i++) {
		Name_Location nl = builder.labels[i];
		int location = nl.location + base;
		memcpy(&program[nl.location_to_patch], &location, sizeof(nl.location));
	}
	
	// this might be more specific? 
	base = 0x1000;
	for (int i = 0; i < builder.jumpinstructions_count; i++) {
		Name_Location nl = builder.jumpinstructions[i];
		int location = nl.location + base;
		memcpy(&program[nl.location_to_patch], &location, sizeof(nl.location));
	}

/* 	 
	printf("After: ");
	for (int i = 0; i < bytes_count; i++) { printf("%02x", builder.code[i]&0xff); } printf("\n"); 
	 */

	free_resources(&builder.vs_sdk);
	
	
	//~
	
	// text section info
/* 
	char *code = (char*)instructions; 
	unsigned int code_size = bytes_count;
 */

/* 
	// without image dos stub
		char code[] = {
			0x48, 0x83, 0xec, 0x28,
			0x41, 0xb9, 0, 0, 0, 0,
			0x41, 0xb8, 0, 0x30, 0x40, 0, 
			0xba, 0x1b, 0x30, 0x40, 0,
			0xb9, 0, 0, 0, 0,
			0xff, 0x14, 0x25, 0x88, 0x20, 0x40, 0,
			0xb9, 0, 0, 0, 0,
			0xff, 0x14, 0x25, 0x78, 0x20, 0x40, 0, 
		};
 */

	// data section info
	char hello_world[]   = "Hello World!";
	char simple_pe_exe[] = "a simple 64b PE executable";
	
	char data[sizeof(hello_world) + sizeof(simple_pe_exe)], *pdata = data;
	MOVE(pdata, simple_pe_exe, sizeof(simple_pe_exe)); 
	MOVE(pdata, hello_world,   sizeof(hello_world)); 
	
	// idata section info, this defines functions you want to import & their respective dll's
	Entry entries[] = {
		{"kernel32.dll", (char *[]){ "ExitProcess", 0 }},
		{"user32.dll",   (char *[]){ "MessageBoxA", 0 }},
	};
	
	builder.data = data; 
	builder.data_size = sizeof(data); 
	//builder.code = code; 
	//builder.code_size = sizeof(code); 
	builder.code = program; 
	builder.code_size = sizeof(program); 
	//printf("%d\n", builder.code_size); 
	
	builder.entries = entries; 
	builder.entries_count = ArrayLength(entries);
		
	write_pe_exe("test.exe", &builder); 
	
	return 0;
}
#endif




/* 
	// register to register
	encode(builder, xor, REG(RAX), REG(RCX));
	encode(builder, xor, REG(EAX), REG(ECX));
	encode(builder, xor, REG(DX), REG(AX));
	encode(builder, xor, REG(CL), REG(AL)); 
	
	// register to memeory 
	encode(builder, xor, MEM(RAX,0,0,0,0), REG(RCX));
	encode(builder, xor, MEM(EAX,0,0,0,0), REG(EDX));
	encode(builder, xor, MEM(EAX,0,0,0,0), REG(AX)); 
	encode(builder, xor, MEM(RBX,0,0,0,0), REG(DL)); 
	
	// memory to register
	encode(builder, xor, REG(RAX), MEM(RAX,RCX,2,0x10,0));
	encode(builder, xor, REG(EAX), MEM(EAX,0,0,0x110,0));
	encode(builder, xor, REG(AX),  MEM(EAX,EDX,3,0,0));
	encode(builder, xor, REG(AL),  MEM(RAX,RBX,2,0x10,0)); 
	
	// immediate
	encode(builder, xor, MEM(RAX,0,0,0,0),  IMM(0x10)); 
	encode(builder, xor, REG(R8),  IMM(0x100));
	 
	// speical instruction
	encode(builder, add, REG(AL), IMM(0x10)); 
	encode(builder, add, REG(AX), IMM(0x10)); 
	encode(builder, add, REG(EAX), IMM(0x10)); 
	encode(builder, add, REG(RAX), IMM(0x10)); 
*/





















































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
