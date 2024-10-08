#include "FlacEncoder.h"

#include <stdexcept>
#include <algorithm>
#include <map>

FlacEncoder::FlacEncoder( const std::string& filename, const uint32_t sampleRate, const uint32_t bitsPerSample, const uint32_t channels, const uint32_t totalSamples, const uint32_t compressionLevel ) : 
  FLAC::Encoder::File(),
  m_Filename( filename )
{
	set_sample_rate( sampleRate );
	set_channels( channels );
	set_bits_per_sample( ( 8 == bitsPerSample || 16 == bitsPerSample ) ? bitsPerSample : ( 32 == bitsPerSample ? 24 : 16 ) );
	set_compression_level( std::clamp( compressionLevel, 0u, kMaximumCompressionLevel ) );
	set_verify( true );
	set_total_samples_estimate( totalSamples );

	constexpr uint32_t kSeekTableSecondsSpacing = 10;
	if ( ( totalSamples > 0 ) && ( sampleRate > 0 ) ) {
		const uint32_t sampleSpacing = static_cast<uint32_t>( sampleRate * kSeekTableSecondsSpacing );
		m_SeekTable = std::make_unique<FLAC::Metadata::SeekTable>();
		if ( m_SeekTable->template_append_spaced_points_by_samples( sampleSpacing, static_cast<FLAC__uint64>( totalSamples ) ) && m_SeekTable->template_sort( false ) ) {
			m_Metadata.push_back( m_SeekTable.get() );
		}
	}
}

FlacEncoder::~FlacEncoder()
{
  finish();
}

uint32_t FlacEncoder::Write( unsigned char* srcBuffer, const long byteCount )
{
  if ( m_FirstPass ) {
    m_FirstPass = false;

    if ( !m_Tags.empty() ) {
      m_VorbisComment = std::make_unique<FLAC::Metadata::VorbisComment>();
      for ( const auto& tag : m_Tags ) {
			  FLAC::Metadata::VorbisComment::Entry entry( tag.first.c_str(), tag.second.c_str() );
			  m_VorbisComment->append_comment( entry );
      }
      m_Metadata.push_back( m_VorbisComment.get() );
    }

		constexpr uint32_t kPaddingSize = 1024;
		m_Padding = std::make_unique<FLAC::Metadata::Padding>( kPaddingSize );
		m_Metadata.push_back( m_Padding.get() );

		set_metadata( m_Metadata.data(), static_cast<uint32_t>( m_Metadata.size() ) );

		if ( FLAC__STREAM_ENCODER_INIT_STATUS_OK != init( m_Filename ) )
      return 0;
  }

	const uint32_t sourceBPS = ( 24 == get_bits_per_sample() ) ? 32 : get_bits_per_sample();
  const uint32_t channels = get_channels();
  const uint32_t sampleCount = static_cast<uint32_t>( byteCount ) / channels / ( sourceBPS / 8 );
	const uint32_t bufferSize = sampleCount * channels;
	std::vector<FLAC__int32> flacBuffer( bufferSize );
  switch ( sourceBPS ) {
    case 8: {
      unsigned char* srcFirst = srcBuffer;
      unsigned char* srcLast = srcFirst + sampleCount * channels;
      std::transform( srcFirst, srcLast, flacBuffer.begin(), [] ( unsigned char value ) { return static_cast<char>( value + 128 ); } );
      break;
    }
    case 16: {
      short* srcFirst = reinterpret_cast<short*>( srcBuffer );
      short* srcLast = srcFirst + sampleCount * channels;
      std::transform( srcFirst, srcLast, flacBuffer.begin(), [] ( short value ) { return value; } );
      break;
    }
    case 32: {
      float* srcFirst = reinterpret_cast<float*>( srcBuffer );
      float* srcLast = srcFirst + sampleCount * channels;
      std::transform( srcFirst, srcLast, flacBuffer.begin(), [] ( float value ) { return static_cast<FLAC__int32>( std::clamp( value * 256, -8388608.0f, 8388607.0f ) ); } );
      break;
    }
  }
	if ( process_interleaved( flacBuffer.data(), sampleCount ) )
	  return static_cast<uint32_t>( byteCount );
  return 0;
}

void FlacEncoder::AddTag( const std::string& name, const std::string& value )
{
  m_Tags.insert( { name, value } );
}
