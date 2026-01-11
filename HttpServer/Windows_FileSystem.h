#pragma once

#include "CPP_Helper.h"
#include "Windows_SystemError.h"

#include <stdint.h>
#include <stddef.h>


ErrorMessage MappingFile(const char *pcFileName, void *&pFile, uint64_t &u64FileSize);
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

	bool Open(const char *pcFileName) noexcept;
	bool Close(void) noexcept;

	bool Read(void *pData, size_t szSize) noexcept;
	bool Write(const void *pData, size_t szSize) noexcept;

	bool GetPos(size_t &szPos) noexcept;
	bool SetPos(size_t szPos) noexcept;


};

