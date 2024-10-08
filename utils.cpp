#include "utils.h"
#include "json.hpp"

#include <vector>
#include <array>
#include <filesystem>
#include <fstream>

#include <Windows.h>
#include <ShlObj.h>

std::wstring AnsiCodePageToWideString( const std::string& text )
{
	std::wstring result;
	if ( !text.empty() ) {
		const int bufferSize = MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, text.c_str(), -1 /*strLen*/, nullptr /*buffer*/, 0 /*bufferSize*/ );
		if ( bufferSize > 0 ) {
			std::vector<WCHAR> buffer( bufferSize );
			if ( 0 != MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, text.c_str(), -1 /*strLen*/, buffer.data(), bufferSize ) ) {
				result = buffer.data();
			}
		}
	}
	return result;
}

std::string WideStringToAnsiCodePage( const std::wstring& text )
{
	std::string result;
	if ( !text.empty() ) {
		const int bufferSize = WideCharToMultiByte( CP_ACP, 0 /*flags*/, text.c_str(), -1 /*strLen*/, nullptr /*buffer*/, 0 /*bufferSize*/, NULL /*defaultChar*/, NULL /*usedDefaultChar*/ );
		if ( bufferSize > 0 ) {
			std::vector<char> buffer( bufferSize );
			if ( 0 != WideCharToMultiByte( CP_ACP, 0 /*flags*/, text.c_str(), -1 /*strLen*/, buffer.data(), bufferSize, NULL /*defaultChar*/, NULL /*usedDefaultChar*/ ) ) {
				result = buffer.data();
			}
		}
	}
	return result;
}

std::wstring UTF8ToWideString( const std::string& text )
{
	std::wstring result;
	if ( !text.empty() ) {
		const int bufferSize = MultiByteToWideChar( CP_UTF8, 0, text.c_str(), -1 /*strLen*/, nullptr /*buffer*/, 0 /*bufferSize*/ );
		if ( bufferSize > 0 ) {
			std::vector<WCHAR> buffer( bufferSize );
			if ( 0 != MultiByteToWideChar( CP_UTF8, 0, text.c_str(), -1 /*strLen*/, buffer.data(), bufferSize ) ) {
				result = buffer.data();
			}
		}
	}
	return result;
}

std::string WideStringToUTF8( const std::wstring& text )
{
	std::string result;
	if ( !text.empty() ) {
		const int bufferSize = WideCharToMultiByte( CP_UTF8, 0, text.c_str(), -1 /*strLen*/, nullptr /*buffer*/, 0 /*bufferSize*/, NULL /*defaultChar*/, NULL /*usedDefaultChar*/ );
		if ( bufferSize > 0 ) {
			std::vector<char> buffer( bufferSize );
			if ( 0 != WideCharToMultiByte( CP_UTF8, 0, text.c_str(), -1 /*strLen*/, buffer.data(), bufferSize, NULL /*defaultChar*/, NULL /*usedDefaultChar*/ ) ) {
				result = buffer.data();
			}
		}
	}
	return result;
}

std::string AnsiCodePageToUTF8( const std::string& text )
{
  return WideStringToUTF8( AnsiCodePageToWideString( text ) );
}

std::filesystem::path GetSettingsFilename()
{
  std::filesystem::path filepath;
  std::array<wchar_t, MAX_PATH> app = {};
  if ( SUCCEEDED( SHGetFolderPath( nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, app.data() ) ) ) {
    filepath = std::filesystem::path( app.data() ) / "Syntrillium\\Cool Edit";
    std::error_code ec;
    if ( !std::filesystem::is_directory( filepath, ec ) )
      filepath = std::filesystem::temp_directory_path( ec );
    filepath /= "CoolEditFltOptions.json";
  }
  return filepath;
}

std::optional<int32_t> ReadSetting( const std::string& name )
{
  try {
    std::ifstream filestream( GetSettingsFilename() );
    const auto j = nlohmann::json::parse( filestream );
    if ( const auto value = j.find( name ); j.end() != value && value->is_number_integer() )
      return *value;
  } catch ( const nlohmann::json::exception& ) { }
  return std::nullopt;
}

void WriteSetting( const std::string& name, const int32_t value )
{
  std::string options;
  try {
    std::ifstream filestream( GetSettingsFilename() );
    auto j = nlohmann::json::parse( filestream );
    j[ name ] = value;
    options = j.dump();
  } catch ( const nlohmann::json::exception& ) {
    try {
      auto j = nlohmann::json();
      j[ name ] = value;
      options = j.dump();
    } catch ( const nlohmann::json::exception& ) { }
  }
  if ( !options.empty() ) {
    std::ofstream filestream( GetSettingsFilename() );
    filestream << options;
  }
}