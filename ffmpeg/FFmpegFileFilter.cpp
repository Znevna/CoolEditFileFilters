#include "FFmpegFileFilter.h"
#include "FFmpegDecoder.h"
#include "utils.h"

constexpr long kChunkSize = 65536;

int __stdcall QueryCoolFilter( COOLQUERY* cq )
{
  _ASSERT_EXPR( 0, L"QueryCoolFilter" );

  // Use a few of the more commonly used extensions for the four available slots.
  strcpy_s( cq->szExt, 4, "M4A" );		
  strcpy_s( cq->szExt2, 4, "MP4" );		
  strcpy_s( cq->szExt3, 4, "MKV" );		
  strcpy_s( cq->szExt4, 4, "AVI" );		

	strcpy_s( cq->szName, 24, "FFmpeg" );		
	strcpy_s( cq->szCopyright, 80, FFmpegDecoder::GetVersion().c_str() );
	cq->lChunkSize = 0; 
	cq->dwFlags = QF_CANLOAD | QF_RATEADJUSTABLE | QF_CANDO32BITFLOATS;
 	cq->Stereo8 = 0xFF;
 	cq->Stereo16 = 0xFF;
 	cq->Stereo32 = 0xFF;
 	cq->Mono8 = 0xFF;
 	cq->Mono16 = 0xFF;
 	cq->Mono32 = 0xFF;
  return C_VALIDLIBRARY;
}

BOOL __stdcall FilterUnderstandsFormat( LPSTR filename )
{
  try {
    FFmpegDecoder decoder( AnsiCodePageToUTF8( filename ) );
    return TRUE;
  } catch ( const std::runtime_error& ) {
    return FALSE;
  }
}

HANDLE __stdcall OpenFilterInput( LPSTR filename, LONG* sampleRate, WORD* bitsPerSample, WORD* channels, HWND, LONG* chunkSize )
{
  try {
    auto decoder = new FFmpegDecoder( AnsiCodePageToUTF8( filename ) );
    if ( nullptr != sampleRate )
      *sampleRate = static_cast<LONG>( decoder->GetSampleRate() );
    if ( nullptr != bitsPerSample )
      *bitsPerSample = static_cast<WORD>( decoder->GetBitsPerSample() );
    if ( nullptr != channels )
      *channels = static_cast<WORD>( decoder->GetChannels() );
    if ( nullptr != chunkSize )
      *chunkSize = kChunkSize;
    return decoder;
  } catch ( const std::runtime_error& ) {
    return 0;
  }
}

DWORD __stdcall FilterGetFileSize( HANDLE hInput )
{
  FFmpegDecoder* decoder = static_cast<FFmpegDecoder*>( hInput );
  if ( nullptr == decoder )
    return 0;

  const uint64_t totalBytes = decoder->GetTotalSamples() * decoder->GetChannels() * decoder->GetBitsPerSample() / 8;
  return static_cast<DWORD>( std::min( totalBytes, static_cast<uint64_t>( std::numeric_limits<DWORD>::max() ) ) );
}

DWORD __stdcall ReadFilterInput( HANDLE hInput, BYTE* data, LONG bytes )
{
  FFmpegDecoder* decoder = static_cast<FFmpegDecoder*>( hInput );
  if ( nullptr == decoder )
    return 0;
  return decoder->Read( data, bytes );
}

void __stdcall CloseFilterInput( HANDLE hInput )
{
  FFmpegDecoder* decoder = static_cast<FFmpegDecoder*>( hInput );
  if ( nullptr != decoder )
    delete decoder;
}

DWORD __stdcall FilterOptionsString( HANDLE hInput, LPSTR str )
{
  constexpr uint32_t kMaxStringLength = 80;
  FFmpegDecoder* decoder = static_cast<FFmpegDecoder*>( hInput );
  if ( nullptr != decoder ) {
    // We don't know the length of the destination buffer, so put a sensible limit on our string length.
    const uint32_t count = std::min( decoder->GetDescription().size(), kMaxStringLength - 1 );
    memcpy( str, decoder->GetDescription().c_str(), count );
    str[ count ] = 0;
  }
  return 0;
}
