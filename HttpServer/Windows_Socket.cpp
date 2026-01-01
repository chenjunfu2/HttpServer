#include "Windows_Socket.h"

#include <WinSock2.h>
#include <Windows.h>

#pragma comment(lib ,"ws2_32.lib")

SOCKET_T GetUnInitSocket(void)
{
	return (SOCKET_T)INVALID_SOCKET;
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

bool OpenSocket(SOCKET_T &socketOpen, int32_t &i32ErrorCode)
{
	SOCKET socketNew = socket
	(
		AF_INET,	//IPV4
		SOCK_STREAM,//TCP
		0			//Default protocol
	);

	if (socketNew == INVALID_SOCKET)
	{
		i32ErrorCode = WSAGetLastError();
		return false;
	}

	socketOpen = (SOCKET_T)socketNew;

	return true;
}

bool CloseSocket(SOCKET_T &socketClose, int32_t &i32ErrorCode)
{
	if (closesocket((SOCKET)socketClose) != 0)
	{
		i32ErrorCode = WSAGetLastError();
		return false;
	}

	socketClose = NULL;
	return true;
}

bool BindSocket(SOCKET_T socketBind, uint16_t u16ServerPort, uint32_t u32ServerAddr, int32_t &i32ErrorCode)
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
		i32ErrorCode = WSAGetLastError();
		return false;
	}

	return true;
}

bool ListenSocket(SOCKET_T socketListen, int32_t i32MaxPendingConnections, int32_t &i32ErrorCode)
{
	if (listen((SOCKET)socketListen, i32MaxPendingConnections) != 0)
	{
		i32ErrorCode = WSAGetLastError();
		return false;
	}

	return true;
}

bool AcceptSocket(SOCKET_T socketAccept, SOCKET_T &socketClient, uint16_t &u16ClientPort, uint32_t &u32ClientAddr, int32_t &i32ErrorCode)
{
	SOCKADDR_IN sockClientInfo{};

	int addrlen = sizeof(sockClientInfo);
	SOCKET socketNew = accept((SOCKET)socketAccept, (sockaddr *)&sockClientInfo, &addrlen);

	if (socketNew == INVALID_SOCKET)
	{
		i32ErrorCode = WSAGetLastError();
		return false;
	}

	socketClient = (SOCKET_T)socketNew;
	u16ClientPort = ntohs(sockClientInfo.sin_port);
	u32ClientAddr = ntohl(sockClientInfo.sin_addr.s_addr);

	return true;
}

bool SendDataPartial(SOCKET_T socketSend, const void *pDataBuffer, int32_t &i32BufferSize, int32_t &i32ErrorCode)
{
	int iSendSize = send((SOCKET)socketSend, (const char *)pDataBuffer, i32BufferSize, 0);
	if (iSendSize < 0)
	{
		i32ErrorCode = WSAGetLastError();
		return false;
	}

	i32BufferSize = iSendSize;
	return true;
}

bool SendDataAll(SOCKET_T socketSend, const void *pDataBuffer, int32_t i32BufferSize, bool &bClientClose, int32_t &i32ErrorCode)
{
	while (true)
	{
		int iSendSize = send((SOCKET)socketSend, (const char *)pDataBuffer, i32BufferSize, 0);
		if (iSendSize < 0)
		{
			i32ErrorCode = WSAGetLastError();
			return false;
		}
		else if (iSendSize == 0)
		{
			bClientClose = true;
			break;
		}

		pDataBuffer = (const void *)((const char *)pDataBuffer + iSendSize);
		i32BufferSize -= iSendSize;

		if (i32BufferSize <= 0)
		{
			break;
		}
	}
	
	return true;
}

bool RecvDataPartial(SOCKET_T socketRecv, void *pDataBuffer, int32_t &i32BufferSize, int32_t &i32ErrorCode)
{
	int iRecvSize = recv((SOCKET)socketRecv, (char *)pDataBuffer, i32BufferSize, 0);
	if (iRecvSize < 0)
	{
		i32ErrorCode = WSAGetLastError();
		return false;
	}

	i32BufferSize = iRecvSize;
	return true;
}

bool RecvDataAll(SOCKET_T socketRecv, void *pDataBuffer, int32_t i32BufferSize, bool &bClientClose, int32_t &i32ErrorCode)
{
	while (true)
	{
		int iRecvSize = recv((SOCKET)socketRecv, (char *)pDataBuffer, i32BufferSize, 0);
		if (iRecvSize < 0)
		{
			i32ErrorCode = WSAGetLastError();
			return false;
		}
		else if (iRecvSize == 0)
		{
			bClientClose = true;
			break;
		}

		pDataBuffer = (void *)((char *)pDataBuffer + iRecvSize);
		i32BufferSize -= iRecvSize;

		if (i32BufferSize <= 0)
		{
			break;
		}
	}

	return true;
}

