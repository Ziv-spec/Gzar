
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

#include "base.h"
#include "lexer.h"
#include "sema.h"
#include "parser.h"

#include "lexer.c"
#include "parser.c"
#include "sema.c"
#include "x86_asm.c"

#include <winnt.h>

#define MOD_RM   1
#define SIB      2
#define DISP     4
#define CONSTANT 8

typedef enum Register_Type {
	NONE=0,
	// REX.r = 0 or REX.b = 0 or REX.x = 0
	AL,  CL,  DL,  BL,  AH,  CH,  DH,  BH,
	_,   __,  ___, ____, SPL, BPL, SIL, DIL,
	AX,  CX,  DX,  BX,  SP,  BP,  SI,  DI,
	EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI,
	RAX, RCX, RDX, RBX, RSP, RBP, RSI, RDI,
	// REX.r = 1 or REX.b = 1 or REX.x = 1
	R8B, R9B, R10B, R11B, R12B, R13B, R14B, R15B,
	R8W, R9W, R10W, R11W, R12W, R13W, R14W, R15W,
	R8D, R9D, R10D, R11D, R12D, R13D, R14D, R15D,
	R8,  R9,  R10,  R11,  R12,  R13,  R14,  R15
} Register_Type;

static char bitness_t[] = {
	1, 1, 2, 4, 8, 1, 2, 4, 8
}; 
static unsigned long long constants_t[] = {
	UINT8_MAX, UINT16_MAX, UINT32_MAX, UINT64_MAX
};

#define GET_REG(x) ((x-1)%8)
#define GET_BITNESS(x) bitness_t[((x-1)/8)]
#define GET_REX(x) (((x-1)/8) > 4)

typedef struct Operand {
	int kind; 
	
	union {
		int reg; 
		unsigned long long immediate;
		struct {
			Register_Type base, index;
			char scale; 
			int disp, bitness;
		};
	}; 
} Operand; 

typedef enum {
	INSTRUCTION_KIND_OPCODE = 1, // regular opcode
	INSTRUCTION_KIND_OPCODE_REG, // opcode + in ModR/M byte the reg is specified here
	INSTRUCTION_KIND_OPCODE_AL_AX_EAX_RAX, // opcode without ModR/M
	INSTRUCTION_KIND_OPCODE_INST_REG, // opcode without ModR/M
} Instruction_Kind; 

typedef enum {
	B8  = 1 << 0, 
	B16 = 1 << 1, 
	B32 = 1 << 2, 
	B64 = 1 << 3,
	
	R = 1 << 4,
	I = 1 << 5, 
	M = 1 << 6 | R, // M is M/R
} Instruction_Operand_Kind; 

typedef struct {
	Instruction_Kind kind;
	u8 opcode; // TODO(ziv): don't assume opcode size = 1byte
	u8 opcode_reg; // for INSTRUCTION_KIND_OPCODE_REG
	u8 operand_kind[2];
} Instruction; 


//~
// Instruction Tables
//

static Instruction add[] = {
	{ INSTRUCTION_KIND_OPCODE, .opcode = 0x00, .operand_kind = { M |B8,          R |B8 } },
	{ INSTRUCTION_KIND_OPCODE, .opcode = 0x01, .operand_kind = { M |B16|B32|B64, R |B16|B32|B64 } }, 
	{ INSTRUCTION_KIND_OPCODE, .opcode = 0x02, .operand_kind = { R |B8,          M |B8 } },
	{ INSTRUCTION_KIND_OPCODE, .opcode = 0x03, .operand_kind = { R |B16|B32|B64, M |B16|B32|B64 } },
	{ INSTRUCTION_KIND_OPCODE_AL_AX_EAX_RAX, 0x04, .operand_kind = { R |B8,          I |B8 } },
	{ INSTRUCTION_KIND_OPCODE_AL_AX_EAX_RAX, 0x05, .operand_kind = { R |B16|B32|B64, I |B16|B32 } },
	{ INSTRUCTION_KIND_OPCODE_REG, 0x80, 0x0,  .operand_kind = { M |B8,          I |B8 } },
	{ INSTRUCTION_KIND_OPCODE_REG, 0x81, 0x0,  .operand_kind = { M |B16|B32|B64, I |B16|B32 } },
	{ INSTRUCTION_KIND_OPCODE_REG, 0x83, 0x0,  .operand_kind = { M |B16|B32|B64, I |B8 } },
	{0}
	};

