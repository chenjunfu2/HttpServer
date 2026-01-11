#pragma once

#include "Windows_SystemError.h"

#include <string>

SystemError ConvertUtf16ToAnsi(const std::basic_string<wchar_t> &u16String, std::basic_string<char> &ansiString);
SystemError ConvertUtf16ToUtf8(const std::basic_string<wchar_t> &u16String, std::basic_string<char8_t> &u8String);
SystemError ConvertUtf8ToAnsi(const std::basic_string<char8_t> &u8String, std::basic_string<char> &ansiString);
SystemError ConvertAnsiToUtf8(const std::basic_string<char> &ansiString, std::basic_string<char8_t> &u8String);

