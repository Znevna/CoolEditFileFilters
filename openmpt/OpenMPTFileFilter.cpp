#include "OpenMPTFileFilter.h"
#include "OpenMPTDecoder.h"
#include "utils.h"
#include "resource.h"
#include "CommCtrl.h"
#include "windowsx.h"

constexpr long kChunkSize = 65536;

int __stdcall QueryCoolFilter( COOLQUERY* cq )
{
  _ASSERT_EXPR( 0, L"QueryCoolFilter" );

  // Use a few of the more commonly used extensions for the four available slots.
  strcpy_s( cq->szExt, 4, "MOD" );		
  strcpy_s( cq->szExt2, 4, "XM" );		
  strcpy_s( cq->szExt3, 4, "S3M" );		
  strcpy_s( cq->szExt4, 4, "IT" );		

	strcpy_s( cq->szName, 24, "libopenmpt" );		
	strcpy_s( cq->szCopyright, 80, OpenMPTDecoder::GetVersion().c_str() );
	cq->lChunkSize = 0; 
	cq->dwFlags = QF_CANLOAD | QF_RATEADJUSTABLE | QF_CANDO32BITFLOATS | QF_HASOPTIONSBOX;
 	cq->Stereo32 = 0xFF;
 	cq->Mono32 = 0xFF;
  return C_VALIDLIBRARY;
}

BOOL __stdcall FilterUnderstandsFormat( LPSTR filename )
{
  try {
    OpenMPTDecoder decoder( AnsiCodePageToWideString( filename ) );
    return TRUE;
  } catch ( const std::exception& ) {
    return FALSE;
  }
}

HANDLE __stdcall OpenFilterInput( LPSTR filename, LONG* sampleRate, WORD* bitsPerSample, WORD* channels, HWND, LONG* chunkSize )
{
  try {
    auto decoder = new OpenMPTDecoder( AnsiCodePageToWideString( filename ) );
    if ( nullptr != sampleRate )
      *sampleRate = static_cast<LONG>( decoder->GetSampleRate() );
    if ( nullptr != bitsPerSample )
      *bitsPerSample = static_cast<WORD>( decoder->GetBitsPerSample() );
    if ( nullptr != channels )
      *channels = static_cast<WORD>( decoder->GetChannels() );
    if ( nullptr != chunkSize )
      *chunkSize = kChunkSize;
    return decoder;
  } catch ( const std::exception& ) {
    return 0;
  }
}

DWORD __stdcall FilterGetFileSize( HANDLE hInput )
{
  OpenMPTDecoder* decoder = static_cast<OpenMPTDecoder*>( hInput );
  if ( nullptr == decoder )
    return 0;

  const uint64_t totalBytes = decoder->GetTotalSamples() * decoder->GetChannels() * decoder->GetBitsPerSample() / 8;
  return static_cast<DWORD>( std::min( totalBytes, static_cast<uint64_t>( std::numeric_limits<DWORD>::max() ) ) );
}

DWORD __stdcall ReadFilterInput( HANDLE hInput, BYTE* data, LONG bytes )
{
  OpenMPTDecoder* decoder = static_cast<OpenMPTDecoder*>( hInput );
  if ( nullptr == decoder )
    return 0;
  return decoder->Read( data, bytes );
}

void __stdcall CloseFilterInput( HANDLE hInput )
{
  OpenMPTDecoder* decoder = static_cast<OpenMPTDecoder*>( hInput );
  if ( nullptr != decoder )
    delete decoder;
}

DWORD __stdcall FilterOptionsString( HANDLE hInput, LPSTR str )
{
  constexpr uint32_t kMaxStringLength = 80;
  OpenMPTDecoder* decoder = static_cast<OpenMPTDecoder*>( hInput );
  if ( nullptr != decoder ) {
    // We don't know the length of the destination buffer, so put a sensible limit on our string length.
    const uint32_t count = std::min( decoder->GetDescription().size(), kMaxStringLength - 1 );
    memcpy( str, decoder->GetDescription().c_str(), count );
    str[ count ] = 0;
  }
  return 0;
}