static Instruction or[] = {
	{ INSTRUCTION_KIND_OPCODE, 0x08, .operand_kind = { M |B8,          R |B8 } },
	{ INSTRUCTION_KIND_OPCODE, 0x09, .operand_kind = { M |B16|B32|B64, R |B16|B32|B64 } }, 
	{ INSTRUCTION_KIND_OPCODE, 0x0a, .operand_kind = { R |B8,          M |B8 } },
	{ INSTRUCTION_KIND_OPCODE, 0x0b, .operand_kind = { R |B16|B32|B64, M |B16|B32|B64 } },
	{ INSTRUCTION_KIND_OPCODE_REG, 0x80, 0x1, .operand_kind = { M |B8,          I |B8 } },
	{ INSTRUCTION_KIND_OPCODE_REG, 0x81, 0x1, .operand_kind = { M |B16|B32|B64, I |B16|B32 } },
	{ INSTRUCTION_KIND_OPCODE_REG, 0x83, 0x1, .operand_kind = { M |B16|B32|B64, I |B8 } },
	{0}
};

static Instruction and[] = {
	{ INSTRUCTION_KIND_OPCODE, 0x20, .operand_kind = { M |B8,          R |B8 } },
	{ INSTRUCTION_KIND_OPCODE, 0x21, .operand_kind = { M |B16|B32|B64, R |B16|B32|B64 } }, 
	{ INSTRUCTION_KIND_OPCODE, 0x22, .operand_kind = { R |B8,          M |B8 } },
	{ INSTRUCTION_KIND_OPCODE, 0x23, .operand_kind = { R |B16|B32|B64, M |B16|B32|B64 } },
	{ INSTRUCTION_KIND_OPCODE_REG, 0x80, 0x4, .operand_kind = { M |B8,          I |B8 } },
	{ INSTRUCTION_KIND_OPCODE_REG, 0x81, 0x4, .operand_kind = { M |B16|B32|B64, I |B16|B32 } },
	{ INSTRUCTION_KIND_OPCODE_REG, 0x83, 0x4, .operand_kind = { M |B16|B32|B64, I |B8 } },
	{0}
};

static Instruction sub[] = {
	{ INSTRUCTION_KIND_OPCODE, 0x28, .operand_kind = { M |B8,          R |B8 } },
	{ INSTRUCTION_KIND_OPCODE, 0x29, .operand_kind = { M |B16|B32|B64, R |B16|B32|B64 } }, 
	{ INSTRUCTION_KIND_OPCODE, 0x2a, .operand_kind = { R |B8,          M |B8 } },
	{ INSTRUCTION_KIND_OPCODE, 0x2b, .operand_kind = { R |B16|B32|B64, M |B16|B32|B64 } },
	{ INSTRUCTION_KIND_OPCODE_REG, 0x80, 0x5, .operand_kind = { M |B8,          I |B8 } },
	{ INSTRUCTION_KIND_OPCODE_REG, 0x81, 0x5, .operand_kind = { M |B16|B32|B64, I |B16|B32 } },
	{ INSTRUCTION_KIND_OPCODE_REG, 0x83, 0x5, .operand_kind = { M |B16|B32|B64, I |B8 } },
	{0}
};

static Instruction xor[] = {
	{ INSTRUCTION_KIND_OPCODE, 0x30, .operand_kind = { M |B8,          R |B8 } },
	{ INSTRUCTION_KIND_OPCODE, 0x31, .operand_kind = { M |B16|B32|B64, R |B16|B32|B64 } }, 
	{ INSTRUCTION_KIND_OPCODE, 0x32, .operand_kind = { R |B8,          M |B8 } },
	{ INSTRUCTION_KIND_OPCODE, 0x33, .operand_kind = { R |B16|B32|B64, M |B16|B32|B64 } },
	{ INSTRUCTION_KIND_OPCODE_REG, 0x80, 0x6, .operand_kind = { M |B8,          I |B8 } },
	{ INSTRUCTION_KIND_OPCODE_REG, 0x81, 0x6, .operand_kind = { M |B16|B32|B64, I |B16|B32 } },
	{ INSTRUCTION_KIND_OPCODE_REG, 0x83, 0x6, .operand_kind = { M |B16|B32|B64, I |B8 } },
	{0}
};

