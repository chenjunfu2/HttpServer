#include "Windows_FileSystem.h"

#include <Windows.h>

FileSystem::FILE_T FileSystem::GetUnInitFile(void) noexcept
{
	return INVALID_HANDLE_VALUE;
}

ErrorMessage MappingFile(const wchar_t *pwcFileName, void *&pFile, uint64_t &u64FileSize)
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

	//通过参数返回
	u64FileSize = liFileSize.QuadPart;

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
		CloseHandle(hFileMapping);//关闭文件映射对象  注：此处不用关闭输入文件，前面已经关闭过了
		return ErrorMessage(dwLastError);
	}
	CloseHandle(hFileMapping);//关闭文件映射对象

	pFile = (void *)lpReadMem;

	//最后可以关闭所有文件对象（除了映射地址外）而不会出问题的原因是：
	//Win底层引用计数会根据多次打开累计。前面的关闭只是减少引用计数，
	//但是在减少之前已经通过其它API增加了一次引用，这就导致对象被延长生命周期，
	//而不是立刻销毁。这样，最后那个指针持有前面文件映射对象的引用，
	//而文件映射对象持有文件的引用。最后，回收的时候只需要关闭指针即可连续释放所有引用。
	return {};
}

ErrorMessage UnMapFile(void *&pFileClose)
{
	if (!UnmapViewOfFile(pFileClose))
	{
		return ErrorMessage(GetLastError());
	}

	pFileClose = NULL;
	return {};
}


//TODO:实现窗口滚动映射


