
//~
// X86 code generation impel
//

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

#define GET_REG(x) ((x-1)%8)
#define GET_BITNESS(x) bitness_t[((x-1)/8)]
#define GET_REX(x) (((x-1)/8) > 4)

static char bitness_t[] = {
	1, 1, 2, 4, 8, 1, 2, 4, 8
};
static unsigned long long constants_t[] = {
	UINT8_MAX, UINT16_MAX, UINT32_MAX, UINT64_MAX
};

typedef struct {
	int idx; // last 4 bits are saved for table kind
} Value;

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
		Value value;
	};
} Operand;

#define NO_OPERAND ((Operand){0})

typedef enum {
	IK_OPCODE = 1,           // regular opcode
	IK_OPCODE_REG,           // opcode + in ModR/M byte the reg is specified here
	IK_OPCODE_AL_AX_EAX_RAX, // opcode without ModR/M ending in reg (3 bits)
	IK_OPCODE_INST_REG,      // opcode without ModR/M instead, reg component inside opcode ending in reg (3 bits)
	IK_OPCODE_CONTITION,     // conditional encoding ending in tttn (4 bits)
} Instruction_Kind;

typedef enum {
	B8  = 1 << 0,
	B16 = 1 << 1,
	B32 = 1 << 2,
	B64 = 1 << 3,

	R = 1 << 4,     // register
	I = 1 << 5,     // immediate
	M = 1 << 6 | R, // memory/register
	RelM = 1 << 7,  // relative memory
	Val  = 1 << 8,  // constant of value e.g. 0x0
	ValM = 1 << 9,  // memory of value e.g.  [0x0]
} Instruction_Operand_Kind;

typedef struct {
	Instruction_Kind kind;
	u8 opcode; // TODO(ziv): don't assume opcode size = 1byte
	u8 opcode_reg; // for IK_OPCODE_REG usually used for instructions with immediate values
	u8 operand_kind[2];
} Instruction;


//~
// Instruction Tables
//

static Instruction add[] = {
	{ IK_OPCODE, .opcode = 0x00, .operand_kind = { M |B8,          R |B8 } },
	{ IK_OPCODE, .opcode = 0x01, .operand_kind = { M |B16|B32|B64, R |B16|B32|B64 } },
	{ IK_OPCODE, .opcode = 0x02, .operand_kind = { R |B8,          M |B8 } },
	{ IK_OPCODE, .opcode = 0x03, .operand_kind = { R |B16|B32|B64, M |B16|B32|B64 } },
	{ IK_OPCODE_AL_AX_EAX_RAX, 0x04, .operand_kind = { R |B8,          I |B8 } },
	{ IK_OPCODE_AL_AX_EAX_RAX, 0x05, .operand_kind = { R |B16|B32|B64, I |B16|B32 } },
	{ IK_OPCODE_REG, 0x80, 0x0,  .operand_kind = { M |B8,          I |B8 } },
	{ IK_OPCODE_REG, 0x81, 0x0,  .operand_kind = { M |B16|B32|B64, I |B16|B32 } },
	{ IK_OPCODE_REG, 0x83, 0x0,  .operand_kind = { M |B16|B32|B64, I |B8 } },
	{0}
};

static Instruction or[] = {
	{ IK_OPCODE, 0x08, .operand_kind = { M |B8,          R |B8 } },
	{ IK_OPCODE, 0x09, .operand_kind = { M |B16|B32|B64, R |B16|B32|B64 } },
	{ IK_OPCODE, 0x0a, .operand_kind = { R |B8,          M |B8 } },
	{ IK_OPCODE, 0x0b, .operand_kind = { R |B16|B32|B64, M |B16|B32|B64 } },
	{ IK_OPCODE_REG, 0x80, 0x1, .operand_kind = { M |B8,          I |B8 } },
	{ IK_OPCODE_REG, 0x81, 0x1, .operand_kind = { M |B16|B32|B64, I |B16|B32 } },
	{ IK_OPCODE_REG, 0x83, 0x1, .operand_kind = { M |B16|B32|B64, I |B8 } },
	{0}
};

