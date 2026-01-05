#include "Windows_TcpSocket.h"

#include <WinSock2.h>
#include <Windows.h>

#pragma comment(lib ,"ws2_32.lib")

SOCKET_T GetUnInitSocket(void)
{
	return (SOCKET_T)INVALID_SOCKET;
}

ErrMessage::ErrMessage(uint32_t u32ErrCode) noexcept :
	pMsg(NULL),
	szLength(0)
{
	void *pRet = NULL;
	DWORD dRet = FormatMessageA
	(
		FORMAT_MESSAGE_FROM_SYSTEM |		//从系统错误描述表中查询
		FORMAT_MESSAGE_IGNORE_INSERTS |		//忽略参数
		FORMAT_MESSAGE_ALLOCATE_BUFFER,		//自行分配字符串内存（需要后续手动释放）
		NULL,				//忽略
		u32ErrCode,			//需要查询的错误码
		LANG_USER_DEFAULT,	//用户当前语言
		(LPSTR)&pRet,		//系统分配字符串
		0,					//预分配长度（从0起）
		NULL				//忽略
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

ErrMessage::~ErrMessage(void) noexcept
{
	if (pMsg != NULL)
	{
		LocalFree((HLOCAL)pMsg);
	}

	pMsg = NULL;
	szLength = 0;
}

SocketError::ErrorCode SocketError::MapSocketError(uint32_t u32ErrorCode)
{
	switch (u32ErrorCode)
	{
	case NO_ERROR:				return ErrorCode::NO_ERR;

	case WSANOTINITIALISED:		return ErrorCode::NOT_INITIALIZED;
	case WSAENOTSOCK:			return ErrorCode::NOT_SOCKET;
	case WSAEINPROGRESS:		return ErrorCode::IN_PROGRESS;

	case WSA_INVALID_HANDLE:	return ErrorCode::INVALID_HANDLE;
	case WSA_INVALID_PARAMETER:	return ErrorCode::INVALID_PARAMETER;

	case WSA_NOT_ENOUGH_MEMORY:	return ErrorCode::NO_MEMORY;
	case WSAENOBUFS:			return ErrorCode::NO_BUFFER;

	case WSAEMFILE:				return ErrorCode::SOCKET_LIMIT;
	case WSAEMSGSIZE:			return ErrorCode::MESSAGE_TOOLONG;

	case WSAEACCES:				return ErrorCode::ACCESS_DENIED;
	case WSAEINTR:				return ErrorCode::CALL_INTERRUPTED;
	case WSAEOPNOTSUPP:			return ErrorCode::OP_NOSUPPORTED;

	case WSAEFAULT:				return ErrorCode::ADDR_FAULT;
	case WSAEINVAL:				return ErrorCode::PARAM_FAULT;

	case WSAEADDRINUSE:			return ErrorCode::ADDR_INUSE;
	case WSAEADDRNOTAVAIL:		return ErrorCode::ADDR_NOTAVAIL;

	case WSAEISCONN:			return ErrorCode::IS_CONNECTED;
	case WSAENOTCONN:			return ErrorCode::NO_CONNECTED;

	case WSAENETDOWN:			return ErrorCode::NET_DOWN;
	case WSAENETUNREACH:		return ErrorCode::NET_UNREACH;
	case WSAENETRESET:			return ErrorCode::NET_RESET;

	case WSAESHUTDOWN:			return ErrorCode::CONNECT_SHUTDOWN;
	case WSAECONNABORTED:		return ErrorCode::CONNECT_ABORTED;
	case WSAECONNRESET:			return ErrorCode::CONNECT_RESET;
	case WSAETIMEDOUT:			return ErrorCode::CONNECT_TIMEDOUT;
	case WSAECONNREFUSED:		return ErrorCode::CONNECT_REFUSED;

	case WSAEHOSTDOWN:			return ErrorCode::HOST_DOWN;
	case WSAEHOSTUNREACH:		return ErrorCode::HOST_UNREACH;

	default:					return ErrorCode::OTHER_ERR;
	}
}

SocketError Startup(void)
{
	WSADATA wsaData{};

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		return SocketError(WSAGetLastError());
	}

	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		WSACleanup();//不可接受的wsa版本，清理资源
		return SocketError(WSAVERNOTSUPPORTED);//主动返回指定错误码
	}

	return {};
}

SocketError Cleanup(void)
{
	return WSACleanup() != 0
		? SocketError(WSAGetLastError())
		: SocketError{};
}

SocketError OpenSocket(SOCKET_T &socketOpen)
{
	SOCKET socketNew = socket
	(
		AF_INET,	//IPV4
		SOCK_STREAM,//TCP
		0			//Default protocol
	);

	if (socketNew == INVALID_SOCKET)
	{
		return SocketError(WSAGetLastError());
	}

	socketOpen = (SOCKET_T)socketNew;

	return {};
}

SocketError CloseSocket(SOCKET_T &socketClose)
{
	if (closesocket((SOCKET)socketClose) != 0)
	{
		return SocketError(WSAGetLastError());
	}

	socketClose = (SOCKET_T)INVALID_SOCKET;
	return {};
}

