#include "EncoderOpus.h"

#include <stdexcept>
#include <algorithm>
#include <map>

std::string EncoderOpus::GetVersion()
{
  return opus_get_version_string();
}

EncoderOpus::EncoderOpus( const std::string& filename, const uint32_t sampleRate, const uint32_t bitsPerSample, const uint32_t channels, const uint32_t totalSamples, const uint32_t bitrate ) :
  m_Filename( filename ),
  m_SampleRate( sampleRate ),
  m_BitsPerSample( bitsPerSample ),
  m_Channels( channels ),
  m_Bitrate( bitrate )
{
}

EncoderOpus::~EncoderOpus()
{
  if ( nullptr != m_OpusEncoder ) {
		ope_encoder_drain( m_OpusEncoder );
		ope_encoder_destroy( m_OpusEncoder );
  }
}

uint32_t EncoderOpus::Write( unsigned char* srcBuffer, const long byteCount )
{
  if ( m_FirstPass ) {
    m_FirstPass = false;

	  OggOpusComments* opusComments = ope_comments_create();
	  if ( nullptr != opusComments ) {
      for ( const auto& [ name, value ] : m_Tags )
        ope_comments_add( opusComments, name.c_str(), value.c_str() );
		  m_OpusEncoder = ope_encoder_create_file( m_Filename.c_str(), opusComments, m_SampleRate, m_Channels, 0, nullptr );
		  if ( nullptr != m_OpusEncoder ) {
			  const int bitrate = 1000 * m_Bitrate;
			  ope_encoder_ctl( m_OpusEncoder, OPUS_SET_BITRATE( bitrate ) );
		  }
		  ope_comments_destroy( opusComments );
	  }

    if ( nullptr == m_OpusEncoder )
      return 0;
  }

  const int sampleCount = byteCount / m_Channels / ( m_BitsPerSample / 8 );
  switch ( m_BitsPerSample ) {
    case 16:
      return ( OPE_OK == ope_encoder_write( m_OpusEncoder, reinterpret_cast<const short*>( srcBuffer ), sampleCount ) ) ? byteCount : 0;
    case 32: {
      float* buffer = reinterpret_cast<float*>( srcBuffer );
      for ( int i = 0; i < sampleCount * m_Channels; i++ )
        buffer[ i ] /= 32768.f;
      return ( OPE_OK == ope_encoder_write_float( m_OpusEncoder, buffer, sampleCount ) ) ? byteCount : 0;
    }
    default:
      return 0;
  }
}

void EncoderOpus::AddTag( const std::string& name, const std::string& value )
{
  m_Tags.insert( { name, value } );
}