static Instruction and[] = {
	{ IK_OPCODE, 0x20, .operand_kind = { M |B8,          R |B8 } },
	{ IK_OPCODE, 0x21, .operand_kind = { M |B16|B32|B64, R |B16|B32|B64 } },
	{ IK_OPCODE, 0x22, .operand_kind = { R |B8,          M |B8 } },
	{ IK_OPCODE, 0x23, .operand_kind = { R |B16|B32|B64, M |B16|B32|B64 } },
	{ IK_OPCODE_REG, 0x80, 0x4, .operand_kind = { M |B8,          I |B8 } },
	{ IK_OPCODE_REG, 0x81, 0x4, .operand_kind = { M |B16|B32|B64, I |B16|B32 } },
	{ IK_OPCODE_REG, 0x83, 0x4, .operand_kind = { M |B16|B32|B64, I |B8 } },
	{0}
};

static Instruction sub[] = {
	{ IK_OPCODE, 0x28, .operand_kind = { M |B8,          R |B8 } },
	{ IK_OPCODE, 0x29, .operand_kind = { M |B16|B32|B64, R |B16|B32|B64 } },
	{ IK_OPCODE, 0x2a, .operand_kind = { R |B8,          M |B8 } },
	{ IK_OPCODE, 0x2b, .operand_kind = { R |B16|B32|B64, M |B16|B32|B64 } },
	{ IK_OPCODE_REG, 0x80, 0x5, .operand_kind = { M |B8,          I |B8 } },
	{ IK_OPCODE_REG, 0x81, 0x5, .operand_kind = { M |B16|B32|B64, I |B16|B32 } },
	{ IK_OPCODE_REG, 0x83, 0x5, .operand_kind = { M |B16|B32|B64, I |B8 } },
	{0}
};

static Instruction xor[] = {
	{ IK_OPCODE, 0x30, .operand_kind = { M |B8,          R |B8 } },
	{ IK_OPCODE, 0x31, .operand_kind = { M |B16|B32|B64, R |B16|B32|B64 } },
	{ IK_OPCODE, 0x32, .operand_kind = { R |B8,          M |B8 } },
	{ IK_OPCODE, 0x33, .operand_kind = { R |B16|B32|B64, M |B16|B32|B64 } },
	{ IK_OPCODE_REG, 0x80, 0x6, .operand_kind = { M |B8,          I |B8 } },
	{ IK_OPCODE_REG, 0x81, 0x6, .operand_kind = { M |B16|B32|B64, I |B16|B32 } },
	{ IK_OPCODE_REG, 0x83, 0x6, .operand_kind = { M |B16|B32|B64, I |B8 } },
	{0}
};

static Instruction cmp[] = {
	{ IK_OPCODE, 0x38, .operand_kind = { M |B8,          R |B8 } },
	{ IK_OPCODE, 0x39, .operand_kind = { M |B16|B32|B64, R |B16|B32|B64 } },
	{ IK_OPCODE, 0x3a, .operand_kind = { R |B8,          M |B8 } },
	{ IK_OPCODE, 0x3b, .operand_kind = { R |B16|B32|B64, M |B16|B32|B64 } },
	{ IK_OPCODE_REG, 0x80, 0x7, .operand_kind = { M |B8,          I |B8 } },
	{ IK_OPCODE_REG, 0x81, 0x7, .operand_kind = { M |B16|B32|B64, I |B16|B32 } },
	{ IK_OPCODE_REG, 0x83, 0x7, .operand_kind = { M |B16|B32|B64, I |B8 } },
	{0}
};

static Instruction call[] = {
	{ IK_OPCODE_REG, 0xff, 0x2, .operand_kind = { M |B16|B32|B64, 0x0 } },
	{ IK_OPCODE_INST_REG, 0xe8, .operand_kind = { RelM |B16|B32, 0x0 } },
	{0}
};

