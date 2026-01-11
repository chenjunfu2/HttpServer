#pragma once

#include "CPP_Helper.h"
#include "Windows_SystemError.h"

#include <stdint.h>
#include <stddef.h>


ErrorMessage MappingFile(const wchar_t *pwcFileName, void *&pFile, uint64_t &u64FileSize);
ErrorMessage UnMapFile(void *&pFileClose);


class FileError : public SystemError
{
public:
	using Base = SystemError;

	enum ErrorCode
	{

	};

protected:
	static ErrorCode MapFileError(uint32_t u32ErrorCode);

public:
	using Base::Base;
	using Base::operator=;

	ErrorCode GetFileErrorCode(void) const noexcept
	{
		return MapFileError(Base::u32ErrorCode);
	}

};



class FileSystem
{
public:
	using FILE_T = void *;

	enum class OpenMode : uint32_t
	{
		NONE	= 0x00000000,
		ALL		= 0x10000000,
		EXECUTE	= 0x20000000,
		WRITE	= 0x40000000,
		READ	= 0x80000000,
	};

	static OpenMode operator|(OpenMode l, OpenMode r)
	{
		return (OpenMode)((uint32_t)l | (uint32_t)r);
	}

	enum ShareMode : uint32_t
	{
		NONE	= 0x00000000,
		READ	= 0x00000001,
		WRITE	= 0x00000002,
		DELETE	= 0x00000004,
	};

	static ShareMode operator|(ShareMode l, ShareMode r)
	{
		return (ShareMode)((uint32_t)l | (uint32_t)r);
	}

	enum CreationMode : uint32_t
	{
		NONE				= 0,
		CREATE_NEW			= 1,
		CREATE_ALWAYS		= 2,
		OPEN_EXISTING		= 3,
		OPEN_ALWAYS			= 4,
		TRUNCATE_EXISTING	= 5,
	};

protected:
	static FILE_T GetUnInitFile(void) noexcept;


private:
	FILE_T fileData;
	FileError fileError;

public:
	void Clear(void) noexcept
	{
		if (!IsValid())
		{
			Close();
		}
		
		fileError.Clear();
	}


	DELETE_COPY(FileSystem);
	GETTER_COPY(FileError, fileError);
	GETTER_COPY(FileRaw, fileData);

	bool IsValid(void) noexcept;

	bool Open(const char *pcFileName, OpenMode enOpenMode, ShareMode enShareMode) noexcept;
	bool Close(void) noexcept;

	bool Read(void *pData, size_t szSize) noexcept;
	bool Write(const void *pData, size_t szSize) noexcept;

	bool GetPos(size_t &szPos) noexcept;
	bool SetPos(size_t szPos) noexcept;

	bool Map(const void *&pFileData) noexcept;
	bool UnMap(const void *&pFileData) noexcept;

};

