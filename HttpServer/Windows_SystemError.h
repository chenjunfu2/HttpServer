#pragma once

#include "CPP_Helper.h"
#include "Windows_ErrorMessage.h"

class SystemError
{
public:
	//预定义部分常用系统异常
	enum class ErrorCode : uint32_t
	{
		NO_ERR				= 0,//NO_ERROR
		OUTOFMEMORY			= 14,//ERROR_OUTOFMEMORY
		MORE_DATA			= 234,//ERROR_MORE_DATA
	};

protected:
	uint32_t u32ErrorCode = GetNoErrorCode();

public:
	static uint32_t GetNoErrorCode(void) noexcept;

	void Clear(void) noexcept
	{
		u32ErrorCode = GetNoErrorCode();
	}

	SystemError(ErrorCode _enErrorCode) :
		u32ErrorCode((uint32_t)_enErrorCode)
	{}

	DEFAULT_CSTC(SystemError);
	DEFAULT_CPMV(SystemError);
	DEFAULT_DSTC(SystemError);

	explicit SystemError(uint32_t _u32ErrorCode) :
		u32ErrorCode(_u32ErrorCode)
	{}

	GETTER_COPY(ErrorCode, u32ErrorCode);
	SETTER_COPY(ErrorCode, u32ErrorCode);

	void SetErrorCode(ErrorCode _enErrorCode) noexcept
	{
		u32ErrorCode = (uint32_t)_enErrorCode;
	}

	SystemError &operator=(uint32_t _u32SysErrorCode) noexcept
	{
		u32ErrorCode = _u32SysErrorCode;
		return *this;
	}

	SystemError &operator=(ErrorCode _enErrorCode) noexcept
	{
		u32ErrorCode = (uint32_t)_enErrorCode;
		return *this;
	}

	explicit operator bool(void) const noexcept//返回是否有错误，有错误为true，否则false
	{
		return IsError();
	}

	bool IsError(void) const noexcept
	{
		return u32ErrorCode != GetNoErrorCode();
	}

	bool IsNoError(void) const noexcept
	{
		return u32ErrorCode == GetNoErrorCode();
	}

	ErrorMessage GetErrorMessage(void) const noexcept
	{
		return ErrorMessage(u32ErrorCode);
	}
};

