


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

internal int write_pe_exe(Builder *builder, const char *file) {
	M_Arena *m = pool_get_scratch(&builder->m);
	
	
	//~
	// Create import entries (dll name -> functions to import)
	//
	
	Find_Result msvc_sdk = find_visual_studio_and_windows_sdk(); 
	if (msvc_sdk.windows_sdk_version) {
		// TODO(ziv): Put an error before exit
		return false; 
	}
	
	size_t extern_functions_len = builder->jumpinstructions_count;
	char **extern_functions = arena_alloc(m, extern_functions_len*sizeof(char *)); 
	for (int i = 0; i < extern_functions_len; i++) 
		extern_functions[i] = builder->jumpinstructions[i].name;
	qsort(extern_functions, extern_functions_len, sizeof(char *), compare);
	
	Map *import_entries = init_map(sizeof(Vector *));
	u16 *hints = arena_alloc(m, extern_functions_len*sizeof(u16));
	u8  *visited = arena_alloc(m, (extern_functions_len/8 + 1)*sizeof(u8));
	
	for (int lib_idx = 0; lib_idx < builder->external_library_paths_count; lib_idx++) {
		char *module = builder->external_library_paths[lib_idx];
		
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
		nt_headers.FileHeader.NumberOfSections = 3; 
		
		nt_headers.OptionalHeader.AddressOfEntryPoint = 0x1000;
		nt_headers.OptionalHeader.SizeOfImage   = 0x4000; // TODO(ziv): automate this
		nt_headers.OptionalHeader.SizeOfHeaders = ALIGN(sizeof_headers_unaligned, file_alignment);
	}
	
	
	
	
	//~
	// Compute .idata size from import entries
	//
	
	// NOTE(ziv): idata section structure
	// Import Directory Table - contains information about the following structures locations
	// Import Name Table - table of references to tell the loader which functions are needed (NULL terminated)
	// Function Names - A array of the function names we want the loader to load. The structure is 'IMAGE_IMPORT_BY_NAME' (it's two first bytes are a 'HINT')
	// Import Address Table - identical to Import Name Table but during loading gets overriten with valid values for function addresses
	// Module names - simple array of the module names
	
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
	
	int descriptors_size = sizeof(IMAGE_IMPORT_DESCRIPTOR) * (import_entries->count + 1);
	int idata_size = descriptors_size + int_tables_size*2 + function_names_size + module_names_size;
	
	int data_size  = builder->current_data_variable_location; 
	int text_size  = builder->bytes_count;
	
	
	
	//~
	// .text .data .idata section headers
	// 
	
	IMAGE_SECTION_HEADER text_section = { 
		.Name = ".text",
		.Misc.VirtualSize = text_size,
		.VirtualAddress   = section_alignment, 
		.SizeOfRawData    = ALIGN(text_size, file_alignment),
		.PointerToRawData = nt_headers.OptionalHeader.SizeOfHeaders, // the sections begin right after all the NT headers
		.Characteristics  = IMAGE_SCN_MEM_EXECUTE |IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_CODE
	}; 
	
	int idata_section_address = ALIGN(text_section.VirtualAddress + text_section.SizeOfRawData, section_alignment);
	IMAGE_SECTION_HEADER idata_section = { 
		.Name = ".idata",
		.Misc.VirtualSize = idata_size, 
		.VirtualAddress   = idata_section_address,
		.SizeOfRawData    = ALIGN(idata_size, file_alignment),
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
		.VirtualAddress   = ALIGN(idata_section.VirtualAddress + idata_section.SizeOfRawData, section_alignment), 
		.SizeOfRawData    = ALIGN(data_size, file_alignment), 
		.PointerToRawData = idata_section.PointerToRawData + idata_section.SizeOfRawData, 
		.Characteristics  = IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_INITIALIZED_DATA,
	};
	
	
	
	//~
	// Creating and filling sections
	//
	
	size_t executable_size = nt_headers.OptionalHeader.SizeOfHeaders + 
		text_section.SizeOfRawData + idata_section.SizeOfRawData + data_section.SizeOfRawData;
	// Allocate once for both for exe
	u8 *buf = VirtualAlloc(NULL, executable_size, MEM_COMMIT, PAGE_READWRITE); 
	u8 *executable = buf, *pexe = executable;
	
	u8 *text  = buf + nt_headers.OptionalHeader.SizeOfHeaders;
	u8 *idata = text + text_section.SizeOfRawData;
	u8 *data  = idata + idata_section.SizeOfRawData; 
	
	// filling .text section
	memcpy(text, builder->code, text_size);
	
	// filling .data section
	for (int i = 0; i < builder->data_variables_count; i++) {
		Name_Location nl = builder->data_variables[i];
		size_t copy_size = strlen(nl.name)+1;
		memcpy(data, nl.name, copy_size); data += copy_size;
	}
	
	if (extern_functions_len > 0) {
		
		// filling .idata section
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
			size_t copy_size = dll.size+1; 
			memcpy(&module_names[module_names_offset], dll.str, copy_size);
			module_names_offset += (int)copy_size;
		}
		
	}
	
	
	
	//~
	// Patch real addresses to code
	//
	
	/*
		printf("Before: ");  
		for (int i = 0; i < builder->bytes_count; i++) { printf("%02x", builder->code[i]&0xff); } printf("\n"); 
		 */
	
	// the patching should be done by a "linker" 
	
#if 1
	// patching data section
	for (int i = 0; i < builder->data_variables_count; i++) {
		Name_Location nl = builder->data_variables[i];
		int location = data_section.VirtualAddress + nl.location;
		memcpy(&builder->code[nl.location_to_patch], &location, sizeof(nl.location));
	}
	
	// patching text section
	for (int i = 0; i < builder->labels_count; i++) {
		Name_Location nl = builder->labels[i];
		int location = text_section.VirtualAddress + nl.location;
		memcpy(&builder->code[nl.location_to_patch], &location, sizeof(nl.location));
	}
	
	for (int i = 0; i < builder->jumpinstructions_count; i++) {
		Name_Location nl = builder->jumpinstructions[i];
		int location = text_section.VirtualAddress + nl.location;
		memcpy(&builder->code[nl.location_to_patch], &location, sizeof(nl.location));
		printf("path location %d\n", nl.location_to_patch);
	}
#endif 
	
	/* 	
		printf("After: ");
		for (int i = 0; i < builder->bytes_count; i++) { printf("%02x", builder->code[i]&0xff); } printf("\n"); 
		 */
	
	// filling signiture, headers, sections
	{
#define MOVE(dest, src, size) do { memcpy(dest, src, size); dest += size; } while(0)
		MOVE(pexe, image_stub_and_signiture, sizeof(image_stub_and_signiture));
		MOVE(pexe, &nt_headers,    sizeof(nt_headers));
		MOVE(pexe, &text_section,  sizeof(text_section));
		MOVE(pexe, &idata_section, sizeof(idata_section));
		MOVE(pexe, &data_section,  sizeof(data_section));
	}
	
	//~
	// Creating final executable
	//
	
	{
		HANDLE handle = CreateFileA(file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		Assert(handle != INVALID_HANDLE_VALUE && "Invalid handle");
		
		DWORD bytes_written;
		BOOL result =  WriteFile(handle, executable, (int)executable_size, &bytes_written, NULL);
		Assert(result && "Couldn't write to file");
		CloseHandle(handle);
	}
	
	return 0;
}

