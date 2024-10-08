#pragma once

#include "filters.h"

#ifdef __cplusplus
extern "C" {
#endif
int __stdcall QueryCoolFilter( COOLQUERY* cq );
BOOL __stdcall FilterUnderstandsFormat( LPSTR filename );
HANDLE __stdcall OpenFilterInput( LPSTR filename, LONG* sampleRate, WORD* bitsPerSample, WORD* channels, HWND hwnd, LONG* chunkSize );
DWORD __stdcall FilterGetFileSize( HANDLE input );
DWORD __stdcall ReadFilterInput( HANDLE input, BYTE* data, LONG bytes );
void __stdcall CloseFilterInput( HANDLE input );
DWORD __stdcall FilterOptionsString( HANDLE input, LPSTR str );
DWORD __stdcall FilterOptions( HANDLE input );
DWORD __stdcall FilterGetOptions( HWND hwnd, HINSTANCE inst, LONG sampleRate, WORD channels, WORD bitsPerSample, DWORD options );
DWORD __stdcall FilterSetOptions( HANDLE input, DWORD options, LONG sampleRate, WORD channels, WORD bitsPerSample );
#ifdef __cplusplus
}
#endif
