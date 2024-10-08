#include "FlacDecoder.h"

#include <algorithm>

std::string FlacDecoder::GetVersion()
{
  return "Free Lossless Audio Codec " + std::string( FLAC__VERSION_STRING );
}

FlacDecoder::FlacDecoder( const std::wstring& filename ) : FLAC::Decoder::Stream()
{
	m_FileStream.open( filename, std::ios::binary | std::ios::in );
	if ( m_FileStream.is_open() ) {
    set_metadata_respond_all();
		if ( init() == FLAC__STREAM_DECODER_INIT_STATUS_OK ) {
			process_until_end_of_metadata();
		}
	}

	if ( !m_Valid ) {
		finish();
		m_FileStream.close();
		throw std::runtime_error( "FlacDecoder failed to initialise" );
	}

  m_Description = m_Vendor.empty() ? std::string( "FLAC" ) : m_Vendor;
  if ( m_SourceChannels > 0 )
    m_Description += std::string( "\n" ) + std::to_string( m_SourceChannels ) + std::string( ( 1 == m_SourceChannels ) ? " channel" : " channels" );
}

FlacDecoder::~FlacDecoder()
{
	finish();
	m_FileStream.close();
}

uint32_t FlacDecoder::Read( unsigned char* destBuffer, const long byteCount )
{
  const uint32_t sampleCount = static_cast<uint32_t>( byteCount ) / m_Channels / ( m_BitsPerSample / 8 );
	uint32_t samplesRead = 0;
  while ( samplesRead < sampleCount ) {
    if ( m_FLACPos < m_FLACFrame.header.blocksize ) {
      const uint32_t samplesToRead = std::min( m_FLACFrame.header.blocksize - m_FLACPos, sampleCount - samplesRead );
      const auto srcFirst = m_FLACBuffer.begin() + m_FLACPos * m_Channels;
      const auto srcLast = srcFirst + samplesToRead * m_Channels;
      switch ( m_BitsPerSample ) {
        case 8: {
          unsigned char* dest = destBuffer + samplesRead * m_Channels;
          std::transform( srcFirst, srcLast, dest, [] ( FLAC__int32 value ) { return static_cast<unsigned char>( value + 128 ); } ); 
          break;
        }
        case 16: {
          short* dest = reinterpret_cast<short*>( destBuffer ) + samplesRead * m_Channels;
          std::transform( srcFirst, srcLast, dest, [] ( FLAC__int32 value ) { return static_cast<short>( value ); } ); 
          break;
        }
        case 32: {
          float* dest = reinterpret_cast<float*>( destBuffer ) + samplesRead * m_Channels;
          std::transform( srcFirst, srcLast, dest, [] ( FLAC__int32 value ) { return value / 65536.f; } ); 
          break;
        }
      }
      samplesRead += samplesToRead;
      m_FLACPos += samplesToRead;
    } else {
      m_FLACPos = 0;
      m_FLACFrame = {};
      if ( !process_single() || ( 0 == m_FLACFrame.header.blocksize ) ) {
        break;
      }
    }
	}
	return samplesRead * m_Channels * m_BitsPerSample / 8;
}

std::optional<std::string> FlacDecoder::GetTagValue( const std::string& name ) const
{
  std::string tagName( name );
  std::transform( tagName.begin(), tagName.end(), tagName.begin(), [] ( unsigned char c ) { return std::tolower(c); } );
  if ( const auto tag = m_Tags.find( tagName ); m_Tags.end() != tag )
    return tag->second;
  return std::nullopt;
}

FLAC__StreamDecoderReadStatus FlacDecoder::read_callback( FLAC__byte buf[], size_t * size )
{
	FLAC__StreamDecoderReadStatus status = FLAC__STREAM_DECODER_READ_STATUS_ABORT;
	if ( m_FileStream.eof() ) {
		*size = 0;
		status = FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
	} else {
		m_FileStream.read( reinterpret_cast<char*>( buf ), *size );
		*size = static_cast<size_t>( m_FileStream.gcount() );
		if ( *size > 0 ) {
			status = FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
		}
	}
	return status;
}

