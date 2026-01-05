#include "Windows_TcpSocket.h"

#include <WinSock2.h>
#include <Windows.h>

#pragma comment(lib ,"ws2_32.lib")

SOCKET_T GetUnInitSocket(void)
{
	return (SOCKET_T)INVALID_SOCKET;
}

SocketError::SocketErrCode SocketError::MapSocketError(uint32_t u32ErrorCode)
{
	switch (u32ErrorCode)
	{
	case NO_ERROR:				return SocketErrCode::NO_ERR;

	case WSANOTINITIALISED:		return SocketErrCode::NOT_INITIALIZED;
	case WSAENOTSOCK:			return SocketErrCode::NOT_SOCKET;
	case WSAEINPROGRESS:		return SocketErrCode::IN_PROGRESS;

	case WSA_INVALID_HANDLE:	return SocketErrCode::INVALID_HANDLE;
	case WSA_INVALID_PARAMETER:	return SocketErrCode::INVALID_PARAMETER;

	case WSA_NOT_ENOUGH_MEMORY:	return SocketErrCode::NO_MEMORY;
	case WSAENOBUFS:			return SocketErrCode::NO_BUFFER;

	case WSAEMFILE:				return SocketErrCode::SOCKET_LIMIT;
	case WSAEMSGSIZE:			return SocketErrCode::MESSAGE_TOOLONG;

	case WSAEACCES:				return SocketErrCode::ACCESS_DENIED;
	case WSAEINTR:				return SocketErrCode::CALL_INTERRUPTED;
	case WSAEOPNOTSUPP:			return SocketErrCode::OP_NOSUPPORTED;

	case WSAEFAULT:				return SocketErrCode::ADDR_FAULT;
	case WSAEINVAL:				return SocketErrCode::PARAM_FAULT;

	case WSAEADDRINUSE:			return SocketErrCode::ADDR_INUSE;
	case WSAEADDRNOTAVAIL:		return SocketErrCode::ADDR_NOTAVAIL;

	case WSAEISCONN:			return SocketErrCode::IS_CONNECTED;
	case WSAENOTCONN:			return SocketErrCode::NO_CONNECTED;

	case WSAENETDOWN:			return SocketErrCode::NET_DOWN;
	case WSAENETUNREACH:		return SocketErrCode::NET_UNREACH;
	case WSAENETRESET:			return SocketErrCode::NET_RESET;

	case WSAESHUTDOWN:			return SocketErrCode::CONNECT_SHUTDOWN;
	case WSAECONNABORTED:		return SocketErrCode::CONNECT_ABORTED;
	case WSAECONNRESET:			return SocketErrCode::CONNECT_RESET;
	case WSAETIMEDOUT:			return SocketErrCode::CONNECT_TIMEDOUT;
	case WSAECONNREFUSED:		return SocketErrCode::CONNECT_REFUSED;

	case WSAEHOSTDOWN:			return SocketErrCode::HOST_DOWN;
	case WSAEHOSTUNREACH:		return SocketErrCode::HOST_UNREACH;

	default:					return SocketErrCode::OTHER_ERR;
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

ErrMessage GetErrorMessage(uint32_t u32ErrCode)
{
	void *pMsg = NULL;
	DWORD dRet = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, NULL, LANG_USER_DEFAULT, (LPSTR)&pMsg, 0, NULL);
	
	//如果函数失败，则返回值为零，设置大小为错误码，并设置消息为NULL以便识别特殊问题
	return dRet == 0 ? ErrMessage(NULL, (size_t)GetLastError()) : ErrMessage((const char *)pMsg, (size_t)dRet);	
}

bool Startup(void)
{
	WSADATA wsaData{};

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		return false;
	}

	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		WSACleanup();//不可接受的wsa版本，清理资源
		return false;
	}

	return true;
}

bool Cleanup(void)
{
	return WSACleanup() == 0;
}

bool OpenSocket(SOCKET_T &socketOpen, uint32_t &u32ErrorCode)
{
	SOCKET socketNew = socket
	(
		AF_INET,	//IPV4
		SOCK_STREAM,//TCP
		0			//Default protocol
	);

	if (socketNew == INVALID_SOCKET)
	{
		u32ErrorCode = WSAGetLastError();
		return false;
	}

	socketOpen = (SOCKET_T)socketNew;

	return true;
}

bool CloseSocket(SOCKET_T &socketClose, uint32_t &u32ErrorCode)
{
	if (closesocket((SOCKET)socketClose) != 0)
	{
		u32ErrorCode = WSAGetLastError();
		return false;
	}

	socketClose = NULL;
	return true;
}