static Instruction mov[] = {
	{ IK_OPCODE, 0x88, .operand_kind = { M |B8,          R |B8 } },
	{ IK_OPCODE, 0x89, .operand_kind = { M |B16|B32|B64, R |B16|B32|B64 } },
	{ IK_OPCODE, 0x8a, .operand_kind = { R |B8,          M |B8 } },
	{ IK_OPCODE, 0x8b, .operand_kind = { R |B16|B32|B64, M |B16|B32|B64 } },
	{ IK_OPCODE_INST_REG, 0xb0, .operand_kind = { R |B8,          I |B8 } },
	{ IK_OPCODE_INST_REG, 0xb8, .operand_kind = { R |B16|B32|B64, I |B16|B32 } },
	{ IK_OPCODE_REG, 0xc6, 0x0, .operand_kind = { M |B8, I |B8 } },
	{ IK_OPCODE_REG, 0xc7, 0x0, .operand_kind = { M |B16|B32|B64, I |B16|B32 } },
	{0}
};

//~

typedef enum {
	LABELS_TABLE_KIND,
	JUMPINSTRUCTIONS_TABLE_KIND,
	DATA_VARIABLES_TABLE_KIND,
	OPERAND_TABLE_KIND,
} Builder_Table_Kind;

typedef struct {
	int location_to_patch; // relative
	int location; // relative to starting position
	char *name;   // name of the thingy
} Name_Location;

typedef struct {
	char *code, *data;
	int code_size, data_size; 
	
	// TODO(ziv): Remove this as this will get replaced by the other thingy yeey
	
	//~
	// data_variables -  variables contained in the .data section
	// jumpinstructions - for addresses of external functions
	//
	
	Name_Location *labels, *jumpinstructions, *data_variables; 
	int labels_count, jumpinstructions_count, data_variables_count;
	int current_data_variable_location; // used to keep track of location of `data_variables`
	
	int bytes_count; // to tell byte count of the program 
	
	//~
	// list of external library paths e.g. kernel32.lib (in linux libc.so)
	char **external_library_paths;
	int external_library_paths_count;
	
	// result of visual studio sdk path on windows
	Find_Result vs_sdk;
	
	M_Pool *m;
	} Builder; 





internal inline Operand REG(Register_Type reg_type) { return (Operand){R, .reg=reg_type }; }
internal inline Operand IMM(int immediate) { return (Operand){I, .immediate=(immediate)}; }
internal inline Operand MEM(Register_Type base, Register_Type index, char scale, int disp, int bitness) {
	return (Operand){M, .base=base, .index=index, .scale=scale, .disp=disp, .bitness=bitness };
}

// TODO(ziv): replace this with label code
// have a global map which will substitude labels to the actual relative addresses
internal Operand RELMEM(Builder *builder, int relative_memory_address) {
	Operand result = {  RelM, .value = builder->data_variables_count | (JUMPINSTRUCTIONS_TABLE_KIND << 28)};
	Name_Location *v = &builder->jumpinstructions[builder->jumpinstructions_count++];
	v->location = relative_memory_address ;
	v->name = NULL;
	return result;
}

internal Operand x86_label(Builder *builder, const char *label) {
	Operand result = { Val, .value = builder->labels_count | (LABELS_TABLE_KIND << (32-4)) };
	Name_Location *v = &builder->labels[builder->labels_count++];
	v->location = builder->bytes_count;
	v->name = (char *)label;
	return result;
}

internal Operand x86_lit(Builder *builder, const char *literal) {
	Operand result = { Val, .value = builder->data_variables_count | (DATA_VARIABLES_TABLE_KIND << 28) };
	Name_Location *v = &builder->data_variables[builder->data_variables_count++];
	v->location = builder->current_data_variable_location;
	builder->current_data_variable_location += (int)strlen(literal)+1;
	v->name = (char *)literal;
	return result;
}



internal Name_Location *x86_get_name_location_from_value(Builder *builder, Value v) {
	Name_Location *nl_table[4] = { builder->labels , builder->jumpinstructions, builder->data_variables };
	int idx  = v.idx & ((1<<28)-1);
	int type = v.idx >> 28;
	return &nl_table[type][idx];
}
 

typedef struct {
	char *function; // name
	char *library;  // .dll HINT + name
} Function_Library;

