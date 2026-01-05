#include <stdint.h>
#include <stddef.h>
#include <string>

#include "CPP_Helper.h"

using SOCKET_T = void *;
SOCKET_T GetUnInitSocket(void);

class ErrMessage
{
private:
	const char *pMsg;
	size_t szLength;

public:
	DELETE_COPY(ErrMessage);

	//因为实现需要win api，放在cpp内
	ErrMessage(uint32_t u32ErrCode) noexcept;
	~ErrMessage(void) noexcept;

	ErrMessage(ErrMessage &&_Move) noexcept :
		pMsg(_Move.pMsg),
		szLength(_Move.szLength)
	{
		_Move.pMsg = NULL;
		_Move.szLength = 0;
	}
	ErrMessage &operator=(ErrMessage &&_Move) noexcept
	{
		pMsg = _Move.pMsg;
		szLength = _Move.szLength;

		_Move.pMsg = NULL;
		_Move.szLength = 0;
	}

	std::string_view MsgStr(void) const noexcept
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

class SocketError
{
public:
	enum ErrorCode :uint8_t
	{
		NO_ERR = 0,		//无错误

		NOT_INITIALIZED,    //WSANOTINITIALISED Winsock未初始化
		NOT_SOCKET,			//WSAENOTSOCK 尝试对不是套接字的内容执行操作
		IN_PROGRESS,		//WSAEINPROGRESS 阻止操作当前正在执行

		INVALID_HANDLE,		//WSA_INVALID_HANDLE 无效的对象
		INVALID_PARAMETER,	//WSA_INVALID_PARAMETER 无效的参数

		NO_MEMORY,			//WSA_NOT_ENOUGH_MEMORY 内存不足
		NO_BUFFER,			//WSAENOBUFS 没有可用的缓冲区空间

		SOCKET_LIMIT,		//WSAEMFILE 打开的套接字过多
		MESSAGE_TOOLONG,	//WSAEMSGSIZE 消息过长

		ACCESS_DENIED,		//WSAEACCES 权限不足
		CALL_INTERRUPTED,	//WSAEINTR 函数调用被中断
		OP_NOSUPPORTED,		//WSAEOPNOTSUPP 对象的类型不支持尝试的操作

		ADDR_FAULT,			//WSAEFAULT 地址错误
		PARAM_FAULT,		//WSAEINVAL 参数无效

		ADDR_INUSE,			//WSAEADDRINUSE 地址已在使用中
		ADDR_NOTAVAIL,		//WSAEADDRNOTAVAIL 地址不可用

		IS_CONNECTED,		//WSAEISCONN 在已连接的套接字上发出连接请求
		NO_CONNECTED,		//WSAENOTCONN 在未连接的套接字上收发数据

		NET_DOWN,			//WSAENETDOWN 网络已关闭
		NET_UNREACH,		//WSAENETUNREACH 无法访问网络
		NET_RESET,			//WSAENETRESET 重置时网络连接断开

		CONNECT_SHUTDOWN,	//WSAESHUTDOWN 套接字已关闭
		CONNECT_ABORTED,	//WSAECONNABORTED 已建立的连接被主计算机中的软件中止
		CONNECT_RESET,		//WSAECONNRESET 远程主机强行关闭现有连接
		CONNECT_TIMEDOUT,	//WSAETIMEDOUT 连接超时
		CONNECT_REFUSED,	//WSAECONNREFUSED 连接被拒绝

		HOST_DOWN,			//WSAEHOSTDOWN 目标主机已关闭
		HOST_UNREACH,		//WSAEHOSTUNREACH 没有到主机的路由

		OTHER_ERR,			//其它错误
		ENUM_END,			//enum结束标记
	};

private:
	uint32_t u32WinErrorCode = 0;//默认值0，无错误

protected:
	static ErrorCode MapSocketError(uint32_t u32ErrCode);

public:
	DEFAULT_CSTC(SocketError);
	DEFAULT_CPMV(SocketError);
	DEFAULT_DSTC(SocketError);

	//阻止隐式转换构造
	explicit SocketError(uint32_t _u32WinErrorCode) :
		u32WinErrorCode(_u32WinErrorCode)
	{}

	GETTER_COPY(WinErrorCode, u32WinErrorCode);
	SETTER_CPMV(WinErrorCode, u32WinErrorCode);

	operator bool(void) const noexcept//返回是否有错误，有错误为true，否则false
	{
		return IsError();
	}

	bool IsError(void) const noexcept
	{
		return u32WinErrorCode != 0;//不为0则出错，返回true，否则false
	}

	ErrorCode ToErrorCode(void) const noexcept
	{
		return MapSocketError(u32WinErrorCode);
	}

	ErrMessage ToErrMessage(void) const noexcept
	{
		return ErrMessage(u32WinErrorCode);
	}
};


SocketError Startup(void);
SocketError Cleanup(void);

SocketError OpenSocket(SOCKET_T &socketOpen);
SocketError CloseSocket(SOCKET_T &socketClose);

SocketError ConnectSocket(SOCKET_T socketConnect, uint16_t u16ServerPort, uint32_t u32ServerAddr);
SocketError BindSocket(SOCKET_T socketBind, uint16_t u16ServerPort, uint32_t u32ServerAddr);
SocketError ListenSocket(SOCKET_T socketListen, uint32_t u32MaxPendingConnections);
SocketError AcceptSocket(SOCKET_T socketAccept, SOCKET_T &socketClient, uint16_t &u16ClientPort, uint32_t &u32ClientAddr);

enum class SocketShutdown : int { RECEIVE = 0, SEND = 1, BOTH = 2 };
SocketError ShutdownSocket(SOCKET_T socketShutdown, SocketShutdown enSocketShutdown);

SocketError SendDataPartial(SOCKET_T socketSend, const void *pDataBuffer, uint32_t &u32BufferSize);
SocketError SendDataAll(SOCKET_T socketSend, const void *pDataBuffer, uint32_t u32BufferSize, bool &bClientClosed);

SocketError RecvDataPartial(SOCKET_T socketRecv, void *pDataBuffer, uint32_t &u32BufferSize);
SocketError RecvDataAll(SOCKET_T socketRecv, void *pDataBuffer, uint32_t u32BufferSize, bool &bClientClosed);

