#include "TcpSocket.h"

#include <WinSock2.h>
#include <Windows.h>

#pragma comment(lib ,"ws2_32.lib")

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

TcpSocket::SockInit::SockInit(void) :e()
{
	WSADATA wsaData{};

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		e = WSAGetLastError();
	}

	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		WSACleanup();//不可接受的wsa版本，清理资源
		e = WSAVERNOTSUPPORTED;//主动返回指定错误码
	}
}

TcpSocket::SockInit::~SockInit(void)
{
	if (WSACleanup() != 0)
	{
		e = WSAGetLastError();
		return;
	}

	e.Clear();
}

TcpSocket::SOCKET_T TcpSocket::GetUnInitSocket(void) noexcept
{
	return (SOCKET_T)INVALID_SOCKET;
}

bool TcpSocket::Open(void) noexcept
{
	if (IsValid())
	{
		socketError = WSAEALREADY;
		return false;
	}

	SOCKET socketNew = socket
	(
		AF_INET,	//IPV4
		SOCK_STREAM,//TCP
		0			//Default protocol
	);

	if (socketNew == INVALID_SOCKET)
	{
		socketError = WSAGetLastError();
		return false;
	}

	socketData = (SOCKET_T)socketNew;

	return true;
}

bool TcpSocket::Close(void) noexcept
{
	if (closesocket((SOCKET)socketData) != 0)
	{
		socketError = WSAGetLastError();
		return false;
	}

	socketData = GetUnInitSocket();
	return true;
}

bool TcpSocket::IsValid(void) const noexcept
{
	return socketData != GetUnInitSocket();
}

bool TcpSocket::Connect(uint16_t u16ServerPort, uint32_t u32ServerAddr) noexcept
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

	if (connect((SOCKET)socketData, (const sockaddr *)&sockServerInfo, sizeof(sockServerInfo)) != 0)
	{
		socketError = WSAGetLastError();
		return false;
	}

	return true;
}

bool TcpSocket::Bind(uint16_t u16ServerPort, uint32_t u32ServerAddr) noexcept
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

	if (bind((SOCKET)socketData, (const sockaddr *)&sockServerInfo, sizeof(sockServerInfo)) != 0)
	{
		socketError = WSAGetLastError();
		return false;
	}

	return true;
}

bool TcpSocket::Listen(int32_t i32MaxPendingConnections) noexcept
{
	if (listen((SOCKET)socketData, i32MaxPendingConnections) != 0)
	{
		socketError = WSAGetLastError();
		return false;
	}

	return true;
}

bool TcpSocket::Accept(TcpSocket &socketClient, uint16_t &u16ClientPort, uint32_t &u32ClientAddr) noexcept
{
	SOCKADDR_IN sockClientInfo{};

	int addrlen = sizeof(sockClientInfo);
	SOCKET socketNew = accept((SOCKET)socketData, (sockaddr *)&sockClientInfo, &addrlen);

	if (socketNew == INVALID_SOCKET)
	{
		socketError = WSAGetLastError();
		return false;
	}

	socketClient.socketData = (SOCKET_T)socketNew;
	socketClient.socketError.Clear();

	u16ClientPort = ntohs(sockClientInfo.sin_port);
	u32ClientAddr = ntohl(sockClientInfo.sin_addr.s_addr);

	return true;
}

bool TcpSocket::Shutdown(ShutdownType enShutdownType) noexcept
{
	if (shutdown((SOCKET)socketData, (int)enShutdownType) != 0)
	{
		socketError = WSAGetLastError();
		return false;
	}

	return true;
}

bool TcpSocket::SendPartial(const void *pDataBuffer, uint32_t &u32BufferSize) noexcept
{
	int iSendSize = send((SOCKET)socketData, (const char *)pDataBuffer, u32BufferSize, 0);
	if (iSendSize == SOCKET_ERROR)
	{
		socketError = WSAGetLastError();
		return false;
	}

	u32BufferSize = iSendSize;
	return true;
}

bool TcpSocket::SendAll(const void *pDataBuffer, uint32_t u32BufferSize, bool &bClientClosed) noexcept
{
	while (true)
	{
		int iSendSize = send((SOCKET)socketData, (const char *)pDataBuffer, u32BufferSize, 0);
		if (iSendSize == SOCKET_ERROR)
		{
			socketError = WSAGetLastError();
			return false;
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
	
	return true;
}

bool TcpSocket::RecvPartial(void *pDataBuffer, uint32_t &u32BufferSize) noexcept
{
	int iRecvSize = recv((SOCKET)socketData, (char *)pDataBuffer, u32BufferSize, 0);
	if (iRecvSize == SOCKET_ERROR)
	{
		socketError = WSAGetLastError();
		return false;
	}

	u32BufferSize = iRecvSize;
	return true;
}

bool TcpSocket::RecvAll(void *pDataBuffer, uint32_t u32BufferSize, bool &bClientClosed) noexcept
{
	while (true)
	{
		int iRecvSize = recv((SOCKET)socketData, (char *)pDataBuffer, u32BufferSize, 0);
		if (iRecvSize == SOCKET_ERROR)
		{
			socketError = WSAGetLastError();
			return false;
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

	return true;
}


