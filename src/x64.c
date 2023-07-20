//~ 
// Definitions
// 

typedef enum {
	DT_BYTE = 1, 
	DT_WORD, 
	DT_DWORD, 
	DT_QWORD, 
} Data_Type;

typedef enum {
	// no operands
	LAYOUT_NONE = 0,
	
	// unary
	LAYOUT_R, 
	LAYOUT_M,
	LAYOUT_I,
	
	// binary
	LAYOUT_RR = (LAYOUT_R << 4) | LAYOUT_R,
	LAYOUT_RM = (LAYOUT_R << 4) | LAYOUT_M,
	LAYOUT_MR = (LAYOUT_M << 4) | LAYOUT_R,
	LAYOUT_MI = (LAYOUT_M << 4) | LAYOUT_I,
	LAYOUT_RI = (LAYOUT_R << 4) | LAYOUT_I,
} Instruction_Layout;

// TODO(ziv): Think about how to do this better?
enum {
	W = 1 << 0,
	S = 1 << 1,
	D = 1 << 2,
	TTTN = 1 << 3,
	
	INST_BINARY = W|S|D,
	INST_EXT    = W,
	INST_UNARY  = W,
};

typedef struct { 
	const char *mnemonic; 
	u8 cat; 
	
	u8 op;
	
	// for immediates
	u8 op_i;
	u8 reg_i;
} InstDesc; 

typedef struct {
	u8 kind;
} Inst; 

typedef struct {
	u8 kind; 
	
	u8 reg; 
	
	u8 index, scale; 
	
	u32 imm;
} Value_Operand; 

//~ definitions derived from the x64 instruction table

#define X(a, ...) a,
typedef enum {
#include "x64_t.inc"
} Inst_Kind; 
#undef X 

static InstDesc inst_table[] = {
#define X(a, b, c, ...) [a] = { .mnemonic = b, .cat = INST_##c, __VA_ARGS__}, 
#include "x64_t.inc"
#undef X
};

//~

enum {
	MOD_INDIRECT        = 0x0, // [rax]
	MOD_INDIRECT_DISP8  = 0x1, // [rax + disp8]
	MOD_INDIRECT_DISP32 = 0x2, // [rax + disp32]
	MOD_DIRECT          = 0x3, // rax
};
 
internal inline char rex(bool w, bool r, bool x, bool b) {
	return (1<<6) | (1<<3)*w | (1<<2)*r | (1<<1)*x | (1<<0)*b;
}
internal inline char mod_rm(char mod, char reg, char r_m) {
	return mod << 6 | reg << 3 | r_m << 0;
}
internal inline char sib(char base, char index, char scale) {
	return scale << 6 | index << 3 | base << 0;
}

internal char get_mod(Value_Operand *mem) {
	if (mem->imm == 0) return MOD_INDIRECT; 
	return (mem->imm < 256) ? MOD_INDIRECT_DISP8 : MOD_INDIRECT_DISP32;
}

//~


#define EMIT1(builder, x) do { builder->code[builder->bytes_count++] = (x); } while(0)
#define EMIT4(builder, x) do { *(int *)(builder->code+builder->bytes_count) = (x); builder->bytes_count+=4;} while(0)

internal void emit_memory_ending(Builder *b, Value_Operand *vm, char reg, bool need_sib) { 
	char mod = get_mod(vm);
	char r_m = need_sib ? 0x4 : GET_REG(vm->reg);
	
	EMIT1(b, mod_rm(mod, reg, r_m));
	if (need_sib) 
		EMIT1(b, sib(vm->reg ? GET_REG(vm->reg) : 0x5, 
					 vm->index ? GET_REG(vm->index) : 0x5,
					 vm->scale));
	if (mod == MOD_INDIRECT_DISP8) EMIT1(b, (char)vm->imm); 
	else if (mod == MOD_INDIRECT_DISP32) EMIT4(b, vm->imm);
}

internal bool inst0(Builder *b, Inst *op) {
	const InstDesc *inst = &inst_table[op->kind];
	EMIT1(b, inst->op); 
	return true;
}

