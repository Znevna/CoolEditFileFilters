#pragma once

#include "FLAC++\all.h"

#include <fstream>
#include <vector>
#include <string>
#include <optional>
#include <map>

class FlacDecoder : public FLAC::Decoder::Stream
{
public:
	// Throws std::runtime_error if the file could not be loaded.
	FlacDecoder( const std::wstring& filename );

	virtual ~FlacDecoder();

  uint32_t GetBitsPerSample() const { return m_BitsPerSample; }
  uint32_t GetChannels() const { return m_Channels; }
  uint32_t GetSampleRate() const { return m_SampleRate; }
  uint64_t GetTotalSamples() const { return m_TotalSamples; }
  const std::string& GetDescription() const { return m_Description; }
  static std::string GetVersion();
  std::optional<std::string> GetTagValue( const std::string& name ) const;

	uint32_t Read( unsigned char* buffer, const long byteCount );

protected:
	// FLAC callbacks
	FLAC__StreamDecoderReadStatus read_callback( FLAC__byte [], size_t * ) override;
	FLAC__StreamDecoderSeekStatus seek_callback( FLAC__uint64 ) override;
	FLAC__StreamDecoderTellStatus tell_callback( FLAC__uint64 * ) override;
	FLAC__StreamDecoderLengthStatus length_callback( FLAC__uint64 * ) override;
	bool eof_callback() override;
	FLAC__StreamDecoderWriteStatus write_callback( const FLAC__Frame *, const FLAC__int32 *const [] ) override;
	void metadata_callback( const FLAC__StreamMetadata * ) override;
	void error_callback( FLAC__StreamDecoderErrorStatus ) override;

private:
	std::ifstream m_FileStream;
  FLAC__Frame m_FLACFrame = {};
  std::vector<FLAC__int32> m_FLACBuffer;
	uint32_t m_FLACPos = 0;
	bool m_Valid = false;

  uint32_t m_BitsPerSample = 0;
  uint32_t m_Channels = 0;
  uint32_t m_SampleRate = 0;
  uint64_t m_TotalSamples = 0;
  uint32_t m_SourceChannels = 0;
  std::string m_Vendor;
  std::string m_Description;
  std::map<std::string /*name*/, std::string /*value*/> m_Tags;
};

