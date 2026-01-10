#include "Windows_FileSystem.h"

#include <Windows.h>

ErrorMessage OpenFileAndMapping(const wchar_t *pwcFileName, void *&pFile, uint64_t &u64FileSize)
{
	//打开输入文件并映射
	HANDLE hReadFile = CreateFileW(pwcFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hReadFile == INVALID_HANDLE_VALUE)
	{
		return ErrorMessage(GetLastError());
	}

	//获得文件大小
	LARGE_INTEGER liFileSize = { 0 };
	if (!GetFileSizeEx(hReadFile, &liFileSize))
	{
		auto dwLastError = GetLastError();
		CloseHandle(hReadFile);//关闭输入文件
		return ErrorMessage(dwLastError);
	}
	u64FileSize = liFileSize.QuadPart;

	//判断文件为空
	if (liFileSize.QuadPart == 0)
	{
		auto dwLastError = GetLastError();
		CloseHandle(hReadFile);//关闭输入文件
		return ErrorMessage(dwLastError);
	}

	//创建文件映射对象
	HANDLE hFileMapping = CreateFileMappingW(hReadFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (!hFileMapping)
	{
		auto dwLastError = GetLastError();
		CloseHandle(hReadFile);//关闭输入文件
		return ErrorMessage(dwLastError);
	}
	CloseHandle(hReadFile);//关闭输入文件

	//映射文件到内存
	LPVOID lpReadMem = MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);
	if (!lpReadMem)
	{
		auto dwLastError = GetLastError();
		CloseHandle(hFileMapping);//关闭文件映射对象
		return ErrorMessage(dwLastError);
	}
	CloseHandle(hFileMapping);//关闭文件映射对象

	pFile = (void *)lpReadMem;
	return {};
}

ErrorMessage UnMappingAndCloseFile(uint8_t *pFileClose)
{
	if (!UnmapViewOfFile(pFileClose))
	{
		return ErrorMessage(GetLastError());
	}

	return {};
}


//TODO:实现窗口滚动映射


