


internal int compare(const void *a, const void *b) { return strcmp(a, b); }

internal void parse_archive_library(M_Pool *m, Map *import_entries, char *archive_library_path, 
									char **external_function_names, size_t external_function_names_length, 
									u8 *visited_functions, u16 *hints) {
	
	// Opening archive library file
	HANDLE handle = CreateFileA(archive_library_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	Assert(handle != INVALID_HANDLE_VALUE && "Invalid handle");
	
	DWORD file_size = GetFileSize(handle, NULL), bytes_read;
	char *lib = VirtualAlloc(NULL, file_size, MEM_COMMIT, PAGE_READWRITE);
	
	BOOL success = ReadFile(handle, lib, file_size, &bytes_read, NULL);
	CloseHandle(handle);
	
	//~
	// Find String Table
	//
	if (!success || memcmp(lib, IMAGE_ARCHIVE_START, IMAGE_ARCHIVE_START_SIZE) != 0) {
		VirtualFree(lib, 0, MEM_RELEASE);
		return;
	}
	
	IMAGE_ARCHIVE_MEMBER_HEADER *first_linker_member_header = (IMAGE_ARCHIVE_MEMBER_HEADER *)(lib + IMAGE_ARCHIVE_START_SIZE);
	int member_size = _atoi((char *)first_linker_member_header->Size);
	
	char *second_linker_member = (char *)first_linker_member_header + sizeof(IMAGE_ARCHIVE_MEMBER_HEADER) + member_size + sizeof(IMAGE_ARCHIVE_MEMBER_HEADER);
	
	u32 n_offsets = *(u32 *)(second_linker_member);
	u32 *offsets = (u32 *)(second_linker_member + sizeof(n_offsets));
	
	s32 offsets_size = sizeof(n_offsets) + 4*n_offsets;
	u32 n_symbols = *(u32 *)(second_linker_member + offsets_size);
	u16 *indicies =  (u16 *)(second_linker_member + offsets_size + sizeof(n_symbols));
	
	s32 symbols_size   = sizeof(n_symbols) + 2*n_symbols;
	char *string_table = second_linker_member + offsets_size + symbols_size;
	
	
	//~
	// Search String Table for function names
	//
	unsigned int string_table_idx = 0;
	for (int function_idx = 0; function_idx < external_function_names_length; function_idx++) {
		// Ignore visited functions
		if (visited_functions[function_idx/8] & (1 << (function_idx%8))) continue; 
		
		for (; string_table_idx < n_symbols; string_table_idx++) {
			
			char *target = external_function_names[function_idx];
			while (*target == *string_table && *string_table != '\0') {
				target++, string_table++; 
			}
			// confirm match
			char l = *target|0x60,  r = *string_table|0x60; 
			if (l != r) {
				if (l < r && ('a' < r && r < 'z'))  break;
				while (*string_table++ != '\0');
				continue;
			}
			
			//
			// find the function and dll names inside 
			//
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
			
			OBJECT_HEADER *object_header_info = (OBJECT_HEADER *)(lib + offset + sizeof(IMAGE_ARCHIVE_MEMBER_HEADER));
			char *function_and_dll_names = lib + offset + sizeof(IMAGE_ARCHIVE_MEMBER_HEADER) + sizeof(OBJECT_HEADER);
			
			if (strcmp(function_and_dll_names, external_function_names[function_idx]) != 0) {
				Assert(!"shouldn't really be in here I think. this would mean that there is a bug in my code? regardless I assume things are correct otherwise and I can continue"); 
				while (*string_table++ != '\0');
				continue;
			}
			
			//
			// Copy data 
			//
			String8 dll_name = str8_lit(function_and_dll_names+strlen(external_function_names[function_idx])+1);
			Vector *vec = map_peek(import_entries, dll_name);
			if (!vec) {
				vec = vec_init();
				M_Arena *arena = pool_get_scratch(m);
				// TODO(ziv): should I lowercase dll names? 
				map_set(import_entries, str8_duplicate(arena, dll_name), vec);
			}
			
			vec_push(m, vec, external_function_names[function_idx]);
			visited_functions[function_idx/8] |= 1 << (function_idx%8);
			hints[function_idx] = object_header_info->Hint;
			
			break;
		}
	}
	
	//IMAGE_ARCHIVE_MEMBER_HEADER *function_member_header = (IMAGE_ARCHIVE_MEMBER_HEADER *)(buff + offset);
	/* TODO(ziv): figure out whether I need this extra information for anything useful
	OBJECT_HEADER *obj_header = (OBJECT_HEADER *)(buff + offset + sizeof(IMAGE_ARCHIVE_MEMBER_HEADER));
		int function_member_size = atoi((const char *)function_member_header->Size);
		int import_type      = obj_header->Type & 0x03;
		int import_name_type = (obj_header->Type & 0x1c) >> 2;
		 */
	
	VirtualFree(lib, 0, MEM_RELEASE);
	return;
}


typedef struct { 
	M_Pool *m;
	
	int file_alignment, section_alignment; 
	int first_section_address;
	Vector *headers;
	
	IMAGE_NT_HEADERS *nt_headers;
	
} PE_Packer;

internal int pe_add_section(PE_Packer *pk, String8 section_name, int section_size, int characteristics) {
	Assert(pk && section_size); 
	Assert(0 < section_name.size && section_name.size <= 8);
	
	IMAGE_SECTION_HEADER *section_header = pool_alloc(pk->m, sizeof(IMAGE_SECTION_HEADER)); 
	
	// calculate sizes for section
	IMAGE_SECTION_HEADER *last_section = pk->headers->data[MAX(pk->headers->index-1, 0)];
	
	memcpy(&section_header->Name, section_name.str, section_name.size);
	section_header->Misc.VirtualSize = section_size;
	section_header->VirtualAddress = (last_section) ? ALIGN(last_section->VirtualAddress + last_section->SizeOfRawData, pk->section_alignment) : pk->section_alignment;
	section_header->SizeOfRawData  = ALIGN(section_size, pk->file_alignment);
	section_header->PointerToRawData = (last_section) ? (last_section->PointerToRawData + last_section->SizeOfRawData) : pk->nt_headers->OptionalHeader.SizeOfHeaders;
	section_header->Characteristics = characteristics;
	
	// add to headers vector
	vec_push(pk->m, pk->headers, section_header); 
	
	pk->first_section_address = section_header->VirtualAddress;
	
	return pk->first_section_address;
}

internal bool write_pe_exe(Builder *builder, const char *file, 
						  char **external_library_paths, int  external_library_paths_count) {
	M_Arena *m = pool_get_scratch(&builder->m);
	
	
	//~
	// Create import entries (dll name -> functions to import)
	//
	
	Find_Result msvc_sdk = find_visual_studio_and_windows_sdk(); 
	if (!msvc_sdk.windows_sdk_version) {
		// TODO(ziv): Put an error before exit
		return false; 
	}
	
	size_t extern_functions_len = builder->pls_maps[PL_C_FUNCS].count;
	char **extern_functions = arena_alloc(m, extern_functions_len*sizeof(char *)); 
	
	Map_Iterator it = map_iterator(&builder->pls_maps[PL_C_FUNCS]);
	for (int i = 0; map_next(&it); i++) 
		extern_functions[i] = it.key.str;
	qsort(extern_functions, extern_functions_len, sizeof(char *), compare);
	
	Map *import_entries = init_map(sizeof(Vector *));
	u16 *hints = arena_alloc(m, extern_functions_len*sizeof(u16));
	u8  *visited = arena_alloc(m, (extern_functions_len/8 + 1)*sizeof(u8));
	
	for (int lib_idx = 0; lib_idx < external_library_paths_count; lib_idx++) {
		char *module = external_library_paths[lib_idx];
		
		// base_path_to_msvc_archive_libraries\module
		wchar_t *library_path = msvc_sdk.windows_sdk_um_library_path;
		char *path = arena_alloc(m, wcslen(library_path)+strlen(module)+2), *path_cursor = path; 
		while (*library_path) *path_cursor++ = (char)*library_path++;
		*path_cursor++ = '\\';
		while (*module) *path_cursor++ = *module++;
		*path_cursor = '\0';
		
		parse_archive_library(&builder->m, 
							  import_entries, path, 
							  extern_functions, extern_functions_len, 
							  visited, hints);
	}
	free_resources(&msvc_sdk);
	
	// TODO(ziv): @Incopmlete Confirm there are no linking issues 
	
	
	
	//~
	// PE executable NT signiture, DOS stub, NT headers
	//
	
	static u8 image_stub_and_signiture[0x40] = { 'M', 'Z' }; /* image NT signiture */
	// Set the address at location 0x3c to be after signiture and stub
	*(unsigned int *)&image_stub_and_signiture[0x3c] = sizeof(image_stub_and_signiture);
	
	IMAGE_NT_HEADERS nt_headers = {
		.Signature = 'P' | 'E' << 8,
		
		.FileHeader = { 
			.Machine = _WIN64 ? IMAGE_FILE_MACHINE_AMD64 : IMAGE_FILE_MACHINE_I386,
			.NumberOfSections     = 3,// TODO(ziv): make NumberOfSections dyanmic
			.SizeOfOptionalHeader = 0,
			.Characteristics      = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_LARGE_ADDRESS_AWARE
		}, 
		
		.OptionalHeader = {
			.Magic = IMAGE_NT_OPTIONAL_HDR_MAGIC, 
			
			.MajorOperatingSystemVersion = 6,
			.MajorSubsystemVersion = 6,
			
			.AddressOfEntryPoint = 0x1000,
			
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
	
	int sizeof_headers_unaligned = sizeof(image_stub_and_signiture) + sizeof(nt_headers) + nt_headers.FileHeader.NumberOfSections*sizeof(IMAGE_SECTION_HEADER);
	{
		nt_headers.FileHeader.SizeOfOptionalHeader = sizeof(nt_headers.OptionalHeader);
			nt_headers.OptionalHeader.SizeOfHeaders = ALIGN(sizeof_headers_unaligned, file_alignment);
	}
	
	
	
	//~
	// section headers
	// 
	
	// NOTE(ziv): idata section structure
	// Import Directory Table - contains information about the following structures locations
	// Import Name Table - table of references to tell the loader which functions are needed (NULL terminated)
	// Function Names - A array of the function names we want the loader to load. The structure is 'IMAGE_IMPORT_BY_NAME' (it's two first bytes are a 'HINT')
	// Import Address Table - identical to Import Name Table but during loading gets overriten with valid values for function addresses
	// Module names - simple array of the module names
	
	PE_Packer pk = { &builder->m, file_alignment, section_alignment, .headers = vec_init(), &nt_headers };
	pk.headers->data[0] = NULL;
	
	// always create a text section
	int text_size  = builder->bytes_count;
	 pe_add_section(&pk, str8_lit(".text"),  text_size, IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_CODE);
	
	
	// Compute sizes for all idata related structures
	int int_tables_size = 0, module_names_size = 0, function_names_size = 0; 
	Map_Iterator iter = map_iterator(import_entries); 
	for (int i = 0; map_next(&iter); i++) {
		String8 dll = iter.key;
		Vector *function_names_vector = iter.value;
		int fn_idx = 0;
		for (; fn_idx < function_names_vector->index; fn_idx++) 
			function_names_size += (int)(2 + strlen(function_names_vector->data[fn_idx]) + 1); // +2 for 'HINT'
		int_tables_size += (int)(sizeof(size_t) * (function_names_vector->index+1));
		module_names_size += (int)(dll.size + 1);
	}
	int descriptors_size = (import_entries->count > 0) ? (sizeof(IMAGE_IMPORT_DESCRIPTOR) * (import_entries->count + 1)) : 0;
	int idata_size = descriptors_size + int_tables_size*2 + function_names_size + module_names_size;
	
	if (idata_size > 0 && extern_functions_len > 0) {
	int idata_section_address = pe_add_section(&pk, str8_lit(".idata"), idata_size, IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_INITIALIZED_DATA);
		
		// importsVA - make sure that idata section is recognized as a imports section
		nt_headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = idata_section_address; 
		nt_headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = idata_size; 
	}
	
	
	int data_size  = builder->current_data_variable_location; 
	if (data_size > 0) {
		pe_add_section(&pk, str8_lit(".data"), data_size, IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_INITIALIZED_DATA);
	}
	
	
	
	nt_headers.FileHeader.NumberOfSections = (WORD)pk.headers->index; 
	nt_headers.OptionalHeader.SizeOfImage  = ((IMAGE_SECTION_HEADER *)pk.headers->data[pk.headers->index-1])->VirtualAddress + section_alignment;
	
	//~
	// Creating and filling executable
	//
	
	int sections_size = 0; 
	for (int i = 0; i < pk.headers->index; i++) {
		sections_size += ((IMAGE_SECTION_HEADER *)pk.headers->data[i])->SizeOfRawData;
	}
	size_t executable_size = nt_headers.OptionalHeader.SizeOfHeaders + sections_size;
		u8 *pe = VirtualAlloc(NULL, executable_size, MEM_COMMIT, PAGE_READWRITE); 
	if (!pe) return false;
	
	u8 *pexe = pe;
	
	
	// filling signiture, headers, sections
	{
#define MOVE(dest, src, size) do { memcpy(dest, src, size); dest += size; } while(0)
		MOVE(pexe, image_stub_and_signiture, sizeof(image_stub_and_signiture));
		MOVE(pexe, &nt_headers,    sizeof(nt_headers));
		for (int i = 0; i < pk.headers->index; i++) {
			MOVE(pexe, pk.headers->data[i], sizeof(IMAGE_SECTION_HEADER));
		}
	}
	
	
	u8 *text  = pe + nt_headers.OptionalHeader.SizeOfHeaders;
	u8 *idata = text + ((IMAGE_SECTION_HEADER *)pk.headers->data[0])->SizeOfRawData;
	//u8 *data  = idata + ((IMAGE_SECTION_HEADER *)pk.headers->data[1])->SizeOfRawData;; 
	
	// filling .text section
	memcpy(text, builder->code, text_size);

/* 	
	// filling .data section
	for (int i = 0; i < builder->pls_cnt[PL_DATA_VARS]; i++) {
		char *name = builder->pls[PL_DATA_VARS][i].name;
		size_t copy_size = strlen(name)+1;
		memcpy(data, name, copy_size); data += copy_size;
	}
	 */

	if (idata_size > 0 && extern_functions_len > 0) {
		
		// filling .idata section
		int descriptors_base_address    = nt_headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
		int int_table_base_address      = descriptors_base_address + descriptors_size;
		int function_names_base_address = int_table_base_address + int_tables_size;
		int iat_table_base_address      = function_names_base_address + function_names_size;
		int module_names_base_address   = iat_table_base_address + int_tables_size;
		
		size_t *int_tables = (size_t *)(idata + descriptors_size); 
		u8 *function_names = (u8 *)int_tables + int_tables_size;
		size_t *iat_tables = (size_t *)(function_names + function_names_size);
		u8 *module_names   = (u8 *)iat_tables + int_tables_size;
		
		int int_tables_offset = 0, module_names_offset = 0, function_names_offset = 0; 
		
		int hint_idx = 0;
		iter = map_iterator(import_entries); 
		for (int i = 0; map_next(&iter); i++) {
			String8 dll = iter.key; dll.str[dll.size] = '\0';
			Vector *function_names_vector = iter.value;
			
#if DEBUG > 1
			printf(" -- Descriptor%d -- \n", i);
			printf("int-table \t%x -> ", int_table_base_address + int_tables_offset);
			printf("%x,", function_names_base_address + function_names_offset);
			printf("0      \\ \n");
			printf("module-name \t%x -> %s | -> function names\n", module_names_base_address + module_names_offset, dll.str);
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
			for (int fn_idx = 0; fn_idx < function_names_vector->index; fn_idx++) {
				memcpy(&function_names[function_names_offset], &hints[hint_idx++], sizeof(u16));
				int copy_size = (int)strlen(function_names_vector->data[fn_idx])+1;
				memcpy(&function_names[function_names_offset+sizeof(u16)], function_names_vector->data[fn_idx], copy_size);
				*int_tables++ = *iat_tables++ = (size_t)function_names_base_address + function_names_offset;
				function_names_offset += copy_size+2;
				
			}
			int_tables_offset += (int)(sizeof(size_t) * (function_names_vector->index+1)); 
			int_tables++; iat_tables++; // for NULL address
			
			// filling module names 
			memcpy(&module_names[module_names_offset], dll.str, dll.size+1);
			module_names_offset += (int)dll.size+1;
		}
		
	}
	
	
	
	//~
	// Creating final executable
	//
	
	{
		HANDLE handle = CreateFileA(file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		Assert(handle != INVALID_HANDLE_VALUE && "Invalid handle");
		
		DWORD bytes_written;
		BOOL result =  WriteFile(handle, pe, (int)executable_size, &bytes_written, NULL);
		Assert(result && "Couldn't write to file");
		CloseHandle(handle);
	}
	
	return 0;
}

