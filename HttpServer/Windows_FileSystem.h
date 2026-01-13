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

class FileMapping;//前向声明

class File
{
	friend class FileMapping;
public:
	using FILE_T = void *;

	enum class AccessMode : uint32_t
	{
		NONE	= 0x00000000,
		ALL		= 0x10000000,
		EXECUTE	= 0x20000000,
		WRITE	= 0x40000000,
		READ	= 0x80000000,
	};

	static AccessMode operator|(AccessMode l, AccessMode r)
	{
		return (AccessMode)((uint32_t)l | (uint32_t)r);
	}

	enum class ShareMode : uint32_t
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

	enum class CreationMode : uint32_t
	{
		NONE				= 0,
		CREATE_NEW			= 1,
		CREATE_ALWAYS		= 2,
		OPEN_EXISTING		= 3,
		OPEN_ALWAYS			= 4,
		TRUNCATE_EXISTING	= 5,
	};

	enum class MoveMethod : uint32_t
	{
		BEG = 0,
		CUR = 1,
		END = 2,
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

	File(void) :
		fileData(GetUnInitFile()),
		fileError()
	{}

	~File(void)
	{
		Clear();
	}

	File(File &&_Move) noexcept :
		fileData(_Move.fileData),
		fileError(std::move(_Move.fileError))
	{
		_Move.fileData = GetUnInitFile();
	}

	File &operator=(File &&_Move) noexcept
	{
		Clear();

		fileData = _Move.fileData;
		fileError = std::move(_Move.fileError);

		_Move.fileData = GetUnInitFile();;

		return *this;
	}

	DELETE_COPY(File);
	GETTER_COPY(FileError, fileError);
	GETTER_COPY(FileRaw, fileData);

	bool IsValid(void) noexcept;

	bool Open(const wchar_t *pwcFileName, AccessMode enAccessMode, ShareMode enShareMode, CreationMode enCreationMode) noexcept;
	bool Close(void) noexcept;

	bool Read(void *pData, uint32_t &u32Size) noexcept;
	bool Write(const void *pData, uint32_t &u32Size) noexcept;

	bool GetPos(int64_t &i64Pos) noexcept;
	bool SetPos(int64_t i64Pos, MoveMethod enMoveMethod) noexcept;

};

class FileMapping
{
public:
	class MemoryFile//持有映射指针，析构释放
	{




	};


public:
	//通过File对象创建








};



