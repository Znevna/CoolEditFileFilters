#pragma once

#include <string>
#include <optional>

std::wstring AnsiCodePageToWideString( const std::string& text );
std::string WideStringToAnsiCodePage( const std::wstring& text );
std::wstring UTF8ToWideString( const std::string& text );
std::string WideStringToUTF8( const std::wstring& text );
std::string AnsiCodePageToUTF8( const std::string& text );

std::optional<int32_t> ReadSetting( const std::string& name );
void WriteSetting( const std::string& name, const int32_t value );