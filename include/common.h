#ifndef common_h__
#define common_h__

#include <Windows.h>
#include <process.h>

#ifdef _WIN32
#define DECL_EXPORT	__declspec(dllexport)
#define DECL_IMPORT	__declspec(dllimport)
// We want to avoid name mangling for exported functions that
// For example - need to be dynamically loaded (GetProcAddress())
#define DECL_DLL extern "C" DECL_EXPORT
#endif //_WIN32

#endif // common_h__