internal bool fill_modules_for_function(Builder *builder, const char *module, Function_Library *function_library_array, 
										int function_library_size, int *func_count_in_lib) {
	// TODO(ziv): @Cleanup this function
	if (!builder->vs_sdk.windows_sdk_version) {
		return false; 
	}

	wchar_t *wchar_path = concat2(builder->vs_sdk.windows_sdk_um_library_path, L"\\"), *temp_wchar_path = wchar_path;

	// TODO(ziv): switch to using VirtualAlloc?
	// convert wcahr to char
	char *path = (char *)malloc(wcslen(wchar_path)+strlen(module)+1), *temp_path = path;
	while ((char)*temp_wchar_path) *temp_path++ = (char)*temp_wchar_path++;
	while (*module) *temp_path++ = *module++;
	*temp_path = '\0';

	char *buff; // archive library buffer
	{

		HANDLE handle = CreateFileA(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		Assert(handle != INVALID_HANDLE_VALUE && "Invalid handle");

		DWORD file_size = GetFileSize(handle, NULL), bytes_read;
		buff = VirtualAlloc(NULL, file_size, MEM_COMMIT, PAGE_READWRITE);

		BOOL success = ReadFile(handle, buff, file_size, &bytes_read, NULL);
		CloseHandle(handle);
		free(wchar_path);
		free(path);
		
		if (!success) { goto fill_modules_cleanup; }
		
	}

	// start parsing the file
	if (memcmp(buff, IMAGE_ARCHIVE_START, IMAGE_ARCHIVE_START_SIZE) != 0) {
		goto fill_modules_cleanup; 
	}
	IMAGE_ARCHIVE_MEMBER_HEADER *first_linker_member_header = (IMAGE_ARCHIVE_MEMBER_HEADER *)(buff + IMAGE_ARCHIVE_START_SIZE);

	// calculate size of first linker archive member
	int member_size = 0;
	{

		// TODO(ziv): make this be a atoi function
		int exp_table[] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000 };
		int size_str_length = 0;
		for (; '0' <= first_linker_member_header->Size[size_str_length] && first_linker_member_header->Size[size_str_length] <= '9'; size_str_length++);
		for (int i = size_str_length-1; i >= 0; i--) {
			member_size += exp_table[size_str_length-1 - i] * (first_linker_member_header->Size[i] - '0');
		}

	}

	// Skip to First Linker Member
	IMAGE_ARCHIVE_MEMBER_HEADER *second_linker_memeber_header = (IMAGE_ARCHIVE_MEMBER_HEADER *)((char *)first_linker_member_header + sizeof(IMAGE_ARCHIVE_MEMBER_HEADER) + member_size);
	char *second_linker_member = (char *)second_linker_memeber_header + sizeof(IMAGE_ARCHIVE_MEMBER_HEADER);

	unsigned long n_offsets = *(unsigned long *)(second_linker_member);
	int offsets_size = sizeof(unsigned long) + 4*n_offsets;
	unsigned long *offsets = (unsigned long *)(second_linker_member + sizeof(unsigned long));
	unsigned long n_symbols = *(unsigned long *)(second_linker_member + offsets_size);
	int symbols_size = sizeof(unsigned long) + 2*n_symbols;
	unsigned short *indicies = (unsigned short *)(second_linker_member + offsets_size + sizeof(unsigned long));
	char *string_table = (second_linker_member + offsets_size + symbols_size);
	
		//
		// Do a full search inside the String Table for all function names given 
		//
	
	char *function_to_check = string_table;
	
	unsigned int string_table_idx = 0;
	for (int function_library_idx = 0; function_library_idx < function_library_size; function_library_idx++) {
		
		// NOTE(ziv): I assume that the function names are sorted alphabetically 
		Function_Library *fl = &function_library_array[function_library_idx];
		if (fl->library)  continue;
		
		for (; string_table_idx < n_symbols; string_table_idx++) {
			
			// check specific function 
		char *function = fl->function;
			while (*function == *function_to_check && *function_to_check != '\0') {
				function++, function_to_check++; 
			}
			
			// this means there is no point to further look at this function
			if ((*function | 0x60) > (*function_to_check | 0x60)) {
				while (*function_to_check++ != '\0');
				continue;
			}
			else if ((*function | 0x60) < (*function_to_check | 0x60)) {
				if ('a' < (*function_to_check | 0x60) && (*function_to_check | 0x60) < 'z')
					break; // @Incomplete check whether this is required or not 
				
				// I don't know what to do so... contineu? 
				while (*function_to_check++ != '\0');
				continue;
				
			}
			
			// check of member offset
			unsigned short offsets_idx = indicies[string_table_idx];
			unsigned long offset = offsets[offsets_idx-1];
			
			// https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#import-library-format
			typedef struct {
				WORD    Sig1;            // Must be IMAGE_FILE_MACHINE_UNKNOWN
				WORD    Sig2;            // Must be 0xffff
				WORD    Version;         // >= 1 (implies the CLSID field is present)
				WORD    Machine;
				DWORD   TimeDateStamp;
				DWORD   SizeOfData;      // Size of data that follows the header
				union {
					WORD Hint;
					WORD Ordinal;
				};
				WORD    Type; 
			} OBJECT_HEADER;
			OBJECT_HEADER *object_header_info = (OBJECT_HEADER *)(buff + offset + sizeof(IMAGE_ARCHIVE_MEMBER_HEADER));
			char *function_and_dll_names = buff + offset + sizeof(IMAGE_ARCHIVE_MEMBER_HEADER) + sizeof(OBJECT_HEADER);
			
			if (strcmp(function_and_dll_names, fl->function) == 0) {
				++*func_count_in_lib;
				
				// duplicate string
				char *dll_name = function_and_dll_names+strlen(fl->function)+1;
				size_t dll_name_length = strlen(dll_name)+1;
				char *copy_dll_name = malloc(2 + dll_name_length);
				memcpy(copy_dll_name, &object_header_info->Hint, sizeof(object_header_info->Hint));
				memcpy(copy_dll_name + 2, dll_name, dll_name_length);
				fl->library = copy_dll_name;
				break;
			}
			
			while (*function_to_check++ != '\0');
		}
		
	}
	
	//IMAGE_ARCHIVE_MEMBER_HEADER *function_member_header = (IMAGE_ARCHIVE_MEMBER_HEADER *)(buff + offset);
	
	/* TODO(ziv): figure out whether I need this extra information for anything useful
	OBJECT_HEADER *obj_header = (OBJECT_HEADER *)(buff + offset + sizeof(IMAGE_ARCHIVE_MEMBER_HEADER));
		int function_member_size = atoi((const char *)function_member_header->Size);
		int import_type      = obj_header->Type & 0x03;
		int import_name_type = (obj_header->Type & 0x1c) >> 2;
		 */
	fill_modules_cleanup:
	
	VirtualFree(buff, 0, MEM_RELEASE); 
	return true;
}