internal bool inst1(Builder *b, Inst *op, Value_Operand *v, Data_Type dt) {
	const InstDesc *inst = &inst_table[op->kind];
	
	u8 layout = v->kind; 
	Assert(layout <= 4); // must be unary
	
	char ending = 0;
	if (inst->cat == INST_UNARY) { 
		ending |= (inst->cat & W)*(dt != 1);
	}
	
	
	// rex
	char is_64b = dt==4;
	char rr = 0, rb = GET_REX(v->reg), rx = 0;
	
	char r_m = 0;
	if (v->kind == LAYOUT_M) {
		bool need_sib = v->index || v->reg == ESP || (!v->reg && !v->index);
		if (need_sib) {
			r_m = 4;
			rx = GET_REX(v->index);
		}
	}
	
	if (dt == DT_WORD)
		EMIT1(b, 0x66);
	if (is_64b || rr || rb || rx) 
		EMIT1(b, rex(is_64b, rr, rx, rb));
	
	EMIT1(b, inst->op_i | ending);
	
	if (v->kind == LAYOUT_R) 
		EMIT1(b, mod_rm(MOD_DIRECT, inst->reg_i, GET_REG(v->reg)));  
	else if (v->kind == LAYOUT_M) {
		bool need_sib = v->index || v->reg == ESP || (!v->reg && !v->index);
		emit_memory_ending(b, v, inst->reg_i, need_sib); 
	}
	else { 
		Assert(false);
	}
	return 1;
}

internal bool inst2(Builder *b, Inst *op, Value_Operand *v1, Value_Operand *v2, Data_Type dt) {
	Assert(b && op && v1 && v2);
	
	const InstDesc *inst = &inst_table[op->kind];
	u8 layout = v1->kind << 4 | v2->kind;
	Assert(layout > 4 && "Must have binary layout");
	
	if (layout == LAYOUT_MR) {
		Value_Operand *temp = v2; 
		v2 = v1;
		v1 = temp; 
	}
	
	// rex
	char is_64b = dt==4;
	char rr = GET_REX(v1->reg), rb = GET_REX(v2->reg), rx = 0;
	
	char ending = 0;
	char cat = inst->cat;
	if (cat <= INST_BINARY) {
		// TODO(ziv): This is currently incorrect, fix it!!!
		ending = W*(dt != 1) | 
			S*(layout == LAYOUT_RM) | 
			D*(layout == LAYOUT_RI && GET_REG(v1->reg) == 0);
	}
	
	if (layout == LAYOUT_RR) {
		if (is_64b || rr || rb) 
			EMIT1(b, rex(is_64b, rr, 0, rb));
		EMIT1(b, inst->op | ending);
		EMIT1(b, mod_rm(MOD_DIRECT, GET_REG(v1->reg), GET_REG(v2->reg))); 
	}
	else if (layout == LAYOUT_RM || layout == LAYOUT_MR) {
		bool need_sib = v2->index || v2->reg == ESP || (!v2->reg && !v2->index);
		rx = need_sib ? GET_REX(v2->index) : 0;
		
		if (is_64b || rr || rb || rx) 
			EMIT1(b, rex(is_64b, rr, rx, rb));
		EMIT1(b, inst->op | ending);
		emit_memory_ending(b, v2, GET_REG(v1->reg), need_sib);
	}
	else if (layout == LAYOUT_MI) {
		Assert(false); 
	}
	else if (layout == LAYOUT_RI) {
		
		if (GET_REG(v1->reg) == 0) {
			ending |= D; // I have found that al, ax, eax, rax afterwards immediate instructions contain the D
		}
		
		if (is_64b || rr || rb) 
			EMIT1(b, rex(is_64b, rr, 0, rb));
		EMIT1(b, inst->op | ending);
		EMIT1(b, mod_rm(MOD_DIRECT, GET_REG(v1->reg), GET_REG(v2->reg))); 
		if (v2->imm != 0) {
			if (v2->imm > 255) EMIT4(b, v2->imm);
			else EMIT1(b, (char)v2->imm);
		}
		
		
	}
	return false;
}