#include "FileSystem.h"

#include <Windows.h>

/*
SystemError MappingFile(const wchar_t *pwcFileName, void *&pFile, uint64_t &u64FileSize)
{
	//打开输入文件并映射
	HANDLE hReadFile = CreateFileW(pwcFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hReadFile == INVALID_HANDLE_VALUE)
	{
		return SystemError(GetLastError());
	}

	//获得文件大小
	LARGE_INTEGER liFileSize = { 0 };
	if (!GetFileSizeEx(hReadFile, &liFileSize))
	{
		auto dwLastError = GetLastError();
		CloseHandle(hReadFile);//关闭输入文件
		return SystemError(dwLastError);
	}

	//通过参数返回
	u64FileSize = liFileSize.QuadPart;

	//创建文件映射对象
	HANDLE hFileMapping = CreateFileMappingW(hReadFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (!hFileMapping)
	{
		auto dwLastError = GetLastError();
		CloseHandle(hReadFile);//关闭输入文件
		return SystemError(dwLastError);
	}
	CloseHandle(hReadFile);//关闭输入文件

	//映射文件到内存
	LPVOID lpReadMem = MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);
	if (!lpReadMem)
	{
		auto dwLastError = GetLastError();
		CloseHandle(hFileMapping);//关闭文件映射对象  注：此处不用关闭输入文件，前面已经关闭过了
		return SystemError(dwLastError);
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

SystemError UnMapFile(void *&pFileClose)
{
	if (!UnmapViewOfFile(pFileClose))
	{
		return SystemError(GetLastError());
	}

	pFileClose = NULL;
	return {};
}
*/

FileError::ErrorCode FileError::MapFileError(uint32_t u32ErrorCode)
{
	switch (u32ErrorCode)
	{
	case ERROR_FILE_NOT_FOUND:	return ErrorCode::NO_FOUND;
	case ERROR_ACCESS_DENIED:	return ErrorCode::ACCESS_DENIED;
	case ERROR_ALREADY_EXISTS:	//与下个相同
	case ERROR_FILE_EXISTS:		return ErrorCode::ALREADY_EXISTS;

	default:					return ErrorCode::OTHER_ERR;
	}
}


SystemError File::MemoryView::Close(void) noexcept
{
	if (UnmapViewOfFile(pViewData) == FALSE)
	{
		return SystemError(GetLastError());
	}

	return {};
}

File::FILE_T File::GetUnInitFile(void) noexcept
{
	return INVALID_HANDLE_VALUE;
}

bool File::IsValid(void) noexcept
{
	return fileData != GetUnInitFile();
}

bool File::Open(const wchar_t *pwcFileName, AccessMode enAccessMode, ShareMode enShareMode, CreationMode enCreationMode) noexcept
{
	if (IsValid())
	{
		fileError = ERROR_ALREADY_EXISTS;//已存在
		return false;
	}

	HANDLE hFile = CreateFileW(pwcFileName, (uint32_t)enAccessMode, (uint32_t)enShareMode, NULL, (uint32_t)enCreationMode, FILE_ATTRIBUTE_NORMAL, NULL);
	fileError = GetLastError();//注意哪怕没出错也要保留错误码，因为文件打开模式会可能会成功并设置错误码。
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	fileData = (FILE_T)hFile;

	return true;
}

bool File::Close(void) noexcept
{
	if (CloseHandle((HANDLE)fileData) == 0)
	{
		fileError = GetLastError();
		return false;
	}

	return true;
}

bool File::Read(void *pData, uint32_t &u32Size) noexcept
{
	if (ReadFile((HANDLE)fileData, pData, u32Size, (LPDWORD)&u32Size, NULL) == FALSE)
	{
		fileError = GetLastError();
		return false;
	}

	return true;
}

bool File::Write(const void *pData, uint32_t &u32Size) noexcept
{
	if (WriteFile((HANDLE)fileData, pData, u32Size, (LPDWORD)&u32Size, NULL) == FALSE)
	{
		fileError = GetLastError();
		return false;
	}

	return true;
}

bool File::GetPos(int64_t &i64Pos) noexcept
{
	LARGE_INTEGER liCurrentPos{};

	if (SetFilePointerEx((HANDLE)fileData, LARGE_INTEGER{ .QuadPart = 0 }, &liCurrentPos, (uint32_t)MoveMethod::CUR) == FALSE)
	{
		fileError = GetLastError();
		return false;
	}

	i64Pos = liCurrentPos.QuadPart;

	return true;
}

bool File::SetPos(int64_t i64Pos, MoveMethod enMoveMethod) noexcept
{
	if (SetFilePointerEx((HANDLE)fileData, LARGE_INTEGER{ .QuadPart = i64Pos }, NULL, (uint32_t)enMoveMethod) == FALSE)
	{
		fileError = GetLastError();
		return false;
	}

	return true;
}

bool File::GetSize(int64_t &i64Size) noexcept
{
	LARGE_INTEGER liFileSize{};

	if (GetFileSizeEx((HANDLE)fileData, &liFileSize) == FALSE)
	{
		fileError = GetLastError();
		return false;
	}

	i64Size = liFileSize.QuadPart;

	return true;
}


bool File::MappingToMemoryView(MemoryView &fileMemoryView) noexcept
{
	int64_t i64FileSize{};
	if (!GetSize(i64FileSize))//内部设置错误码，无需重复设置
	{
		return false;
	}

	//创建文件映射对象
	HANDLE hFileMapping = CreateFileMappingW((HANDLE)fileData, NULL, PAGE_READONLY, 0, 0, NULL);
	if (hFileMapping == NULL)
	{
		fileError = GetLastError();
		return false;
	}

	//映射文件到内存
	LPVOID lpViewMemory = MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);//具体关闭实现由MemoryView类完成
	if (lpViewMemory == NULL)
	{
		fileError = GetLastError();
		CloseHandle(hFileMapping);//关闭映射对象
		return false;
	}
	CloseHandle(hFileMapping);//关闭映射对象

	//仅保留指针
	fileMemoryView.SetViewData(lpViewMemory);
	fileMemoryView.SetViewSize(i64FileSize);

	return true;
}