internal Operand x86_c_function(Builder *builder, char *function) {
	Operand result = { Val, .value = builder->jumpinstructions_count | (JUMPINSTRUCTIONS_TABLE_KIND << 28)};
	Name_Location *v = &builder->jumpinstructions[builder->jumpinstructions_count++];
	v->location = 0; // NOTE(ziv): @Incomplete the location should get reset here at a later date
	v->name = function;
	return result;

}


internal Operand x86_function(Builder *builder, const char *module, const char *function) {
	Operand result = { Val, .value = builder->jumpinstructions_count | (JUMPINSTRUCTIONS_TABLE_KIND << (32-4)) };

	Name_Location *v = &builder->jumpinstructions[builder->jumpinstructions_count++];
	v->location = 0;
	v->name = (char *)function;

	module; function;
	return result;
}






#define MOD_RM   (1 << 0)
#define SIB      (1 << 1)
#define DISP     (1 << 2)
#define CONSTANT (1 << 3)
#define LOCATION_DISP  (1 << 4)
#define LOCATION_CONST (1 << 5)

internal void x86_encode(Builder *builder, Instruction *inst, Operand to, Operand from) {
	// don't allow for MEM->MEM, I->I, Any->I kinds of instructions
	Assert(!(to.kind == from.kind && ((to.kind == M || to.kind == I) || to.kind == I)));

	char bitness_to = 0, bitness_from = 0, address_bitness = 0, disp_bitness = 0;
	char rex_b = 0 , rex_r = 0, rex_x = 0, is_inst_64bit = 0;
	char mod = 0, reg = 0, r_m = 0;

	int instruction_found = false;
	unsigned long long constant = 0;
	int require = MOD_RM, disp = 0;

	Operand memory = {0};
	Name_Location *nl = 0;

	if (from.kind == Val) {
		nl = x86_get_name_location_from_value(builder, from.value);
		from = IMM(0x0);
		require |= LOCATION_CONST;
	}
	else if (from.kind == ValM) {
		nl = x86_get_name_location_from_value(builder, from.value);
		from = MEM(0,0,0,0,0);
		require |= LOCATION_DISP;
	}
	else {
		// unary operation / operation with no operands
		require |= 0;
	}

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
	else if (to.kind == RelM) {
		nl = x86_get_name_location_from_value(builder, to.value);
		require |= LOCATION_CONST;
		bitness_to = 4;
		constant = nl->location;
		require |= CONSTANT;
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
		while (i < ArrayLength(constants_t) && ((constants_t[i] & constant) != constant)) i++;
		bitness_from = (1 << i);

		if (!bitness_to) { // TODO(ziv): move this outside
			bitness_to = bitness_from;
		}

		require |= CONSTANT;
	}



	//
	// find correct instruction to encode
	//

	char temp = bitness_from;
	for (; inst->operand_kind[0] != 0; inst++) {

		// TODO(ziv): @refactor Find a way to do this in a better way

		if (inst->kind == IK_OPCODE_INST_REG && rex_b) {
			continue; // this type of instruction doesn't work well with r8, r9... registers
		}

		// handle special kinds of instructions
		if (inst->kind == IK_OPCODE_AL_AX_EAX_RAX) {
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
			((operands_kind & ((int)bitness_to << 8 | (int)bitness_from)) == ((int)bitness_to << 8 | (int)bitness_from))) {

			instruction_found = true;
			break;
		}
	}
	if (!instruction_found) {
		Assert(!"Couldn't find the right instruction!");
		return;
	}



	//
	// Beginning to encode the instruction that was found
	//

	if (inst->kind == IK_OPCODE_REG) {
		Assert(!reg);  // make sure there is no conflict?
		reg = inst->opcode_reg;
	}
	else if (inst->kind == IK_OPCODE_AL_AX_EAX_RAX ||
			 inst->kind == IK_OPCODE_INST_REG) {
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





	//
	// encode final instrution
	//

#define MOVE_BYTE(buffer, x) do { buffer[builder->bytes_count++] =(x); } while(0)
	char *instruction_buffer = builder->code;

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
		char inst_reg = (inst->kind == IK_OPCODE_INST_REG) ? r_m : 0;
		MOVE_BYTE(instruction_buffer, inst->opcode + inst_reg);
	}

	// mod R/m
	if (require & MOD_RM) {
		MOVE_BYTE(instruction_buffer,  mod << 6 | reg << 3 | r_m << 0);
	}

	// sib
	if (require & SIB) {
		MOVE_BYTE(instruction_buffer, scale << 6 | index << 3 | base << 0);
	}

	// displace
	if (require & LOCATION_DISP) { nl->location_to_patch = builder->bytes_count; }
	if (require & DISP) {
		for (int i = 0; i < disp_bitness; i++) {
			MOVE_BYTE(instruction_buffer, (char)((disp >> i*8) & 0xff));
		}
	}

	// constant
	if (require & LOCATION_CONST) { nl->location_to_patch = builder->bytes_count; }
	if (require & CONSTANT) {
		int copy_size = to.kind == RelM ? bitness_to : bitness_from;
		memcpy(&instruction_buffer[builder->bytes_count], &constant, copy_size); builder->bytes_count+= copy_size;
	}
#undef MOVE_BYTE

}