INT_PTR CALLBACK DialogProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch ( message ) {
		case WM_INITDIALOG : {
      // Take note of the previous sample rate option.
      SetWindowLongPtr( hwnd, GWLP_USERDATA, lParam );

      OpenMPTDecoder::Options options;
      SendDlgItemMessage( hwnd, IDC_SEPARATIONSLIDER, TBM_SETRANGEMIN, 0, 0 );
      SendDlgItemMessage( hwnd, IDC_SEPARATIONSLIDER, TBM_SETRANGEMAX, 0, OpenMPTDecoder::Options::kMaximumStereoSeparation );
      SendDlgItemMessage( hwnd, IDC_SEPARATIONSLIDER, TBM_SETTICFREQ, 10, 0 );
      SendDlgItemMessage( hwnd, IDC_SEPARATIONSLIDER, TBM_SETPAGESIZE, 0, 10 );
      SendDlgItemMessage( hwnd, IDC_SEPARATIONSLIDER, TBM_SETPOS, 1, options.StereoSeparation );

      const auto interpolationOptions = OpenMPTDecoder::Options::GetInterpolationOptions();
      for ( const auto& [ value, description ] : interpolationOptions ) {
        ComboBox_AddString( GetDlgItem( hwnd, IDC_INTERPOLATION ), description.c_str() );
        ComboBox_SetItemData( GetDlgItem( hwnd, IDC_INTERPOLATION ), ComboBox_GetCount( GetDlgItem( hwnd, IDC_INTERPOLATION ) ) - 1, value );
        if ( value == options.Interpolation )
          ComboBox_SetCurSel( GetDlgItem( hwnd, IDC_INTERPOLATION ), ComboBox_GetCount( GetDlgItem( hwnd, IDC_INTERPOLATION ) ) - 1 );
      }

      const auto rampingOptions = OpenMPTDecoder::Options::GetVolumeRampingOptions();
      for ( const auto& [ value, description ] : rampingOptions ) {
        ComboBox_AddString( GetDlgItem( hwnd, IDC_RAMPING ), description.c_str() );
        ComboBox_SetItemData( GetDlgItem( hwnd, IDC_RAMPING ), ComboBox_GetCount( GetDlgItem( hwnd, IDC_RAMPING ) ) - 1, value );
        if ( value == options.VolumeRamping )
          ComboBox_SetCurSel( GetDlgItem( hwnd, IDC_RAMPING ), ComboBox_GetCount( GetDlgItem( hwnd, IDC_RAMPING ) ) - 1 );
      }

      const auto samplerateOptions = OpenMPTDecoder::Options::GetSampleRateOptions();
      for ( const auto& [ value, description ] : samplerateOptions ) {
        ComboBox_AddString( GetDlgItem( hwnd, IDC_SAMPLERATE ), description.c_str() );
        ComboBox_SetItemData( GetDlgItem( hwnd, IDC_SAMPLERATE ), ComboBox_GetCount( GetDlgItem( hwnd, IDC_SAMPLERATE ) ) - 1, value );
        if ( value == options.SampleRate )
          ComboBox_SetCurSel( GetDlgItem( hwnd, IDC_SAMPLERATE ), ComboBox_GetCount( GetDlgItem( hwnd, IDC_SAMPLERATE ) ) - 1 );
      }

      Button_SetCheck( GetDlgItem( hwnd, IDC_REPEAT ), options.RepeatCount );
      return TRUE;
		}
    case WM_COMMAND : {
			switch ( LOWORD( wParam ) ) {
        case IDC_SAMPLERATE: {
          if ( CBN_SELCHANGE == HIWORD( wParam ) ) {
            const int32_t previousSampleRate = static_cast<int32_t>( GetWindowLongPtr( hwnd, GWLP_USERDATA ) );
            const int32_t selectedSampleRate = ComboBox_GetItemData( GetDlgItem( hwnd, IDC_SAMPLERATE ), ComboBox_GetCurSel( GetDlgItem( hwnd, IDC_SAMPLERATE ) ) );
            SetDlgItemTextA( hwnd, IDC_LABEL_SAMPLERATE, ( previousSampleRate == selectedSampleRate ? "Sample Rate" : "*Sample Rate" ) );
            ShowWindow( GetDlgItem( hwnd, IDC_WARNING_SAMPLERATE ), ( previousSampleRate == selectedSampleRate ? SW_HIDE : SW_SHOW ) ); 
          }
          break;
        }
				case IDOK : {
          OpenMPTDecoder::Options options(
            ComboBox_GetItemData( GetDlgItem( hwnd, IDC_SAMPLERATE ), ComboBox_GetCurSel( GetDlgItem( hwnd, IDC_SAMPLERATE ) ) ),
            SendDlgItemMessage( hwnd, IDC_SEPARATIONSLIDER, TBM_GETPOS, 0, 0 ),
            ComboBox_GetItemData( GetDlgItem( hwnd, IDC_INTERPOLATION ), ComboBox_GetCurSel( GetDlgItem( hwnd, IDC_INTERPOLATION ) ) ),
            ComboBox_GetItemData( GetDlgItem( hwnd, IDC_RAMPING ), ComboBox_GetCurSel( GetDlgItem( hwnd, IDC_RAMPING ) ) ),
            Button_GetCheck( GetDlgItem( hwnd, IDC_REPEAT ) ) );

          // Cancel input if the sample rate has changed, so that the file can be re-opened with Cool Edit being aware of the correct setting.
          const int32_t previousSampleRate = static_cast<int32_t>( GetWindowLongPtr( hwnd, GWLP_USERDATA ) );
				  EndDialog( hwnd, previousSampleRate == options.SampleRate );
          return TRUE;
        }
				case IDCANCEL :
					EndDialog( hwnd, 0 );
					return TRUE;
			}
			break;
		}
    case WM_NOTIFY: {
      LPNMHDR nmheader = reinterpret_cast<LPNMHDR>( lParam );
      if ( nullptr != nmheader && IDC_SEPARATIONSLIDER == nmheader->idFrom ) {
        const std::string separation = std::to_string( SendDlgItemMessage( hwnd, IDC_SEPARATIONSLIDER, TBM_GETPOS, 0, 0 ) );
        SetDlgItemTextA( hwnd, IDC_SEPARATIONVALUE, separation.c_str() );
        return TRUE;
      }
      break;
    }
	}
	return FALSE;
}

DWORD __stdcall FilterOptions( HANDLE hInput )
{
	return 0;
}

DWORD __stdcall FilterGetOptions( HWND hwnd, HINSTANCE inst, LONG sampleRate, WORD channels, WORD bitsPerSample, DWORD options )
{
	return DialogBoxParam( inst, MAKEINTRESOURCE(IDD_CONFIG), hwnd, DialogProc, OpenMPTDecoder::Options().SampleRate );
}

DWORD __stdcall FilterSetOptions( HANDLE hInput, DWORD options, LONG sampleRate, WORD channels, WORD bitsPerSample )
{
	return 0;
}
