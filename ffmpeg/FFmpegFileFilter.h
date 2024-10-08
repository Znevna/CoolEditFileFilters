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
#ifdef __cplusplus
}
#endif
