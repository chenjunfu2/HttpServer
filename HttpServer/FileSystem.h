#pragma once

#include "CPP_Helper.h"
#include "SystemError.h"

#include <stdint.h>
#include <stddef.h>

class FileError : public SystemError
{
public:
	using Base = SystemError;

	enum class ErrorCode : uint32_t
	{
		NO_ERR = 0,			//NO_ERROR 无错误
		NO_FOUND,			//ERROR_FILE_NOT_FOUND
		ACCESS_DENIED,		//ERROR_ACCESS_DENIED
		ALREADY_EXISTS,		//ERROR_ALREADY_EXISTS or ERROR_FILE_EXISTS

		OTHER_ERR,			//其它错误
		ENUM_END,			//enum结束标记
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

class File
{
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

	AccessMode friend operator|(AccessMode l, AccessMode r)
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

	ShareMode friend operator|(ShareMode l, ShareMode r)
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


	class MemoryView//持有映射指针，析构释放
	{
		friend class File;
	private:
		const void *pViewData;
		int64_t i64ViewSize;

	protected:
		SETTER_COPY(ViewData, pViewData);
		SETTER_COPY(ViewSize, i64ViewSize);

	public:
		void Clear(void) noexcept
		{
			if (IsValid())
			{
				Close();
			}

			pViewData = NULL;
			i64ViewSize = 0;
		}

		MemoryView(void) :
			pViewData(NULL),
			i64ViewSize(0)
		{}

		~MemoryView(void)
		{
			Clear();
		}

		DELETE_COPY(MemoryView);
		
		MemoryView(MemoryView &&_Move):
			pViewData(_Move.pViewData),
			i64ViewSize(_Move.i64ViewSize)
		{
			_Move.pViewData = NULL;
			_Move.i64ViewSize = 0;
		}

		MemoryView &operator=(MemoryView &&_Move)
		{
			Clear();

			pViewData = _Move.pViewData;
			i64ViewSize = _Move.i64ViewSize;

			_Move.pViewData = NULL;
			_Move.i64ViewSize = 0;

			return *this;
		}

		GETTER_COPY(ViewData, pViewData);
		GETTER_COPY(ViewSize, i64ViewSize);

		bool IsValid(void) const noexcept
		{
			return pViewData != NULL;
		}

		SystemError Close(void) noexcept;
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

	bool GetSize(int64_t &i64Size) noexcept;

	bool MappingToMemoryView(MemoryView &fileMemoryView) noexcept;

};


