#include "OpenMPTDecoder.h"
#include "utils.h"

#include <algorithm>
#include <array>

std::string OpenMPTDecoder::GetVersion()
{
  const uint32_t version = openmpt::get_library_version();
  const uint32_t major = version >> 24;
  const uint32_t minor = ( version >> 16 ) & 0xff;
  const uint32_t patch = version & 0xff;
	const std::string result = "libopenmpt " + std::to_string( major ) + "." + std::to_string( minor ) + "." + std::to_string( patch );
	return result;
}

OpenMPTDecoder::OpenMPTDecoder( const std::wstring& filename, const Options& options ) :
  m_stream( filename, std::ios::binary ),
  m_module( m_stream ),
  m_SampleRate( options.SampleRate ),
  m_Channels( 2 ),
  m_BitsPerSample( 32 )
{
  m_module.set_render_param( openmpt::module::RENDER_STEREOSEPARATION_PERCENT, options.StereoSeparation );
  m_module.set_render_param( openmpt::module::RENDER_INTERPOLATIONFILTER_LENGTH, options.Interpolation );
  m_module.set_render_param( openmpt::module::RENDER_VOLUMERAMPING_STRENGTH, options.VolumeRamping );
  m_module.set_repeat_count( options.RepeatCount );

  // Multiply by the repeat count to give an upper bound on the number of samples - when reading we will stop at the actual end position.
  m_TotalSamples = static_cast<uint64_t>( std::llround( m_module.get_duration_seconds() * m_SampleRate * ( 1 + options.RepeatCount ) ) );
  if ( m_TotalSamples > 0 ) {
    std::string type = m_module.get_metadata( "type_long" );
    m_Description = type.empty() ? std::string( "libopenmpt" ) : type;
    if ( const int32_t channels = m_module.get_num_channels() )
      m_Description += std::string( "\n" ) + std::to_string( channels ) + std::string( 1 == channels ? " channel" : " channels" );
  } else {
		throw std::runtime_error( "OpenMPTDecoder failed to initialise" );
  }
}

OpenMPTDecoder::~OpenMPTDecoder()
{
}

uint32_t OpenMPTDecoder::Read( unsigned char* destBuffer, const long byteCount )
{
  float* buffer = reinterpret_cast<float*>( destBuffer );
  const uint32_t sampleCount = byteCount / m_Channels / ( m_BitsPerSample / 8 );
  const uint32_t samplesRead = m_module.read_interleaved_stereo( static_cast<int32_t>( m_SampleRate ), sampleCount, buffer );
  // Floating point audio needs to be rescaled for Cool Edit.
  std::for_each( buffer, buffer + samplesRead * m_Channels, [] ( float& f ) { f *= 32768.f; } );
	return samplesRead * m_Channels * m_BitsPerSample / 8;
}
