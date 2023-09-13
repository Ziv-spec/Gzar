// From:     https://github.com/janivanecky/builder/blob/master/microsoft_craziness.h
// Author:   Jonathan Blow
// Version:  1
// Date:     31 August, 2018
//
// This code is released under the MIT license, which you can find at
//
//          https://opensource.org/licenses/MIT
//
//
//
// See the comments for how to use this library just below the includes.
//


//
// NOTE(Kalinovcic): I have translated the original implementation to C,
// and made a preprocessor define that enables people to include it without
// the implementation, just as a header.
//
// I also fixed two bugs:
//   - If COM initialization for VS2017 fails, we actually properly continue
//     searching for earlier versions in the registry.
//   - For earlier versions, the code now returns the "bin\amd64" VS directory.
//     Previously it was returning the "bin" directory, which is for x86 stuff.
//
// To include the implementation, before including microsoft_craziness.h, define:
//
//   #define MICROSOFT_CRAZINESS_IMPLEMENTATION
//   #include "microsoft_craziness.h"
//

// 
// NOTE(ziv): 
// Removed functions not used by the compiler to remove bloat and confusion 
// around what is used and what is not used for the specific case, along with 
// a reduction in total loc
//

#ifndef MICROSOFT_CRAZINESS_HEADER_GUARD
#define MICROSOFT_CRAZINESS_HEADER_GUARD
	
	typedef struct
	{
		int windows_sdk_version;   // Zero if no Windows SDK found.
		
		wchar_t *windows_sdk_root;
		wchar_t *windows_sdk_um_library_path;
		wchar_t *windows_sdk_ucrt_library_path;
	} Find_Result;
	
	Find_Result find_visual_studio_and_windows_sdk();
	void free_resources(Find_Result *result);
	
	
	//
	// HOW TO USE THIS CODE
	//
	// The purpose of this file is to find the folders that contain libraries
	// you may need to link against, on Windows, if you are linking with any
	// compiled C or C++ code. This will be necessary for many non-C++ programming
	// language environments that want to provide compatibility.
	//
	// We find the place where the Visual Studio libraries live (for example,
	// libvcruntime.lib), where the linker and compiler executables live
	// (for example, link.exe), and where the Windows SDK libraries reside
	// (kernel32.lib, libucrt.lib).
	//
	// We all wish you didn't have to worry about so many weird dependencies,
	// but we don't really have a choice about this, sadly.
	//
	// I don't claim that this is the absolute best way to solve this problem,
	// and so far we punt on things (if you have multiple versions of Visual Studio
	// installed, we return the first one, rather than the newest). But it
	// will solve the basic problem for you as simply as I know how to do it,
	// and because there isn't too much code here, it's easy to modify and expand.
	//
	//
	// Here is the API you need to know about:
	//
	
	
	//
	// Call find_visual_studio_and_windows_sdk, look at the resulting
	// paths, then call free_resources on the result.
	//
	// Everything else in this file is implementation details that you
	// don't need to care about.
	//
	
	//
	// This file was about 400 lines before we started adding these comments.
	// You might think that's way too much code to do something as simple
	// as finding a few library and executable paths. I agree. However,
	// Microsoft's own solution to this problem, called "vswhere", is a
	// mere EIGHT THOUSAND LINE PROGRAM, spread across 70 files,
	// that they posted to github *unironically*.
	//
	// I am not making this up: https://github.com/Microsoft/vswhere
	//
	// Several people have therefore found the need to solve this problem
	// themselves. We referred to some of these other solutions when
	// figuring out what to do, most prominently ziglang's version,
	// by Ryan Saunderson.
	//
	// I hate this kind of code. The fact that we have to do this at all
	// is stupid, and the actual maneuvers we need to go through
	// are just painful. If programming were like this all the time,
	// I would quit.
	//
	// Because this is such an absurd waste of time, I felt it would be
	// useful to package the code in an easily-reusable way, in the
	// style of the stb libraries. We haven't gone as all-out as some
	// of the stb libraries do (which compile in C with no includes, often).
	// For this version you need C++ and the headers at the top of the file.
	//
	// We return the strings as Windows wide character strings. Aesthetically
	// I don't like that (I think most sane programs are UTF-8 internally),
	// but apparently, not all valid Windows file paths can even be converted
	// correctly to UTF-8. So have fun with that. It felt safest and simplest
	// to stay with wchar_t since all of this code is fully ensconced in
	// Windows crazy-land.
	//
	// One other shortcut I took is that this is hardcoded to return the
	// folders for x64 libraries. If you want x86 or arm, you can make
	// slight edits to the code below, or, if enough people want this,
	// I can work it in here.
	//
	
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <io.h>         // For _get_osfhandle
	
	void free_resources(Find_Result *result) {
		free(result->windows_sdk_root);
		free(result->windows_sdk_um_library_path);
	free(result->windows_sdk_ucrt_library_path);
	}
	
	
	// COM objects for the ridiculous Microsoft craziness.
	
