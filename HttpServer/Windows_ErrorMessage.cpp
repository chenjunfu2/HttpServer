#include "Windows_ErrorMessage.h"

#include <Windows.h>

ErrorMessage::ErrorMessage(uint32_t u32ErrCode) noexcept
{
	void *pRet = NULL;
	DWORD dRet = FormatMessageA
	(
		FORMAT_MESSAGE_FROM_SYSTEM |		//从系统错误描述表中查询
		FORMAT_MESSAGE_IGNORE_INSERTS |		//忽略参数
		FORMAT_MESSAGE_ALLOCATE_BUFFER,		//自行分配字符串内存（需要后续手动释放）
		NULL,								//忽略
		u32ErrCode,							//需要查询的错误码
		LANG_USER_DEFAULT,					//用户当前语言
		(LPSTR)&pRet,						//系统分配字符串
		0,									//预分配长度（从0起）
		NULL								//忽略
	);

	//如果函数失败，则返回值为零，设置大小为错误码，并设置消息为NULL以便识别特殊问题
	if (dRet != 0)
	{
		pMsg = (const char *)pRet;
		szLength = (size_t)dRet;
	}
	else
	{
		pMsg = NULL;
		szLength = (size_t)GetLastError();
	}
}

ErrorMessage::~ErrorMessage(void) noexcept
{
	Clear();
}

void ErrorMessage::Clear(void) noexcept
{
	if (pMsg != NULL)
	{
		LocalFree((HLOCAL)pMsg);
	}

	pMsg = NULL;
	szLength = 0;
}