static Instruction cmp[] = {
	{ INSTRUCTION_KIND_OPCODE, 0x38, .operand_kind = { M |B8,          R |B8 } },
	{ INSTRUCTION_KIND_OPCODE, 0x39, .operand_kind = { M |B16|B32|B64, R |B16|B32|B64 } }, 
	{ INSTRUCTION_KIND_OPCODE, 0x3a, .operand_kind = { R |B8,          M |B8 } },
	{ INSTRUCTION_KIND_OPCODE, 0x3b, .operand_kind = { R |B16|B32|B64, M |B16|B32|B64 } },
	{ INSTRUCTION_KIND_OPCODE_REG, 0x80, 0x7, .operand_kind = { M |B8,          I |B8 } },
	{ INSTRUCTION_KIND_OPCODE_REG, 0x81, 0x7, .operand_kind = { M |B16|B32|B64, I |B16|B32 } },
	{ INSTRUCTION_KIND_OPCODE_REG, 0x83, 0x7, .operand_kind = { M |B16|B32|B64, I |B8 } },
	{0}
};

static Instruction call[] = {
	{ INSTRUCTION_KIND_OPCODE_REG, 0xff, 0x2, .operand_kind = { M |B16|B32|B64, 0x0 } },
	{0}
};

static Instruction mov[] = {
	{ INSTRUCTION_KIND_OPCODE, 0x88, .operand_kind = { M |B8,          R |B8 } },
	{ INSTRUCTION_KIND_OPCODE, 0x89, .operand_kind = { M |B16|B32|B64, R |B16|B32|B64 } }, 
	{ INSTRUCTION_KIND_OPCODE, 0x8a, .operand_kind = { R |B8,          M |B8 } },
	{ INSTRUCTION_KIND_OPCODE, 0x8b, .operand_kind = { R |B16|B32|B64, M |B16|B32|B64 } },
	{ INSTRUCTION_KIND_OPCODE_INST_REG, 0xb0, .operand_kind = { R |B8,          I |B8 } },
	{ INSTRUCTION_KIND_OPCODE_INST_REG, 0xb8, .operand_kind = { R |B16|B32|B64, I |B16|B32 } },
	{ INSTRUCTION_KIND_OPCODE_REG, 0xc6, 0x0, .operand_kind = { M |B8, I |B8 } },
	{ INSTRUCTION_KIND_OPCODE_REG, 0xc7, 0x0, .operand_kind = { M |B16|B32|B64, I |B16|B32 } },
	{0}
};

//~

internal inline Operand REG(Register_Type reg_type) { return (Operand){R, .reg=reg_type }; }
internal inline Operand IMM(int immediate) { return (Operand){I, .immediate=(immediate)}; }
internal inline Operand MEM(Register_Type base, Register_Type index, char scale, int disp, int bitness) { 
	return (Operand){M, .base=base, .index=index, .scale=scale, .disp=disp, .bitness=bitness }; 
}

static int bytes_count = 0; 

/* 
static u8 add_opcodes[] = {
	0x0, 0x1, 0x2, 0x3, 0x80, 0x83, 0x81
};

static u8 add_operand_kinds[][2] = {
	{ M  |B8,           R  |B8 },          // 0x0
	{ M  |B16|B32|B64,  R  |B16|B32|B64 }, // 0x1
	{ R  |B8,           M  |B8 },          // 0x2 
	{ R  |B16|B32|B64,  M  |B16|B32|B64 }, // 0x3
	{ M  |B8,           I  |B8 },          // 0x80
	{ M  |B16|B32|B64,  I  |B8 },          // 0x83
	{ M  |B16|B32|B64,  I  |B16|B32|B64 }, // 0x81
};

static u8 add_instruction_kinds[] = {
	INSTRUCTION_KIND_OPCODE, INSTRUCTION_KIND_OPCODE, INSTRUCTION_KIND_OPCODE, INSTRUCTION_KIND_OPCODE, INSTRUCTION_KIND_OPCODE, INSTRUCTION_KIND_OPCODE, INSTRUCTION_KIND_OPCODE
};
 */

typedef struct {
	char *instructions;
	char *current_instruction;
} Builder; 