FLAC__StreamDecoderSeekStatus FlacDecoder::seek_callback( FLAC__uint64 pos )
{
	if ( !m_FileStream.good() ) {
		m_FileStream.clear();
	}
	m_FileStream.seekg( pos, std::ios_base::beg );
	return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

FLAC__StreamDecoderTellStatus FlacDecoder::tell_callback( FLAC__uint64 * pos )
{
	if ( !m_FileStream.good() ) {
		m_FileStream.clear();
	}
	*pos = m_FileStream.tellg();
	return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

FLAC__StreamDecoderLengthStatus FlacDecoder::length_callback( FLAC__uint64 * pos )
{
	if ( !m_FileStream.good() ) {
		m_FileStream.clear();
	}
	const std::streampos currentPos = m_FileStream.tellg();
	m_FileStream.seekg( 0, std::ios_base::end );
	*pos = m_FileStream.tellg();
	m_FileStream.seekg( currentPos );
	return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}

bool FlacDecoder::eof_callback()
{
	const bool eof = m_FileStream.eof();
	return eof;
}

FLAC__StreamDecoderWriteStatus FlacDecoder::write_callback( const FLAC__Frame * frame, const FLAC__int32 *const buffer[] )
{
  m_FLACFrame = {};
  uint32_t offset = 0;
  m_FLACFrame = *frame;
  m_FLACBuffer.resize( m_FLACFrame.header.blocksize * m_Channels );
  for ( uint32_t sample = 0; sample < m_FLACFrame.header.blocksize; sample++ ) {
    for ( uint32_t channel = 0; channel < m_Channels; channel++, offset++ ) {
      m_FLACBuffer[ offset ] = buffer[ channel ][ sample ] << ( m_BitsPerSample - m_FLACFrame.header.bits_per_sample );
    }
  }
  return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void FlacDecoder::metadata_callback( const FLAC__StreamMetadata * metadata )
{
	if ( metadata->type == FLAC__METADATA_TYPE_STREAMINFO ) {
    m_Valid = metadata->data.stream_info.bits_per_sample > 0 && metadata->data.stream_info.channels > 0 && metadata->data.stream_info.sample_rate > 0;
    if ( m_Valid ) {
      if ( metadata->data.stream_info.bits_per_sample <= 8 )
        m_BitsPerSample = 8;
      else if (metadata->data.stream_info.bits_per_sample <= 16)
        m_BitsPerSample = 16;
      else if (metadata->data.stream_info.bits_per_sample > 16)
        m_BitsPerSample = 32;
      m_SourceChannels = metadata->data.stream_info.channels;
		  m_Channels = std::min( 2u, m_SourceChannels );
      m_SampleRate = metadata->data.stream_info.sample_rate;
      m_TotalSamples = metadata->data.stream_info.total_samples;
    }
	} else if ( metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT ) {
    const auto& comment = metadata->data.vorbis_comment;
    if ( comment.vendor_string.length > 0 && nullptr != comment.vendor_string.entry ) {
      m_Vendor = std::string( comment.vendor_string.entry, comment.vendor_string.entry + comment.vendor_string.length );
    }
    for ( uint32_t i = 0; i < comment.num_comments; i++) {
      if ( nullptr != comment.comments[ i ].entry && comment.comments[ i ].length > 0 ) {
        const std::string entry( reinterpret_cast<char*>( comment.comments[ i ].entry ), comment.comments[ i ].length );
        if ( const auto pos = entry.find( '=' ); std::string::npos != pos ) {
          auto name = entry.substr( 0, pos );
          const auto value = entry.substr( 1 + pos );
          if ( !name.empty() && !value.empty() ) {
            std::transform( name.begin(), name.end(), name.begin(), [] ( unsigned char c ) { return std::tolower(c); } );
            m_Tags.insert( { name, value } );
          }
        }
      }
    }
  }
}

void FlacDecoder::error_callback( FLAC__StreamDecoderErrorStatus )
{
}
