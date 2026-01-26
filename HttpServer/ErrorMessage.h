#pragma once

#include "CPP_Helper.h"

#include <stdint.h>
#include <stddef.h>
#include <string>

class ErrorMessage
{
private:
	const char *pMsg = NULL;
	size_t szLength = 0;

private:
	void Clear(void) noexcept;

public:
	DELETE_COPY(ErrorMessage);
	DEFAULT_CSTC(ErrorMessage);

	//因为实现需要win api，放在cpp内
	explicit ErrorMessage(uint32_t u32ErrCode) noexcept;
	~ErrorMessage(void) noexcept
	{
		Clear();
	}

	ErrorMessage(ErrorMessage &&_Move) noexcept :
		pMsg(_Move.pMsg),
		szLength(_Move.szLength)
	{
		_Move.pMsg = NULL;
		_Move.szLength = 0;
	}


	ErrorMessage &operator=(ErrorMessage &&_Move) noexcept
	{
		Clear();

		pMsg = _Move.pMsg;
		szLength = _Move.szLength;

		_Move.pMsg = NULL;
		_Move.szLength = 0;

		return *this;
	}

	std::string_view GetStrView(void) const noexcept
	{
		return pMsg != NULL
			? std::string_view(pMsg, szLength)	//指针非空返回实际说明	
			: std::string_view("", 0);			//指针为空返回空字符串
	}

	bool IsGetMessageError(void) const noexcept
	{
		return pMsg == NULL && szLength != 0;
	}

	uint32_t GetMessageErrorError(void) const noexcept
	{
		return (uint32_t)szLength;
	}
};



