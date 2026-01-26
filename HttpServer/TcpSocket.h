#pragma once

#include "CPP_Helper.h"
#include "SystemError.h"

#include <stdint.h>
#include <stddef.h>

class SocketError : public SystemError
{
public:
	using Base = SystemError;

	enum class ErrorCode : uint32_t
	{
		NO_ERR = 0,			//无错误

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

protected:
	static ErrorCode MapSocketError(uint32_t u32ErrorCode);

public:
	using Base::Base;
	using Base::operator=;

	ErrorCode GetSocketErrorCode(void) const noexcept
	{
		return MapSocketError(Base::u32ErrorCode);
	}
};

class TcpSocket
{
private:
	class SockInit
	{
	private:
		SocketError e;
	public:
		SockInit(void);
		~SockInit(void);

		DELETE_CPMV(SockInit);

		SocketError GetError() const noexcept
		{
			return e;
		}
	};

	//全局单次初始化保证，线程安全保证，程序生命周期结束析构保证
	static inline const SockInit sockInit{};

public:
	enum class ShutdownType : int
	{
		RECEIVE = 0, SEND = 1, BOTH = 2
	};

	static SocketError GetSockInitError(void) noexcept
	{
		return sockInit.GetError();
	}

	using SOCKET_T = void *;

private:
	SOCKET_T socketData;
	SocketError socketError;

protected:
	//直接从内部成员对象构造
	TcpSocket(SOCKET_T _socketData, SocketError _socketError) :
		socketData(_socketData),
		socketError(socketError)
	{}

	static SOCKET_T GetUnInitSocket(void) noexcept;

public:
	void Clear(void) noexcept
	{
		if (!IsValid())
		{
			Close();
		}

		socketError.Clear();
	}

	TcpSocket(void) noexcept :
		socketData(GetUnInitSocket()),
		socketError()
	{}

	~TcpSocket(void) noexcept
	{
		Clear();
	}

	TcpSocket(TcpSocket &&_Move) noexcept :
		socketData(_Move.socketData),
		socketError(std::move(_Move.socketError))
	{
		_Move.socketData = GetUnInitSocket();
	}

	TcpSocket &operator=(TcpSocket &&_Move) noexcept
	{
		Clear();

		socketData = _Move.socketData;
		socketError = std::move(_Move.socketError);

		_Move.socketData = GetUnInitSocket();

		return *this;
	}

	DELETE_COPY(TcpSocket);
	GETTER_COPY(SocketError, socketError);
	GETTER_COPY(SocketRaw, socketData);

	bool Open(void) noexcept;

	bool Close(void) noexcept;

	bool IsValid(void) const noexcept;

	explicit operator bool(void) const noexcept
	{
		return IsValid();
	}

	bool Connect(uint16_t u16ServerPort, uint32_t u32ServerAddr) noexcept;

	bool Bind(uint16_t u16ServerPort, uint32_t u32ServerAddr) noexcept;

	bool Listen(int32_t i32MaxPendingConnections) noexcept;

	bool Accept(TcpSocket &socketClient, uint16_t &u16ClientPort, uint32_t &u32ClientAddr) noexcept;

	bool Shutdown(ShutdownType enShutdownType) noexcept;

	bool SendPartial(const void *pDataBuffer, uint32_t &u32BufferSize) noexcept;

	bool SendAll(const void *pDataBuffer, uint32_t u32BufferSize, bool &bClientClosed) noexcept;

	bool RecvPartial(void *pDataBuffer, uint32_t &u32BufferSize) noexcept;

	bool RecvAll(void *pDataBuffer, uint32_t u32BufferSize, bool &bClientClosed) noexcept;
};