bool ConnectSocket(SOCKET_T socketConnect, uint16_t u16ServerPort, uint32_t u32ServerAddr, uint32_t &u32ErrorCode)
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
		u32ErrorCode = WSAGetLastError();
		return false;
	}

	return true;
}

bool BindSocket(SOCKET_T socketBind, uint16_t u16ServerPort, uint32_t u32ServerAddr, uint32_t &u32ErrorCode)
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
		u32ErrorCode = WSAGetLastError();
		return false;
	}

	return true;
}

bool ListenSocket(SOCKET_T socketListen, uint32_t u32MaxPendingConnections, uint32_t &u32ErrorCode)
{
	if (listen((SOCKET)socketListen, u32MaxPendingConnections) != 0)
	{
		u32ErrorCode = WSAGetLastError();
		return false;
	}

	return true;
}

bool AcceptSocket(SOCKET_T socketAccept, SOCKET_T &socketClient, uint16_t &u16ClientPort, uint32_t &u32ClientAddr, uint32_t &u32ErrorCode)
{
	SOCKADDR_IN sockClientInfo{};

	int addrlen = sizeof(sockClientInfo);
	SOCKET socketNew = accept((SOCKET)socketAccept, (sockaddr *)&sockClientInfo, &addrlen);

	if (socketNew == INVALID_SOCKET)
	{
		u32ErrorCode = WSAGetLastError();
		return false;
	}

	socketClient = (SOCKET_T)socketNew;
	u16ClientPort = ntohs(sockClientInfo.sin_port);
	u32ClientAddr = ntohl(sockClientInfo.sin_addr.s_addr);

	return true;
}

bool ShutdownSocket(SOCKET_T socketShutdown, SocketShutdown enSocketShutdown, uint32_t &u32ErrorCode)
{
	if (shutdown((SOCKET)socketShutdown, (int)enSocketShutdown) != 0)
	{
		u32ErrorCode = WSAGetLastError();
		return false;
	}

	return true;
}

bool SendDataPartial(SOCKET_T socketSend, const void *pDataBuffer, uint32_t &u32BufferSize, uint32_t &u32ErrorCode)
{
	int iSendSize = send((SOCKET)socketSend, (const char *)pDataBuffer, u32BufferSize, 0);
	if (iSendSize < 0)
	{
		u32ErrorCode = WSAGetLastError();
		return false;
	}

	u32BufferSize = iSendSize;
	return true;
}

bool SendDataAll(SOCKET_T socketSend, const void *pDataBuffer, uint32_t u32BufferSize, bool &bClientClose, uint32_t &u32ErrorCode)
{
	while (true)
	{
		int iSendSize = send((SOCKET)socketSend, (const char *)pDataBuffer, u32BufferSize, 0);
		if (iSendSize < 0)
		{
			u32ErrorCode = WSAGetLastError();
			return false;
		}
		else if (iSendSize == 0)
		{
			bClientClose = true;
			break;
		}

		pDataBuffer = (const void *)((const char *)pDataBuffer + iSendSize);
		u32BufferSize -= iSendSize;

		if (u32BufferSize <= 0)
		{
			break;
		}
	}
	
	return true;
}

bool RecvDataPartial(SOCKET_T socketRecv, void *pDataBuffer, uint32_t &u32BufferSize, uint32_t &u32ErrorCode)
{
	int iRecvSize = recv((SOCKET)socketRecv, (char *)pDataBuffer, u32BufferSize, 0);
	if (iRecvSize < 0)
	{
		u32ErrorCode = WSAGetLastError();
		return false;
	}

	u32BufferSize = iRecvSize;
	return true;
}

bool RecvDataAll(SOCKET_T socketRecv, void *pDataBuffer, uint32_t u32BufferSize, bool &bClientClose, uint32_t &u32ErrorCode)
{
	while (true)
	{
		int iRecvSize = recv((SOCKET)socketRecv, (char *)pDataBuffer, u32BufferSize, 0);
		if (iRecvSize < 0)
		{
			u32ErrorCode = WSAGetLastError();
			return false;
		}
		else if (iRecvSize == 0)
		{
			bClientClose = true;
			break;
		}

		pDataBuffer = (void *)((char *)pDataBuffer + iRecvSize);
		u32BufferSize -= iRecvSize;

		if (u32BufferSize <= 0)
		{
			break;
		}
	}

	return true;
}




