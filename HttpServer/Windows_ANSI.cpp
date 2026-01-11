#pragma once

#include "Windows_ANSI.h"

#include <Windows.h>

SystemError ConvertUtf16ToAnsi(const std::basic_string<wchar_t> &u16String, std::basic_string<char> &ansiString)
{
	if (u16String.empty())
	{
		ansiString.clear();//返回空字符串
		return {};//无错误
	}

	// 获取转换后所需的缓冲区大小（包括终止符）
	const int lengthNeeded = WideCharToMultiByte(//注意此函数u16接受字符数，而u8接受字节数
		CP_ACP,                // 使用当前ANSI代码页
		WC_NO_BEST_FIT_CHARS,  // 替换无法直接映射的字符
		(wchar_t*)u16String.data(),
		u16String.size(),//主动传入大小，则转换结果不包含\0
		NULL,
		0,
		NULL,
		NULL
	);

	if (lengthNeeded == 0)
	{
		//ERROR_INSUFFICIENT_BUFFER//。 提供的缓冲区大小不够大，或者错误地设置为 NULL。
		//ERROR_INVALID_FLAGS//。 为标志提供的值无效。
		//ERROR_INVALID_PARAMETER//。 任何参数值都无效。
		//ERROR_NO_UNICODE_TRANSLATION//。 在字符串中发现无效的 Unicode。
		
		return SystemError(GetLastError());
	}

	//创建string并预分配大小
	std::basic_string<char> ansiString;
	ansiString.resize(lengthNeeded);

	// 执行实际转换
	int convertedSize = WideCharToMultiByte(
		CP_ACP,
		WC_NO_BEST_FIT_CHARS,
		(wchar_t *)u16String.data(),
		u16String.size(),
		(char *)ansiString.data(),
		lengthNeeded,
		NULL,
		NULL
	);

	if (convertedSize == 0)
	{
		return SystemError(GetLastError());
	}

	return {};
}

SystemError ConvertUtf16ToUtf8(const std::basic_string<wchar_t> &u16String, std::basic_string<char8_t> &u8String)
{
	if (u16String.empty())
	{
		u8String.clear();//返回空字符串
		return {};//无错误
	}

	// 获取转换后所需的缓冲区大小（包括终止符）
	const int lengthNeeded = WideCharToMultiByte(//注意此函数u16接受字符数，而u8接受字节数
		CP_UTF8,                // 使用CP_UTF8代码页
		WC_NO_BEST_FIT_CHARS,  // 替换无法直接映射的字符
		(wchar_t *)u16String.data(),
		u16String.size(),//主动传入大小，则转换结果不包含\0
		NULL,
		0,
		NULL,
		NULL
	);

	if (lengthNeeded == 0)
	{
		//ERROR_INSUFFICIENT_BUFFER//。 提供的缓冲区大小不够大，或者错误地设置为 NULL。
		//ERROR_INVALID_FLAGS//。 为标志提供的值无效。
		//ERROR_INVALID_PARAMETER//。 任何参数值都无效。
		//ERROR_NO_UNICODE_TRANSLATION//。 在字符串中发现无效的 Unicode。

		return SystemError(GetLastError());
	}

	//创建string并预分配大小
	std::basic_string<char8_t> u8String;
	u8String.resize(lengthNeeded);

	// 执行实际转换
	int convertedSize = WideCharToMultiByte(
		CP_UTF8,
		WC_NO_BEST_FIT_CHARS,
		(wchar_t *)u16String.data(),
		u16String.size(),
		(char *)u8String.data(),
		lengthNeeded,
		NULL,
		NULL
	);

	if (convertedSize == 0)
	{
		return SystemError(GetLastError());
	}

	return {};
}

//两次转换，开销较大
SystemError ConvertUtf8ToAnsi(const std::basic_string<char8_t> &u8String, std::basic_string<char> &ansiString)
{
	if (u8String.empty())
	{
		ansiString.clear();//返回空字符串
		return {};//无错误
	}

	// UTF-8 -> UTF-16
	const int lengthNeeded = MultiByteToWideChar(
		CP_UTF8,
		0,
		(char *)u8String.data(),
		u8String.size(),
		NULL,
		0
	);

	if (lengthNeeded == 0)
	{
		return SystemError(GetLastError());
	}

	std::basic_string<wchar_t> utf16Str;
	utf16Str.resize(lengthNeeded);

	int convertedSize = MultiByteToWideChar(
		CP_UTF8,
		0,
		(char *)u8String.data(),
		u8String.size(),
		(wchar_t *)utf16Str.data(),
		lengthNeeded
	);

	if (convertedSize == 0)
	{
		return SystemError(GetLastError());
	}

	// UTF-16 -> ANSI
	return ConvertUtf16ToAnsi(utf16Str, ansiString);
}

//两次转换，开销较大
SystemError ConvertAnsiToUtf8(const std::basic_string<char> &ansiString, std::basic_string<char8_t> &u8String)
{
	if (ansiString.empty())
	{
		u8String.clear();//返回空字符串
		return {};//无错误
	}

	// ANSI -> UTF-16
	const int lengthNeeded = MultiByteToWideChar(
		CP_ACP,
		0,
		(char *)ansiString.data(),
		ansiString.size(),
		NULL,
		0
	);

	if (lengthNeeded == 0)
	{
		return SystemError(GetLastError());
	}

	std::basic_string<wchar_t> utf16Str;
	utf16Str.resize(lengthNeeded);

	int convertedSize = MultiByteToWideChar(
		CP_ACP,
		0,
		(char *)ansiString.data(),
		ansiString.size(),
		(wchar_t *)utf16Str.data(),
		lengthNeeded
	);

	if (convertedSize == 0)
	{
		return SystemError(GetLastError());
	}

	// UTF-16 -> UTF-8
	return ConvertUtf16ToUtf8(utf16Str, u8String);
}