#undef  INTERFACE
#define INTERFACE ISetupInstance
	DECLARE_INTERFACE_ (ISetupInstance, IUnknown)
	{
		BEGIN_INTERFACE
			
			// IUnknown methods
			STDMETHOD (QueryInterface)   (THIS_  REFIID, void **) PURE;
		STDMETHOD_(ULONG, AddRef)    (THIS) PURE;
		STDMETHOD_(ULONG, Release)   (THIS) PURE;
		
		// ISetupInstance methods
		STDMETHOD(GetInstanceId)(THIS_ _Out_ BSTR* pbstrInstanceId) PURE;
		STDMETHOD(GetInstallDate)(THIS_ _Out_ LPFILETIME pInstallDate) PURE;
		STDMETHOD(GetInstallationName)(THIS_ _Out_ BSTR* pbstrInstallationName) PURE;
		STDMETHOD(GetInstallationPath)(THIS_ _Out_ BSTR* pbstrInstallationPath) PURE;
		STDMETHOD(GetInstallationVersion)(THIS_ _Out_ BSTR* pbstrInstallationVersion) PURE;
		STDMETHOD(GetDisplayName)(THIS_ _In_ LCID lcid, _Out_ BSTR* pbstrDisplayName) PURE;
		STDMETHOD(GetDescription)(THIS_ _In_ LCID lcid, _Out_ BSTR* pbstrDescription) PURE;
		STDMETHOD(ResolvePath)(THIS_ _In_opt_z_ LPCOLESTR pwszRelativePath, _Out_ BSTR* pbstrAbsolutePath) PURE;
		
		END_INTERFACE
	};
	
#undef  INTERFACE
#define INTERFACE IEnumSetupInstances
	DECLARE_INTERFACE_ (IEnumSetupInstances, IUnknown)
	{
		BEGIN_INTERFACE
			
			// IUnknown methods
			STDMETHOD (QueryInterface)   (THIS_  REFIID, void **) PURE;
		STDMETHOD_(ULONG, AddRef)    (THIS) PURE;
		STDMETHOD_(ULONG, Release)   (THIS) PURE;
		
		// IEnumSetupInstances methods
		STDMETHOD(Next)(THIS_ _In_ ULONG celt, _Out_writes_to_(celt, *pceltFetched) ISetupInstance** rgelt, _Out_opt_ _Deref_out_range_(0, celt) ULONG* pceltFetched) PURE;
		STDMETHOD(Skip)(THIS_ _In_ ULONG celt) PURE;
		STDMETHOD(Reset)(THIS) PURE;
		STDMETHOD(Clone)(THIS_ _Deref_out_opt_ IEnumSetupInstances** ppenum) PURE;
		
		END_INTERFACE
	};
	
#undef  INTERFACE
#define INTERFACE ISetupConfiguration
	DECLARE_INTERFACE_ (ISetupConfiguration, IUnknown)
	{
		BEGIN_INTERFACE
			
			// IUnknown methods
			STDMETHOD (QueryInterface)   (THIS_  REFIID, void **) PURE;
		STDMETHOD_(ULONG, AddRef)    (THIS) PURE;
		STDMETHOD_(ULONG, Release)   (THIS) PURE;
		
		// ISetupConfiguration methods
		STDMETHOD(EnumInstances)(THIS_ _Out_ IEnumSetupInstances** ppEnumInstances) PURE;
		STDMETHOD(GetInstanceForCurrentProcess)(THIS_ _Out_ ISetupInstance** ppInstance) PURE;
		STDMETHOD(GetInstanceForPath)(THIS_ _In_z_ LPCWSTR wzPath, _Out_ ISetupInstance** ppInstance) PURE;
		
		END_INTERFACE
	};
	