internal void encode(Builder builder, Instruction *inst, Operand to, Operand from) {
	// don't allow for MEM->MEM, I->I, Any->I kinds of instructions
	Assert(!(to.kind == from.kind && ((to.kind == M || to.kind == I) || to.kind == I))); 
	
	int instruction_found = false; 
	char bitness_to = 0, bitness_from = 0; 
	int address_bitness = 0; 
	char rex_b = 0 , rex_r = 0, rex_x = 0, is_inst_64bit = 0; 
	int require = MOD_RM, disp = 0; 
	Operand memory = {0}; 
	
	char mod = 0, reg = 0, r_m = 0;
	int disp_bitness = 0;
	unsigned long long constant = 0; 
	
	//
	// Extract info from operands
	//
	
	
	// handling operand TO
	if (to.kind == R) {
		r_m = GET_REG(to.reg); bitness_to = GET_BITNESS(to.reg); rex_b = GET_REX(to.reg);
	}
	else if (to.kind == M) {
		bitness_to = (char)to.bitness; memory = to; 
		require |= SIB;
	}
	else {
		Assert(false); // shouldn't be reaching this as of right now
	}
	
	// handling operand FROM
	if (from.kind == R) {
		reg = GET_REG(from.reg); bitness_from = GET_BITNESS(from.reg); rex_r = GET_REX(from.reg);
		if (!bitness_to) bitness_to = bitness_from; 
		Assert(bitness_to == bitness_from && "Register bitness doesn't match");
	}
	else if (from.kind == M) {
		bitness_from = (char)from.bitness; memory = from;
		Assert((bitness_from || bitness_to) && "Please provide a size for a FROM memory");
		bitness_to = bitness_from = (bitness_to > bitness_from ? bitness_to : bitness_from); 
		require |= SIB; 
		
	}
	else if (from.kind == I) {
		 constant = from.immediate;
		
			int i = 0;
			while (i < ArrayLength(constants_t) && ((constants_t[i] & constant) != constant)) 
				i++; 
		bitness_from = (1 << i);
		
		if (!bitness_to) {
			bitness_to = bitness_from; 
		}
		require |= CONSTANT;
		}
	else {
		// unary operation / operation with no operands
		
		require |= 0;
		
	}
	
	
	char temp = bitness_from; 
	// find correct instruction to encode
	for (; inst->operand_kind[0] != 0; inst++) {
		
		// handle special kinds of instructions
		if (inst->kind == INSTRUCTION_KIND_OPCODE_AL_AX_EAX_RAX) {
			bitness_from = bitness_to;
			if (bitness_to == B64) {
				bitness_from = B32;
			}
		}
		else { 
			bitness_from = temp; 
		}
		
		
		// make sure bitness of operands make senese
		if (bitness_to != bitness_from) {
			int bitness = inst->operand_kind[1] & 0x0f; 
			int i = 3, not_seen_bitness = true;
			while (i > 0 && not_seen_bitness) {
				if (bitness & (1 << i)) {
					not_seen_bitness = false;
					break;
				}
				i--;
			}
			
			// this will be the largest valid bitness value for this operand
			bitness_from = (1 << i); 
		}
		
		
		// check if correct instruction
		int operands_kind = inst->operand_kind[0] << 8 | inst->operand_kind[1]; 
		if (((operands_kind & (to.kind << 8 | from.kind)) == (to.kind << 8 | from.kind)) &&
			((operands_kind & (bitness_to << 8 | bitness_from)) == (bitness_to << 8 | bitness_from))) {
			
			instruction_found = true; 
			break;
		}
	}
	
	
	
	if (!instruction_found) {
		Assert(!"Couldn't find the right instruction!"); 
		return; 
	}
	
	if (inst->kind == INSTRUCTION_KIND_OPCODE_REG) {
		Assert(!reg);  // make sure there is no conflict?
		reg = inst->opcode_reg;
	}
	else if (inst->kind == INSTRUCTION_KIND_OPCODE_AL_AX_EAX_RAX ||
			 inst->kind == INSTRUCTION_KIND_OPCODE_INST_REG) {
		require &= ~MOD_RM;
		}
	
	
	
	// special handling if you have memory as a operand
		 char base = memory.base, index = memory.index, scale=memory.scale; disp = memory.disp;
	if (require & SIB) {
		
		// mod
		// 00 - [eax]          ; r->m/m->r
		// 00 - [disp32]       ; special case
		// 10 - [eax] + dips32 ; r->m/m->r
		// 01 - [eax] + dips8  ; r->m/m->r
		require |= DISP; 
		if (!base && !index)          { mod = 0b00; disp_bitness = 4; }
		else if (disp == 0)           { mod = 0b00; disp_bitness = 0; require &= ~DISP; }
		else if (disp > 0xff)         { mod = 0b10; disp_bitness = 4; }
		else if ((disp&0xff) == disp) { mod = 0b01; disp_bitness = 1; }
		
		if (base) { 
			r_m = GET_REG(base); address_bitness = GET_BITNESS(memory.base); 
			
			if (rex_b) {
				rex_r = rex_b; 
			}
			else {
				rex_b = GET_REX(base);
			}
		}
		
		if (index || base == ESP || (!base && !index)) { 
			r_m = 0b100; // use the sib byte
			if (index) rex_x = GET_REX(index); 
			base  = base ? GET_REG(base) : 0b101;   // remove base if needed 
			index = index ? GET_REG(index) : 0b100; // remove index if neede
		}
		else {
			require &= ~SIB; // remove when not needed
		}
		
	}
	else {
		mod = 0b11; // regular register addressing e.g. add rax, rax
	}
	
	is_inst_64bit = bitness_to == 8; 
	
	
	
#define MOVE_BYTE(buffer, x) do { buffer[bytes_count++] = x; } while(0)
		//
		// encode final instrution
		//
	
	char *instruction_buffer = builder.current_instruction;
	
		// prefix 
		{
			if (bitness_to == 2)      MOVE_BYTE(instruction_buffer, 0x66); // AX, BX, CX...
			if (address_bitness == 4) MOVE_BYTE(instruction_buffer, 0x67); // [EAX], [EBX], [ECX]...
		}
		
		// rex
		if (is_inst_64bit | rex_b | rex_r | rex_x) {
			char rex = ((1 << 6) | // Must have for it to be recognized as rex byte
						(1 << 0)*rex_b | 
						(1 << 1)*rex_x | 
						(1 << 2)*rex_r | 
						(1 << 3)*is_inst_64bit); // rex_w
			MOVE_BYTE(instruction_buffer, rex); 
		}
		
		// opcode
	{
		 char inst_reg = (inst->kind == INSTRUCTION_KIND_OPCODE_INST_REG) ? r_m : 0;
		MOVE_BYTE(instruction_buffer, inst->opcode + inst_reg);
		}
		
		// mod R/m
		if (require & MOD_RM) {
			MOVE_BYTE(instruction_buffer,  mod << 6 | reg << 3 | r_m << 0; ); 
		}
		
		// sib
		if (require & SIB) {
			MOVE_BYTE(instruction_buffer, scale << 6 | index << 3 | base << 0); 
			}
		
		// displace
		if (require & DISP) {
			for (int i = 0; i < disp_bitness; i++) {
					MOVE_BYTE(instruction_buffer, (char)((disp >> i*8) & 0xff));
			}
		}
		
		// constant
		if (require & CONSTANT) {
			memcpy(instruction_buffer + bytes_count, &constant, bitness_from); bytes_count+= bitness_from;
		}
		
	
}

