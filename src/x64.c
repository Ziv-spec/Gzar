
enum {
	PL_LABELS, 
	PL_DATA_VARS, 
	PL_C_FUNCS,
	PL_COUNT
};

typedef struct { 
	int *loc; // locations to patch
	int loc_cnt;
	int loc_sz;
	int rva;
} Patch_Locations;

typedef struct {
	M_Pool m; // temporary memory allocator
	
	Patch_Locations *pls;
	int pls_cnt;
	int pls_sz;
		int current_data_variable_location; // used to keep track of location of `data_variables`
	// name -> Patch_Locations
	
	Map pls_maps[PL_COUNT]; 
	
	int bytes_count; // to tell byte count of the program 
	char *code;
	
} Builder; 

//~ 
// x64 Definitions
// 

// TODO(ziv): check whether I need all the names or just rax, r8 and their continuation along with a size description
typedef enum Register_Type {
	UNKNOWN_REG=0,
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

#define GET_REG(x) ((x-1)%8)
#define GET_REX(x) (((x-1)/8) > 4)
#define GET_BITNESS(x) bitness_t[((x-1)/8)]
 
typedef enum {
	DT_BYTE  = 1, 
	DT_WORD  = 2, 
	DT_DWORD = 4, 
	DT_QWORD = 8, 
} Data_Type;

// TODO(ziv): Think about how to do this better?
// These are the definition of instrution categories,
// composed of instruction properties.
typedef enum {
	W = 1 << 0,
	S = 1 << 1,
	D = 1 << 2,
	REG  = 1 << 3,
	TTTN = 1 << 4,
	
	INST_BINARY = W|S|D,
	INST_BINARY_EXT = W|S,
	INST_EXT    = W,
	INST_UNARY  = 0,
	INST_UNARY_EXT = W,
} Instruction_Category;

typedef enum {
	// no operands
	LAYOUT_NONE = 0,
	// unary
	LAYOUT_R, LAYOUT_M, LAYOUT_I, LAYOUT_V,
	// binary
	LAYOUT_RR = LAYOUT_R<<4 | LAYOUT_R,
	LAYOUT_RM = LAYOUT_R<<4 | LAYOUT_M,
	LAYOUT_MR = LAYOUT_M<<4 | LAYOUT_R,
	LAYOUT_MI = LAYOUT_M<<4 | LAYOUT_I,
	LAYOUT_RI = LAYOUT_R<<4 | LAYOUT_I,
} Instruction_Layout;

typedef struct { 
	const char *mnemonic; 
	u8 cat; 
	
	u8 op;
	
	// for immediates
	u8 op_i;
	u8 reg_i;
	} Instruction_Desc; 

typedef struct {
	u8 kind;
} Inst; 

typedef struct {
	u8 kind; 
	
	u8 reg; 
	
	u8 index, scale; 
	
	u32 imm;
} Value_Operand; 

//~

internal void set_patch_location(Builder *b, int idx, int value) {
	Assert(idx < b->pls_sz);
	
	Patch_Locations *pls = &b->pls[idx]; 
	if (pls->loc_cnt >= pls->loc_sz) {
		int new_sz = (pls->loc_sz+1) * 2;
		pls->loc = pool_resize(&b->m, pls->loc, pls->loc_sz, new_sz);
		pls->loc_sz = new_sz;
	}
	pls->loc[pls->loc_cnt++] = value;
}

internal Value_Operand get_patch_location(Builder *b,String8 lit, int type, int rva) {
	Assert(b && lit.size > 0);
	Assert(type < PL_COUNT);
	
	Patch_Locations *loc = map_peek(&b->pls_maps[type], lit);
	int idx = (int)(loc - b->pls);
	if (!loc) {
		b->pls[idx = b->pls_cnt].rva = rva;
		map_set(&b->pls_maps[type], lit, &b->pls[b->pls_cnt++]);
	}
	
	return (Value_Operand){ .kind = LAYOUT_V, .imm = type << 28 | idx };
}

internal Value_Operand e_label(Builder *b, char *label) {
	return get_patch_location(b, str8_lit(label), PL_LABELS, b->bytes_count);
}

internal Value_Operand e_cfunction(Builder *b, char *label) {
	 return get_patch_location(b, str8_lit(label), PL_C_FUNCS, 0); 
}

internal Value_Operand e_lit(Builder *b, char *literal) {
	String8 lit = str8_lit(literal);
	Value_Operand result = get_patch_location(b, lit, PL_DATA_VARS, b->current_data_variable_location);
	b->current_data_variable_location += (int)lit.size+1;
	return result;
}

//~

#define X(a, ...) a,
typedef enum {
#include "x64_t.inc"
} Inst_Kind; 
#undef X 

static Instruction_Desc inst_table[] = {
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
	return 0x40 | (1<<3)*w | (1<<2)*r | (1<<1)*x | (1<<0)*b;
}
internal inline char mod_rm(char mod, char reg, char r_m) {
	return mod << 6 | reg << 3 | r_m << 0;
}
internal inline char sib(char base, char index, char scale) {
	return scale << 6 | index << 3 | base << 0;
}


//~
// Emitting instructions
//

#define EMIT_N(builder, x, type) do { *(type *)(builder->code+builder->bytes_count) = (x); builder->bytes_count+=sizeof(type) ;} while(0)
#define EMIT1(builder, x) EMIT_N(builder, x, char)
#define EMIT2(builder, x) EMIT_N(builder, x, short)
#define EMIT4(builder, x) EMIT_N(builder, x, int)

internal void emit_memory_ending(Builder *b, Value_Operand *vm, char reg, bool need_sib) { 
	// intel manual 2-6 Vol. 2A
	char mod = (vm->imm == 0 || (!vm->index && !vm->reg)) ? MOD_INDIRECT : ((vm->imm < 256) ? MOD_INDIRECT_DISP8 : MOD_INDIRECT_DISP32);
	
	char r_m = need_sib ? 0x4 : GET_REG(vm->reg);
	
	EMIT1(b, mod_rm(mod, reg, r_m));
	if (need_sib) 
		EMIT1(b, sib(vm->reg ? GET_REG(vm->reg) : 0x5, 
					 vm->index ? GET_REG(vm->index) : 0x4,
					 vm->scale));
	if (mod == MOD_INDIRECT_DISP8) EMIT1(b, (char)vm->imm); 
	else if (mod == MOD_INDIRECT_DISP32) EMIT4(b, vm->imm);
	else EMIT4(b, vm->imm);
}

internal inline bool memory_operand_need_sib(Value_Operand *mem) {
	return mem->index || mem->reg == ESP || mem->reg == RSP || (!mem->reg && !mem->index);
}

internal void inst0(Builder *b, Inst *op) {
	const Instruction_Desc *inst = &inst_table[op->kind];
	EMIT1(b, inst->op); 
}

internal void inst1(Builder *b, Inst *op, Value_Operand *v, Data_Type dt) {
	Assert(b && op && b && dt); 
	
	u8 layout = v->kind; 
	Assert(layout <= 4); // must be unary
	if (layout == 0)  return;
	
	
	bool bookkeep = false;
	if (layout & LAYOUT_V) {
		layout = (layout == (LAYOUT_V|LAYOUT_M)) ? LAYOUT_M : LAYOUT_I;
		*v = (Value_Operand){ .kind = layout, .imm = v->imm};
		bookkeep = true;
	}
	
	
	const Instruction_Desc *inst = &inst_table[op->kind];
	
	char ending = 0;
	if (inst->cat == INST_UNARY) { 
		ending |= (inst->cat & W)*(dt != 1);
	}
	
	// rex
	char is_64b = (dt==8);
	char rr = 0, rb = GET_REX(v->reg), rx = 0;
	
	// preperation
	char r_m = 0;
	bool need_sib = 0;
	if (layout == LAYOUT_M) {
		if (need_sib = memory_operand_need_sib(v)) {
			r_m = 4;
			rx = GET_REX(v->index);
		}
	}
	
	
	// common thingy
	if (dt == DT_WORD)
		EMIT1(b, 0x66);
	if (is_64b || rr || rb || rx) 
		EMIT1(b, rex(is_64b, rr, rx, rb));
	
	// special case
	if (inst->cat == INST_UNARY_EXT && layout == LAYOUT_I) {
		EMIT1(b, inst->op);
		EMIT4(b, v->imm);
		goto unary_end; 
	}
	
	EMIT1(b, inst->op_i | ending);
	
	// less common thingy
	if (layout == LAYOUT_R) 
		EMIT1(b, mod_rm(MOD_DIRECT, inst->reg_i, GET_REG(v->reg)));  
	else if (layout == LAYOUT_M) {
		emit_memory_ending(b, v, inst->reg_i, need_sib); 
	}
	else { 
		Assert(!"Type that I can't handle at the moment");
	}
	
	unary_end: 
	
	if (bookkeep) { 
		int idx = v->imm & ((1<<28)-1);
		set_patch_location(b, idx, b->bytes_count-4);
	}
	
}

internal void inst2(Builder *b, Inst *op, Value_Operand *v1, Value_Operand *v2, Data_Type dt) {
	Assert(b && op && v1 && v2);
	
	const Instruction_Desc *inst = &inst_table[op->kind];
	
	bool bookkeep = false;
	if (v2->kind & LAYOUT_V) {
		*v2 = (Value_Operand){ 
			.kind = (v2->kind == (LAYOUT_V|LAYOUT_M)) ? LAYOUT_M : LAYOUT_I,
			.imm = v2->imm
		};
		bookkeep = true;
	}
	
	u8 layout = v1->kind << 4 | v2->kind;
	Assert(layout > 4 && "Must have binary layout");
	if (layout == 0)  return;
	
	if (layout == LAYOUT_MR) {
		Value_Operand *temp = v2; 
		v2 = v1;
		v1 = temp; 
	}
	
	// rex
	char is_64b = dt==8;
	char rr = GET_REX(v1->reg), rb = GET_REX(v2->reg), rx = 0;
	// TODO(ziv): Handle special cases - Intel Manual Vol. 2A 2-11
	
	
	// handle special cases with different operands
	bool need_sib = 0; 
	if (v2->kind == LAYOUT_M) {
		need_sib = memory_operand_need_sib(v2);
		rx = need_sib ? GET_REX(v2->index) : 0;
	}
	else if (GET_REG(v1->reg) == 0 && v2->kind == LAYOUT_I) {
		rb = rr; rr = 0;
	}
	
	
	// instruction ending
	char ending = 0;
	char cat = inst->cat;
	if (cat <= INST_BINARY) {
		// TODO(ziv): This is currently incorrect, fix it!!!
		ending = (cat & W)*(dt != 1) | (cat & S)*(layout == LAYOUT_RM) ;
		//| D*(layout == LAYOUT_RI && GET_REG(v1->reg) == 0) && !rr;
		// I have found that al, ax, eax, rax afterwards immediate instructions contain the D
	}
	
	//
	// Emit instruction 
	//
	
	// prologue
	if (dt == 2) 
		EMIT1(b, 0x66);
	if (GET_BITNESS(v2->reg) == 4)
		EMIT1(b, 0x67);
	if (is_64b || rr || rb || rx) 
		EMIT1(b, rex(is_64b, rr, rx, rb));
	
	
	// epilogue
	if (layout == LAYOUT_RR) {
		EMIT1(b, inst->op | ending);
		EMIT1(b, mod_rm(MOD_DIRECT, GET_REG(v1->reg), GET_REG(v2->reg))); 
	}
	else if (layout == LAYOUT_RM || layout == LAYOUT_MR) {
		EMIT1(b, inst->op | ending); 
		emit_memory_ending(b, v2, GET_REG(v1->reg), need_sib);
		
		// patch location
		
	}
	else if (layout == LAYOUT_RI) {
		
		if (GET_REG(v1->reg) == 0 && !rb && (cat & D)) {
			EMIT1(b, inst->op | ending); 
		}
		else {
			EMIT1(b, inst->op_i | ending);
			EMIT1(b, mod_rm(MOD_DIRECT, inst->reg_i, GET_REG(v1->reg))); 
		}
		
		
		// TODO(ziv): add an option for emitting 64 bit values (for mov inst...)
		if (dt == 1)      EMIT1(b, (char)v2->imm);
		else if (dt == 2) EMIT2(b, (short)v2->imm);
		else              EMIT4(b, v2->imm);
		
	}
	else if (layout == LAYOUT_MI) {
		Assert(!"I can't handle this at the moment"); 
	}
	
	 
	if (bookkeep) { 
		int idx = v2->imm & ((1<<28)-1);
		set_patch_location(b, idx, b->bytes_count-4);
	}
	
}



//~
// Testing
//

static Value_Operand x64_test_values_t[] = {
	// register variations
	[0] = { .kind = LAYOUT_R, .reg = AL  },
	[1] = { .kind = LAYOUT_R, .reg = AX  },
	[2] = { .kind = LAYOUT_R, .reg = RSP },
	[3] = { .kind = LAYOUT_R, .reg = EBX },
	[4] = { .kind = LAYOUT_R, .reg = R9D },
	[5] = { .kind = LAYOUT_R, .reg = R8  },
	// memory variations
	[6] = { .kind = LAYOUT_M, .reg = RSP, .index = R9,  .scale = 2, .imm=0x1000 },
	[7] = { .kind = LAYOUT_M, .reg = EAX },
	[10] ={ .kind = LAYOUT_M, .imm=0x10  },
	// immediate variations
	[8] = { .kind = LAYOUT_I, .imm = 0x20   },
	[9] = { .kind = LAYOUT_I, .imm = 0x1000 },
};

internal void test_x64_unary(Builder *b, Inst_Kind kind) {
	
	static u8 dt_t[] = {
		DT_BYTE, 
		DT_WORD, 
		DT_QWORD, 
		DT_DWORD, 
		DT_DWORD, 
		DT_QWORD, 
		
		DT_WORD, 
		DT_DWORD,
	};
	
	Inst inst = (Inst){ kind };
	for (int i = 0; i < ArrayLength(dt_t); i++) {
		inst1(b, &inst, &x64_test_values_t[i], dt_t[i]);
	}
	
}

internal void test_x64_binary(Builder *b, Inst_Kind kind) {
	
	static u8 binary_matching_t[][3] = {
		// rr
		{ 0, 0, DT_BYTE  },
		{ 1, 1, DT_WORD  },
		{ 2, 2, DT_QWORD }, 
		{ 5, 5, DT_QWORD }, 
		{ 4, 3, DT_DWORD },
		{ 2, 5, DT_QWORD }, 
		// rm TODO(ziv): make memory thingy without an index, or base, variations with displacement and things like that
		{ 0, 6, DT_BYTE  }, 
		{ 0, 7, DT_BYTE  }, 
		{ 1, 6, DT_WORD  }, 
		{ 1, 7, DT_WORD  }, 
		{ 2, 6, DT_DWORD }, 
		{ 2, 7, DT_DWORD }, 
		{ 5, 6, DT_QWORD },
		{ 5, 7, DT_QWORD },
		{ 5, 10, DT_QWORD }, 
		{ 10, 5, DT_QWORD },
		// ri
		{ 0, 8, DT_BYTE  },
		{ 1, 8, DT_WORD  },
		{ 2, 8, DT_QWORD },
		{ 5, 8, DT_QWORD },
		//{ 0, 9, DT_BYTE  },
		{ 1, 9, DT_WORD  },
		{ 2, 9, DT_QWORD },
		{ 5, 9, DT_QWORD },
	};
	
	Inst inst = (Inst){ kind };
	for (int i = 0; i < ArrayLength(binary_matching_t); i++) {
		inst2(b, &inst,
			  &x64_test_values_t[binary_matching_t[i][0]],
			  &x64_test_values_t[binary_matching_t[i][1]],
			  binary_matching_t[i][2]);
	}
	
}