SocketError ConnectSocket(SOCKET_T socketConnect, uint16_t u16ServerPort, uint32_t u32ServerAddr)
{
	const SOCKADDR_IN sockServerInfo
	{
		.sin_family = AF_INET,				//IPV4 - TCP/UDP
		.sin_port = htons(u16ServerPort),	//port（网络字节序）
		.sin_addr =
		{
			.S_un =
			{
				.S_addr = htonl(u32ServerAddr)//ip（网络字节序）
			}
		},
	};

	if (connect((SOCKET)socketConnect, (const sockaddr *)&sockServerInfo, sizeof(sockServerInfo)) != 0)
	{
		return SocketError(WSAGetLastError());
	}

	return {};
}

SocketError BindSocket(SOCKET_T socketBind, uint16_t u16ServerPort, uint32_t u32ServerAddr)
{
	const SOCKADDR_IN sockServerInfo
	{
		.sin_family = AF_INET,				//IPV4 - TCP/UDP
		.sin_port = htons(u16ServerPort),	//port（网络字节序）
		.sin_addr =
		{
			.S_un =
			{
				.S_addr = htonl(u32ServerAddr)//ip（网络字节序）
			}
		},
	};

	if (bind((SOCKET)socketBind, (const sockaddr *)&sockServerInfo, sizeof(sockServerInfo)) != 0)
	{
		return SocketError(WSAGetLastError());
	}

	return {};
}

SocketError ListenSocket(SOCKET_T socketListen, uint32_t u32MaxPendingConnections)
{
	if (listen((SOCKET)socketListen, u32MaxPendingConnections) != 0)
	{
		return SocketError(WSAGetLastError());
	}

	return {};
}

SocketError AcceptSocket(SOCKET_T socketAccept, SOCKET_T &socketClient, uint16_t &u16ClientPort, uint32_t &u32ClientAddr)
{
	SOCKADDR_IN sockClientInfo{};

	int addrlen = sizeof(sockClientInfo);
	SOCKET socketNew = accept((SOCKET)socketAccept, (sockaddr *)&sockClientInfo, &addrlen);

	if (socketNew == INVALID_SOCKET)
	{
		return SocketError(WSAGetLastError());
	}

	socketClient = (SOCKET_T)socketNew;
	u16ClientPort = ntohs(sockClientInfo.sin_port);
	u32ClientAddr = ntohl(sockClientInfo.sin_addr.s_addr);

	return {};
}

SocketError ShutdownSocket(SOCKET_T socketShutdown, SocketShutdown enSocketShutdown)
{
	if (shutdown((SOCKET)socketShutdown, (int)enSocketShutdown) != 0)
	{
		return SocketError(WSAGetLastError());
	}

	return {};
}

SocketError SendDataPartial(SOCKET_T socketSend, const void *pDataBuffer, uint32_t &u32BufferSize)
{
	int iSendSize = send((SOCKET)socketSend, (const char *)pDataBuffer, u32BufferSize, 0);
	if (iSendSize < 0)
	{
		return SocketError(WSAGetLastError());
	}

	u32BufferSize = iSendSize;
	return {};
}

SocketError SendDataAll(SOCKET_T socketSend, const void *pDataBuffer, uint32_t u32BufferSize, bool &bClientClosed)
{
	while (true)
	{
		int iSendSize = send((SOCKET)socketSend, (const char *)pDataBuffer, u32BufferSize, 0);
		if (iSendSize < 0)
		{
			return SocketError(WSAGetLastError());
		}
		else if (iSendSize == 0)
		{
			bClientClosed = true;
			break;
		}

		pDataBuffer = (const void *)((const char *)pDataBuffer + iSendSize);
		u32BufferSize -= iSendSize;

		if (u32BufferSize <= 0)
		{
			break;
		}
	}
	
	return {};
}

SocketError RecvDataPartial(SOCKET_T socketRecv, void *pDataBuffer, uint32_t &u32BufferSize)
{
	int iRecvSize = recv((SOCKET)socketRecv, (char *)pDataBuffer, u32BufferSize, 0);
	if (iRecvSize < 0)
	{
		return SocketError(WSAGetLastError());
	}

	u32BufferSize = iRecvSize;
	return {};
}

SocketError RecvDataAll(SOCKET_T socketRecv, void *pDataBuffer, uint32_t u32BufferSize, bool &bClientClosed)
{
	while (true)
	{
		int iRecvSize = recv((SOCKET)socketRecv, (char *)pDataBuffer, u32BufferSize, 0);
		if (iRecvSize < 0)
		{
			return SocketError(WSAGetLastError());
		}
		else if (iRecvSize == 0)
		{
			bClientClosed = true;
			break;
		}

		pDataBuffer = (void *)((char *)pDataBuffer + iRecvSize);
		u32BufferSize -= iRecvSize;

		if (u32BufferSize <= 0)
		{
			break;
		}
	}

	return {};
}