#ifdef __cplusplus
#define CALL_STDMETHOD(object, method, ...) object->method(__VA_ARGS__)
#define CALL_STDMETHOD_(object, method) object->method()
#else
#define CALL_STDMETHOD(object, method, ...) object->lpVtbl->method(object, __VA_ARGS__)
#define CALL_STDMETHOD_(object, method) object->lpVtbl->method(object)
#endif
	
	
	// The beginning of the actual code that does things.
	
	typedef struct {
		int32_t best_version[4];  // For Windows 8 versions, only two of these numbers  are used.
		wchar_t *best_name;
	} Version_Data;
	
	bool os_file_exists(wchar_t *name) {
		// @Robustness: What flags do we really want to check here?
		
		DWORD attrib = GetFileAttributesW(name);
		if (attrib == INVALID_FILE_ATTRIBUTES) return false;
		if (attrib & FILE_ATTRIBUTE_DIRECTORY) return false;
		
		return true;
	}
	
#define concat2(a, b) concat(a, b, NULL, NULL)
#define concat3(a, b, c) concat(a, b, c, NULL)
#define concat4(a, b, c, d) concat(a, b, c, d)
	wchar_t *concat(wchar_t *a, wchar_t *b, wchar_t *c, wchar_t *d) {
		// Concatenate up to 4 wide strings together. Allocated with malloc.
		// If you don't like that, use a programming language that actually
		// helps you with using custom allocators. Or just edit the code.
		
		size_t len_a = wcslen(a);
		size_t len_b = wcslen(b);
		
		size_t len_c = 0;
		if (c) len_c = wcslen(c);
		
		size_t len_d = 0;
		if (d) len_d = wcslen(d);
		
		wchar_t *result = (wchar_t *)malloc((len_a + len_b + len_c + len_d + 1) * 2);
		memcpy(result, a, len_a*2);
		memcpy(result + len_a, b, len_b*2);
		
		if (c) memcpy(result + len_a + len_b, c, len_c * 2);
		if (d) memcpy(result + len_a + len_b + len_c, d, len_d * 2);
		
		result[len_a + len_b + len_c + len_d] = 0;
		
		return result;
	}
	
	typedef void (*Visit_Proc_W)(wchar_t *short_name, wchar_t *full_name, Version_Data *data);
	bool visit_files_w(wchar_t *dir_name, Version_Data *data, Visit_Proc_W proc) {
		
		// Visit everything in one folder (non-recursively). If it's a directory
		// that doesn't start with ".", call the visit proc on it. The visit proc
		// will see if the filename conforms to the expected versioning pattern.
		
		WIN32_FIND_DATAW find_data;
		
		wchar_t *wildcard_name = concat2(dir_name, L"\\*");
		HANDLE handle = FindFirstFileW(wildcard_name, &find_data);
		free(wildcard_name);
		
		if (handle == INVALID_HANDLE_VALUE) return false;
		
		while (true) {
			if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (find_data.cFileName[0] != '.')) {
				wchar_t *full_name = concat3(dir_name, L"\\", find_data.cFileName);
				proc(find_data.cFileName, full_name, data);
				free(full_name);
			}
			
			BOOL success = FindNextFileW(handle, &find_data);
			if (!success) break;
		}
		
		FindClose(handle);
		
		return true;
	}
	
	
	wchar_t *find_windows_kit_root_with_key(HKEY key, wchar_t *version) {
		// Given a key to an already opened registry entry,
		// get the value stored under the 'version' subkey.
		// If that's not the right terminology, hey, I never do registry stuff.
		
		DWORD required_length;
		int rc = RegQueryValueExW(key, version, NULL, NULL, NULL, &required_length);
		if (rc != 0)  return NULL;
		
		DWORD length = required_length + 2;  // The +2 is for the maybe optional zero later on. Probably we are over-allocating.
		wchar_t *value = (wchar_t *)malloc(length);
		if (!value)  return NULL;
		
		rc = RegQueryValueExW(key, version, NULL, NULL, (LPBYTE)value, &length);  // We know that version is zero-terminated...
		if (rc != 0)  return NULL;
		
		// The documentation says that if the string for some reason was not stored
		// with zero-termination, we need to manually terminate it. Sigh!!

		if (value[length/2]) {
			value[length/2] = 0;
		}

		return value;
	}
	
	void win10_best(wchar_t *short_name, wchar_t *full_name, Version_Data *data) {
		// Find the Windows 10 subdirectory with the highest version number.
		
		int i0, i1, i2, i3;
		int success = swscanf_s(short_name, L"%d.%d.%d.%d", &i0, &i1, &i2, &i3);
		if (success < 4) return;
		
		if (i0 < data->best_version[0]) return;
		else if (i0 == data->best_version[0]) {
			if (i1 < data->best_version[1]) return;
			else if (i1 == data->best_version[1]) {
				if (i2 < data->best_version[2]) return;
				else if (i2 == data->best_version[2]) {
					if (i3 < data->best_version[3]) return;
				}
			}
		}
		
		// we have to copy_string and free here because visit_files free's the full_name string
		// after we execute this function, so Win*_Data would contain an invalid pointer.
		if (data->best_name) free(data->best_name);
		data->best_name = _wcsdup(full_name);
		
		if (data->best_name) {
			data->best_version[0] = i0;
			data->best_version[1] = i1;
			data->best_version[2] = i2;
			data->best_version[3] = i3;
		}
	}
	
	void win8_best(wchar_t *short_name, wchar_t *full_name, Version_Data *data) {
		// Find the Windows 8 subdirectory with the highest version number.
		
		int i0, i1;
		int success = swscanf_s(short_name, L"winv%d.%d", &i0, &i1);
		if (success < 2) return;
		
		if (i0 < data->best_version[0]) return;
		else if (i0 == data->best_version[0]) {
			if (i1 < data->best_version[1]) return;
		}
		
		// we have to copy_string and free here because visit_files free's the full_name string
		// after we execute this function, so Win*_Data would contain an invalid pointer.
		if (data->best_name) free(data->best_name);
		data->best_name = _wcsdup(full_name);
		
		if (data->best_name) {
			data->best_version[0] = i0;
			data->best_version[1] = i1;
		}
	}
	
	void find_windows_kit_root(Find_Result *result) {
		// Information about the Windows 10 and Windows 8 development kits
		// is stored in the same place in the registry. We open a key
		// to that place, first checking preferntially for a Windows 10 kit,
		// then, if that's not found, a Windows 8 kit.
		
		HKEY main_key;
		
		LSTATUS rc = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots",
								   0, KEY_QUERY_VALUE | KEY_WOW64_32KEY | KEY_ENUMERATE_SUB_KEYS, &main_key);
		if (rc != S_OK) return;
		
		// Look for a Windows 10 entry.
		wchar_t *windows10_root = find_windows_kit_root_with_key(main_key, L"KitsRoot10");
		
		if (windows10_root) {
			wchar_t *windows10_lib = concat2(windows10_root, L"Lib");
			free(windows10_root);
			
			Version_Data data = {0};
			visit_files_w(windows10_lib, &data, win10_best);
			free(windows10_lib);
			
			if (data.best_name) {
				result->windows_sdk_version = 10;
				result->windows_sdk_root = data.best_name;
				RegCloseKey(main_key);
				return;
			}
		}
		
		// Look for a Windows 8 entry.
		wchar_t *windows8_root = find_windows_kit_root_with_key(main_key, L"KitsRoot81");
		
		if (windows8_root) {
			wchar_t *windows8_lib = concat2(windows8_root, L"Lib");
			free(windows8_root);
			
			Version_Data data = {0};
			visit_files_w(windows8_lib, &data, win8_best);
			free(windows8_lib);
			
			if (data.best_name) {
				result->windows_sdk_version = 8;
				result->windows_sdk_root = data.best_name;
				RegCloseKey(main_key);
				return;
			}
		}
		
		// If we get here, we failed to find anything.
		RegCloseKey(main_key);
	}
	
	Find_Result find_visual_studio_and_windows_sdk() {
		Find_Result result;
		
		find_windows_kit_root(&result);
		
		if (result.windows_sdk_root) {
			result.windows_sdk_um_library_path   = concat2(result.windows_sdk_root, L"\\um\\x64");
			result.windows_sdk_ucrt_library_path = concat2(result.windows_sdk_root, L"\\ucrt\\x64");
		}
		
		return result;
	}
	
#endif  // MICROSOFT_CRAZINESS_HEADER_GUARD