internal inline int align(int x, int alignment) {
	int remainder = x % alignment; 
	return remainder ? (x - remainder + alignment) : x;
}

int main(int argc, char *argv[]) {
	argc, argv;
	
#if 1
	
	//~
	// Generate Code
	//
	
	char instructions[0x50] = {0};
	Builder builder = { instructions, instructions };
	
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
	
	encode(builder, sub,  REG(RSP), IMM(0x28));
	encode(builder, mov,  REG(R9D), IMM(0));
	encode(builder, mov,  REG(R8D), IMM(0x403000));
	encode(builder, mov,  REG(EDX), IMM(0x40301b));
	encode(builder, mov,  REG(ECX), IMM(0));
	encode(builder, call, MEM(0,0,0,0x402088,0), (Operand){0});
	encode(builder, mov,  REG(ECX), IMM(0));
	encode(builder, call, MEM(0,0,0,0x402078,0), (Operand){0});
	
	for (int i = 0; i < bytes_count; i++) { printf("%02x", builder.instructions[i]&0xff); } printf("\n");
	printf("");
#endif
	
	
	
	
#define MOVEA(dest, src) do { memcpy(dest, src, sizeof(src)); dest += sizeof(src); } while(0)
#define MOVEC(dest, src, size) do { memcpy(dest, src, size); dest += size; } while(0)
#define MOVES(dest, src) do { memcpy(dest, &src, sizeof(src)); dest += sizeof(src); } while(0)
#define ALIGN(x, alignment) ((alignment)+(x)-(x)%(alignment))
	
	typedef struct { 
		char *dll_name;
		char **function_names;
	} Entry; 
	
	
	//~
	// .text .data sections data
	//
	
	// text section info
	
	char *code = (char*)instructions; 
	unsigned int code_size = bytes_count;
	
	/* 
	// without image dos stub
		u8 code[] = {
			0x48, 0x83, 0xec, 0x28,
			0x41, 0xb9, 0, 0, 0, 0,
			0x41, 0xb8, 0, 0x30, 0x40, 0, 
			0xba, 0x1b, 0x30, 0x40, 0,
			0xb9, 0, 0, 0, 0,
			0xff, 0x14, 0x25, 0x88, 0x20, 0x40, 0,
			0xb9, 0, 0, 0, 0,
			0xff, 0x14, 0x25, 0x78, 0x20, 0x40, 0, 
		};
		unsigned int code_size = sizeof(code); 
		 */
	
	// data section info
	u8 hello_world[]   = "Hello World!";
	u8 simple_pe_exe[] = "a simple 64b PE executable";
	u8 data[sizeof(hello_world) + sizeof(simple_pe_exe)], *pdata = data;
	const unsigned int data_size = sizeof(data);
	MOVEA(pdata, simple_pe_exe); 
	MOVEA(pdata, hello_world); 
	
	
	// idata section info, this defines functions you want to import & their respective dll's
	Entry entries[] = {
		{"kernel32.dll", (char *[]){ "ExitProcess", 0 }},
		{"user32.dll",   (char *[]){ "MessageBoxA", 0 }}, 
	};
	
	
	//~
	// Generation of executable 
	//
	
	// nt signiture and stub will not change
	const u8 image_stub_and_signiture[] = {
		// image NT signiture
		0x4D, 0x5A, 0x90, 0x00, 0x03, 0x00, 0x00, 0x00, 
		0x04, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 
		0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		// image dos stub
		
		/* 		*/
				0x0E, 0x1F, 0xBA, 0x0E, 0x00, 0xB4, 0x09, 0xCD,
				0x21, 0xB8, 0x01, 0x4C, 0xCD, 0x21, 0x54, 0x68,
				0x69, 0x73, 0x20, 0x70, 0x72, 0x6F, 0x67, 0x72, 
				0x61, 0x6D, 0x20, 0x63, 0x61, 0x6E, 0x6E, 0x6F, 
				0x74, 0x20, 0x62, 0x65, 0x20, 0x72, 0x75, 0x6E, 
				0x20, 0x69, 0x6E, 0x20, 0x44, 0x4F, 0x53, 0x20, 
				0x6D, 0x6F, 0x64, 0x65, 0x2E, 0x0D, 0x0D, 0x0A, 
				0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
				0x86, 0x07, 0x08, 0xF2, 0xC2, 0x66, 0x66, 0xA1, 
				0xC2, 0x66, 0x66, 0xA1, 0xC2, 0x66, 0x66, 0xA1, 
				0xD6, 0x0D, 0x62, 0xA0, 0xC9, 0x66, 0x66, 0xA1, 
				0xD6, 0x0D, 0x65, 0xA0, 0xC5, 0x66, 0x66, 0xA1, 
				0xD6, 0x0D, 0x63, 0xA0, 0x52, 0x66, 0x66, 0xA1, 
				0xA0, 0x1E, 0x63, 0xA0, 0xE5, 0x66, 0x66, 0xA1, 
				0xA0, 0x1E, 0x62, 0xA0, 0xD2, 0x66, 0x66, 0xA1, 
				0xA0, 0x1E, 0x65, 0xA0, 0xCB, 0x66, 0x66, 0xA1, 
				0xD6, 0x0D, 0x67, 0xA0, 0xCB, 0x66, 0x66, 0xA1, 
				0xC2, 0x66, 0x67, 0xA1, 0xB1, 0x66, 0x66, 0xA1, 
				0xA3, 0x1C, 0x63, 0xA0, 0xC3, 0x66, 0x66, 0xA1, 
				0xA3, 0x1C, 0x64, 0xA0, 0xC3, 0x66, 0x66, 0xA1, 
				0x52, 0x69, 0x63, 0x68, 0xC2, 0x66, 0x66, 0xA1, 
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				 
	};
	// Set the address at location 0x3c to be after signiture and stub
	*(unsigned int *)&image_stub_and_signiture[0x3c] = sizeof(image_stub_and_signiture);
	
	// NOTE(ziv): Values set to 0 will have their values set later
	IMAGE_NT_HEADERS nt_headers = {
		.Signature = 'P' | 'E' << 8,
		
		.FileHeader = { 
			.Machine = IMAGE_FILE_MACHINE_AMD64, // TODO(ziv): make it support both 32bit and 64bit OS's
			.NumberOfSections     = 3, // in our case we need a .text .data .idata
			.SizeOfOptionalHeader = 0,
			.Characteristics      = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_LARGE_ADDRESS_AWARE
		}, 
		
		.OptionalHeader = {
			.Magic = IMAGE_NT_OPTIONAL_HDR_MAGIC, 
			
			.MajorOperatingSystemVersion = 6,
			.MajorSubsystemVersion = 6,
			
			// TODO(ziv): Do I need these? 
			.SizeOfCode              = 0, // size of .text section  
			.SizeOfInitializedData   = 0, // size of .data section 
			.SizeOfUninitializedData = 0, // size of .bss section 
			.AddressOfEntryPoint     = 0, // address of the entry point
			
			//.BaseOfCode = 0x1000,
			.ImageBase        = 0x400000,
			.SectionAlignment = 0x1000,   // Minimum space that can a section can occupy when loaded that is, 
			.FileAlignment    = 0x200,    // .exe alignment on disk
			.SizeOfImage      = 0,
			.SizeOfHeaders    = 0,
			
			.Subsystem = IMAGE_SUBSYSTEM_WINDOWS_GUI, 
			.SizeOfStackReserve = 0x100000,
			.SizeOfStackCommit  = 0x1000,
			.SizeOfHeapReserve  = 0x100000,
			.SizeOfHeapCommit   = 0x1000,
			
			.NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES,
			.DataDirectory = {0}
		}
	};
	
	int section_alignment = nt_headers.OptionalHeader.SectionAlignment; 
	int file_alignment    = nt_headers.OptionalHeader.FileAlignment; 
	
	// special numbers that are not known ahead of time and will have to get calculated
	int sizeof_headers_unaligned = sizeof(image_stub_and_signiture) + sizeof(nt_headers) + nt_headers.FileHeader.NumberOfSections*sizeof(IMAGE_SECTION_HEADER);
	//int offset = align((sizeof(image_stub_and_signiture) - 0x40), 0x200);
	{
		nt_headers.FileHeader.SizeOfOptionalHeader = sizeof(nt_headers.OptionalHeader);
		
		nt_headers.OptionalHeader.AddressOfEntryPoint = 0x1000;
		nt_headers.OptionalHeader.SizeOfImage   = 0x4000; // TODO(ziv): Make automatic
		nt_headers.OptionalHeader.SizeOfHeaders = align(sizeof_headers_unaligned, file_alignment);
	}
	
	
	//~
	// .text .data .idata section headers
	// 
	
	// NOTE(ziv): idata section structure
	// Import Directory Table - contains information about the following structures locations
	// Import Name Table - table of references to tell the loader which functions are needed (NULL terminated)
	// Function Names - A array of the function names we want the loader to load. The structure is 'IMAGE_IMPORT_BY_NAME' (it's two first bytes are a 'HINT')
	// Import Address Table - identical to Import Name Table but during loading gets overriten with valid values for function addresses
	// Module names - simple array of the module names
	
	// Compute sizes for all idata related structures
	int descriptors_size = sizeof(IMAGE_IMPORT_DESCRIPTOR) * (ArrayLength(entries) + 1);
	int int_tables_size = 0, module_names_size = 0, function_names_size = 0; 
	for (int i = 0; i < ArrayLength(entries); i++) {
		char **fnames = entries[i].function_names, **fnames_temp = fnames;
		for (; *fnames_temp; fnames_temp++) 
			function_names_size += (int)(strlen(*fnames_temp) + 1 + 2); // +2 for 'HINT'
		int_tables_size += (int)(sizeof(size_t) * (fnames_temp-fnames+1));
		module_names_size += (int)(strlen(entries[i].dll_name) + 1);
	}
	int idata_size = descriptors_size + int_tables_size*2 + function_names_size + module_names_size;
	
	
	IMAGE_SECTION_HEADER text_section = { 
		.Name = ".text",
		.Misc.VirtualSize = 0x1000, // set actual size 
		.VirtualAddress   = section_alignment, 
		.SizeOfRawData    = align(code_size, file_alignment),
		.PointerToRawData = nt_headers.OptionalHeader.SizeOfHeaders, // the sections begin right after all the NT headers
		.Characteristics  = IMAGE_SCN_MEM_EXECUTE |IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_CODE
	}; 
	
	int idata_section_address = align(text_section.VirtualAddress + text_section.SizeOfRawData, section_alignment);
	IMAGE_SECTION_HEADER idata_section = { 
		.Name = ".idata",
		.Misc.VirtualSize = idata_size, 
		.VirtualAddress   = idata_section_address,
		.SizeOfRawData    = align(idata_size, file_alignment),
		.PointerToRawData = text_section.PointerToRawData + text_section.SizeOfRawData,
		.Characteristics  = IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_INITIALIZED_DATA,
	}; 
	
	// importsVA - make sure that idata section is recognized as a imports section
	{
		nt_headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = idata_section_address; 
		nt_headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = idata_size; 
	}
	
	IMAGE_SECTION_HEADER data_section = { 
		.Name = ".data",
		.Misc.VirtualSize = data_size, 
		.VirtualAddress   = align(idata_section.VirtualAddress + idata_section.SizeOfRawData, section_alignment), 
		.SizeOfRawData    = align(data_size, file_alignment), 
		.PointerToRawData = idata_section.PointerToRawData + idata_section.SizeOfRawData, 
		.Characteristics  = IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_INITIALIZED_DATA,
	}; 
	
	
	//~
	// Creating and filling .idata section
	//
	
	u8 *idata = calloc(idata_size, 1); // idata buffer to fill
	
	int descriptors_base_address    = idata_section_address;
	int int_table_base_address      = descriptors_base_address + descriptors_size;
	int function_names_base_address = int_table_base_address + int_tables_size;
	int iat_table_base_address      = function_names_base_address + function_names_size;
	int module_names_base_address   = iat_table_base_address + int_tables_size;
	
	size_t *int_tables = (size_t *)(idata + descriptors_size); 
	u8 *function_names = (u8 *)int_tables + int_tables_size;
	size_t *iat_tables = (size_t *)(function_names + function_names_size);
	u8 *module_names   = (u8 *)iat_tables + int_tables_size;
	
	int int_tables_offset = 0, module_names_offset = 0, function_names_offset = 0; 
	for (int i = 0; i < ArrayLength(entries); i++) {
#if DEBUG > 1
		printf(" -- Descriptor%d -- \n", i);
		printf("int-table \t%x -> ", int_table_base_address + int_tables_offset);
		printf("%x,", function_names_base_address + function_names_offset);
		printf("0      \\ \n");
		printf("module-name \t%x -> %s | ->\n", module_names_base_address + module_names_offset, entries[i].dll_name);
		printf("iat-table \t%x -> ", iat_table_base_address + int_tables_offset);
		printf("%x,", function_names_base_address + function_names_offset);
		printf("0      / \n");
#endif // DEBUG
		
		// filling the descriptors 
		((IMAGE_IMPORT_DESCRIPTOR *)idata)[i] = (IMAGE_IMPORT_DESCRIPTOR){
			.OriginalFirstThunk = int_table_base_address    + int_tables_offset,
			.Name               = module_names_base_address + module_names_offset,
			.FirstThunk         = iat_table_base_address    + int_tables_offset
		};
		
		// filling function names, int table and iat table
		char **fnames = entries[i].function_names, **fnames_start = fnames;
		for (; *fnames; fnames++) {
			int copy_size = (int)strlen(*fnames)+1;
			// NOTE(ziv): first two bytes are used for 'HINT'
			memcpy(function_names + function_names_offset + 2, *fnames, copy_size); 
			// fill the function's addresses into the INT and IAT tables. 
			*int_tables++ = *iat_tables++ = (size_t)function_names_base_address + function_names_offset;
			function_names_offset += copy_size + 2; 
		}
		int_tables_offset += (int)(sizeof(size_t) * ((fnames-fnames_start)+1)); 
		int_tables++; iat_tables++; // for NULL address
		
		// filling module names 
		size_t copy_size = strlen(entries[i].dll_name)+1; 
		memcpy((void *)(module_names + module_names_offset), (void *)entries[i].dll_name, copy_size);
		module_names_offset += (int)copy_size;
	}
	
	//~
	// Creating final executable
	//
	
	size_t exe_size = nt_headers.OptionalHeader.SizeOfHeaders + 
		text_section.SizeOfRawData + idata_section.SizeOfRawData + data_section.SizeOfRawData;
	u8 *exe = calloc(exe_size, 1), *pexe = exe;
	
	MOVEA(pexe, image_stub_and_signiture); 
	MOVES(pexe, nt_headers);
	MOVES(pexe, text_section);
	MOVES(pexe, idata_section);
	MOVES(pexe, data_section);      pexe += nt_headers.OptionalHeader.SizeOfHeaders - sizeof_headers_unaligned;
	MOVEC(pexe, code, code_size);   pexe += text_section.SizeOfRawData  - code_size;
	MOVEC(pexe, idata, idata_size); pexe += idata_section.SizeOfRawData - idata_size;
	MOVEA(pexe, data);              pexe += data_section.SizeOfRawData  - data_size;
	
	
	FILE *fp = fopen("test.exe", "wb"); 
	fwrite(exe, 1, exe_size, fp);
	fclose(fp);
	
	
	
	
	
	
	
	
	
	
	
	
#if 0
	
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
	
	Token_Stream token_stream;
	token_stream.capacity = 4*1024; // some default value for the time being a page size
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
	
#endif
	
	return 0;
}
