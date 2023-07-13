
//~
// PE Executable
//

internal inline int align(int x, int alignment) {
	int remainder = x % alignment; 
	return remainder ? (x - remainder + alignment) : x;
}

internal int write_pe_exe(const char *file, 
						  Builder *builder) {
	
	// find out whether all external functions have their references in known locations
	
	
	typedef struct {
		char *dll_name;
		char **function_names;
	} Entry; 
	
	// TODO(ziv): @Incomplete currently I assume that the all archive file will have functions, which might not be the case. Find a way to remove unwanted entries before I forward them to the creation of the idate section
	
			// create array of function library pairs
		Function_Library *function_library_array = malloc(builder->jumpinstructions_count * sizeof(Function_Library)); 
		int function_library_array_size = 0; 
		for (int ji_idx = 0; ji_idx < builder->jumpinstructions_count; ji_idx++) {
			Name_Location *nl = &builder->jumpinstructions[ji_idx];
			Function_Library *fl = &function_library_array[function_library_array_size++]; 
			fl->function = nl->name; 
			fl->library = NULL;
		}
		
		// sort the names 
		{
			
		}
		
		
	int *func_count_in_lib = (int *)malloc(builder->external_library_paths_count * sizeof(int)); 
	memset(func_count_in_lib, 0, builder->external_library_paths_count*sizeof(int));
	
		// fill all the slots with dll names 
		for (int lib_idx = 0; lib_idx < builder->external_library_paths_count; lib_idx++) {

						char *module = builder->external_library_paths[lib_idx];
			if (!fill_modules_for_function(builder, module, function_library_array, function_library_array_size, 
											   &func_count_in_lib[lib_idx])) {
				// error out of here since I am unable for some reason to fill the modules
				return false; 
			}
			 
		}
	
	
	int entries_count = builder->external_library_paths_count;
	Entry *entries = (Entry *)malloc(entries_count * sizeof(Entry)); 
	
	for (int i = 0; i < entries_count; i++) {
		Entry *e = &entries[i]; 
		e->function_names = (char **)malloc(func_count_in_lib[i]*sizeof(char *));
	}
	
	
	int x = entries_count; // dll name count next power of two (for table)
	// round to next power of two 
	x--;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x++; 
	x = x << 1;
	// check whether they are all actually filled & fill entries structure
	for (int i = 0; i < function_library_array_size; i++) {
		Function_Library *fl = &function_library_array[i];
		if (!fl->library) {
			// link error couldn't find reference for %s
			return false; 
		}
		
		
		// TODO(ziv): Finish implementing this @Incomplete
		int *dll_name_t = (int *)malloc(x * sizeof(int)); 
		int index = ((size_t)(fl->library) >> 8) & (x-1);
		dll_name_t[index] = 0;
		
		fl->function;
		fl->library; 
		
	}
	
//
	// PE executable NT signiture, DOS stub, NT headers
	//
	
	// NOTE(ziv): 0x4d 0x5a stands for 'MZ' which are a must in the NT signiture
	static u8 image_stub_and_signiture[0x40] = { 0x4D, 0x5A }; /* image NT signiture */
	// Set the address at location 0x3c to be after signiture and stub
	*(unsigned int *)&image_stub_and_signiture[0x3c] = sizeof(image_stub_and_signiture);
	
	IMAGE_NT_HEADERS nt_headers = {
		.Signature = 'P' | 'E' << 8,
		
		.FileHeader = { 
			.Machine = _WIN64 ? IMAGE_FILE_MACHINE_AMD64 : IMAGE_FILE_MACHINE_I386,
			.NumberOfSections     = 3,
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
	int descriptors_size = sizeof(IMAGE_IMPORT_DESCRIPTOR) * (entries_count + 1);
	int int_tables_size = 0, module_names_size = 0, function_names_size = 0; 
	for (int i = 0; i < entries_count; i++) {
		Entry entry = entries[i];
		char **fnames = entry.function_names, **fnames_temp = fnames;
		for (; *fnames_temp; fnames_temp++) 
			function_names_size += (int)(strlen(*fnames_temp+2) + 1 + 2); // +2 for 'HINT'
		int_tables_size += (int)(sizeof(size_t) * (fnames_temp-fnames+1));
		module_names_size += (int)(strlen(entry.dll_name) + 1);
	}
	int idata_size = descriptors_size + int_tables_size*2 + function_names_size + module_names_size;
			
	
	IMAGE_SECTION_HEADER text_section = { 
		.Name = ".text",
		.Misc.VirtualSize = 0x1000, // set actual size 
		.VirtualAddress   = section_alignment, 
		.SizeOfRawData    = align(builder->code_size, file_alignment),
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
		.Misc.VirtualSize = builder->data_size, 
		.VirtualAddress   = align(idata_section.VirtualAddress + idata_section.SizeOfRawData, section_alignment), 
		.SizeOfRawData    = align(builder->data_size, file_alignment), 
		.PointerToRawData = idata_section.PointerToRawData + idata_section.SizeOfRawData, 
		.Characteristics  = IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_INITIALIZED_DATA,
	};
	
	//~
	// Creating and filling .idata section
	//
	
	size_t executable_size = nt_headers.OptionalHeader.SizeOfHeaders + 
		text_section.SizeOfRawData + idata_section.SizeOfRawData + data_section.SizeOfRawData;
	// Allocate once for both idata buffer and exe
	u8 *buf = VirtualAlloc(NULL, idata_size + executable_size, MEM_COMMIT, PAGE_READWRITE); 
	u8 *idata = buf, *executable = buf+idata_size, *pexe = executable;
	
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
	for (int i = 0; i < entries_count; i++) {
#if DEBUG > 1
		printf(" -- Descriptor%d -- \n", i);
		printf("int-table \t%x -> ", int_table_base_address + int_tables_offset);
		printf("%x,", function_names_base_address + function_names_offset);
		printf("0      \\ \n");
		printf("module-name \t%x -> %s | -> function names\n", module_names_base_address + module_names_offset, entries[i].dll_name);
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
			int copy_size = (int)strlen(*fnames+2)+1+2;
			// NOTE(ziv): first two bytes are used for 'HINT'
			memcpy(function_names + function_names_offset, *fnames, copy_size); 
			// fill the function's addresses into the INT and IAT tables. 
			*int_tables++ = *iat_tables++ = (size_t)function_names_base_address + function_names_offset;
			function_names_offset += copy_size; 
		}
		int_tables_offset += (int)(sizeof(size_t) * ((fnames-fnames_start)+1)); 
		int_tables++; iat_tables++; // for NULL address
		
		// filling module names 
		size_t copy_size = strlen(entries[i].dll_name)+1; 
		memcpy(module_names + module_names_offset, entries[i].dll_name, copy_size);
		module_names_offset += (int)copy_size;
	}
	
	
	//~
	// Patch real addresses to code
	//
	
	// Fill addresses to functions from dynamic libraries 
	{
		
	}
	
	
	
	printf("Before: ");  
	for (int i = 0; i < builder->bytes_count; i++) { printf("%02x", builder->code[i]&0xff); } printf("\n"); 
	/* 	
		 */
	
	// maybe I should just use relative locations for all of those things? 
	// this is a good question to be asked of how to handle it.
	int base = 0x4000;
	for (int i = 0; i < builder->data_variables_count; i++) {
		Name_Location nl = builder->data_variables[i];
		int location = nl.location + base;
		memcpy(&builder->code[nl.location_to_patch], &location, sizeof(nl.location));
	}
	
	base = 0x1000;
	for (int i = 0; i < builder->labels_count; i++) {
		Name_Location nl = builder->labels[i];
		int location = nl.location + base;
		memcpy(&builder->code[nl.location_to_patch], &location, sizeof(nl.location));
	}
	
	// this might be more specific? 
	base = 0x9000;
	for (int i = 0; i < builder->jumpinstructions_count; i++) {
		Name_Location nl = builder->jumpinstructions[i];
		int location = nl.location + base;
		memcpy(&builder->code[nl.location_to_patch], &location, sizeof(nl.location));
		printf("path location %d\n", nl.location_to_patch);
	}
	
	printf("After: ");
	for (int i = 0; i < builder->bytes_count; i++) { printf("%02x", builder->code[i]&0xff); } printf("\n"); 
	/* 	 
		 */
	
	
	//~
	// Creating final executable
	//
	
	{
#define MOVE(dest, src, size) do { memcpy(dest, src, size); dest += size; } while(0)
		MOVE(pexe, image_stub_and_signiture, sizeof(image_stub_and_signiture)); 
		MOVE(pexe, &nt_headers,    sizeof(nt_headers));
		MOVE(pexe, &text_section,  sizeof(text_section));
		MOVE(pexe, &idata_section, sizeof(idata_section));
		MOVE(pexe, &data_section,  sizeof(data_section)); pexe += nt_headers.OptionalHeader.SizeOfHeaders - sizeof_headers_unaligned;
		MOVE(pexe, builder->code,  builder->code_size);  pexe += text_section.SizeOfRawData  - builder->code_size;
		MOVE(pexe, idata, idata_size);          pexe += idata_section.SizeOfRawData - idata_size;
		MOVE(pexe, builder->data,  builder->data_size);  pexe += data_section.SizeOfRawData  - builder->data_size;
	}
	
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