#include "Windows_TcpSocket.h"
#include "MyAssert.hpp"

//#include "TcpSocket.hpp"

#include <stdio.h>
#include <string>


constexpr const char rsp[] =
R"(HTTP/1.1 200 OK
Content-Type: text/html; charset=utf-8
Content-Length: 183
Connection: keep-alive
Content-Language: en-US

<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <title>A simple webpage</title>
</head>
<body>
  <h1>Simple HTML webpage</h1>
  <p>Hello, world!</p>
</body>
</html>)";

#define BIND_PORT 25565

int main(void)
{
	MyAssert(Startup(), "Startup Error");

	SOCKET_T sock{};
	SocketError e{};


	MyAssert(e = OpenSocket(sock), "OpenSocket Error: %s", e.ToErrMessage().GetMsg());

	MyAssert(BindSocket(sock, BIND_PORT, 0, u32Errcode), "BindSocket ErrCode: %d", u32Errcode);
	MyAssert(ListenSocket(sock, 2, u32Errcode), "ListenSocket ErrCode: %d", u32Errcode);

	auto ret = GetErrorMessage(10054L);

	while (true)
	{
		printf("Listening [0.0.0.0:%d]...\n", BIND_PORT);

		SOCKET_T sockclient{};
		uint16_t u16ClientPort{};
		uint32_t u32ClientAddr{};
		MyAssert(AcceptSocket(sock, sockclient, u16ClientPort, u32ClientAddr, u32Errcode), "AcceptSocket ErrCode: %d", u32Errcode);

		printf("Client Connection: [%d.%d.%d.%d:%d]\n", 
			(uint8_t)((u32ClientAddr >> 3 * 8) & 0xFF),
			(uint8_t)((u32ClientAddr >> 2 * 8) & 0xFF),
			(uint8_t)((u32ClientAddr >> 1 * 8) & 0xFF),
			(uint8_t)((u32ClientAddr >> 0 * 8) & 0xFF),
			u16ClientPort
		);

		constexpr const size_t RECV_SIZE = 1024;
		char charArrRecvData[RECV_SIZE];
		while (true)
		{
			uint32_t u32BufferSize = RECV_SIZE;
			bool b = RecvDataPartial(sockclient, charArrRecvData, u32BufferSize, u32Errcode);
			if (!b)
			{
				printf("Recv Error: %d\n", u32Errcode);
				MyAssert(CloseSocket(sockclient, u32Errcode), "CloseSocket ErrCode: %d", u32Errcode);
				break;
			}

			if (u32BufferSize == 0)
			{
				printf("Client Disconnect!\n");
				break;
			}

			printf("Recv Data: [\n");
			MyAssert(fwrite(charArrRecvData, sizeof(charArrRecvData[0]), u32BufferSize, stdout) == u32BufferSize);
			printf("\n] Recv End\n\n");

			u32BufferSize = sizeof(rsp) - 1;
			bool bClientClose = false;
			b = SendDataAll(sockclient, rsp, u32BufferSize, bClientClose, u32Errcode);
			if (!b)
			{
				printf("Send Error: %d\n", u32Errcode);
				MyAssert(CloseSocket(sockclient, u32Errcode), "CloseSocket ErrCode: %d", u32Errcode);
				break;
			}

			if (bClientClose)
			{
				printf("Connect Closed\n");
				MyAssert(CloseSocket(sockclient, u32Errcode), "CloseSocket ErrCode: %d", u32Errcode);
				break;
			}
		}
	}

	MyAssert(CloseSocket(sock, u32Errcode), "CloseSocket ErrCode: %d", u32Errcode);
	MyAssert(Cleanup(), "Cleanup Error");
	return 0;
}
