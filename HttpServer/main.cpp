#include "Windows_Socket.h"
#include "MyAssert.hpp"

#include <stdio.h>
#include <string>


constexpr const char rsp[] =
R"(
HTTP/1.1 200 OK
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
</html>

)";



int main(void)
{
	MyAssert(Startup(), "Startup Error");

	SOCKET_T sock{};
	int32_t i32Errcode{};

	MyAssert(OpenSocket(sock, i32Errcode), "OpenSocket ErrCode: %d", i32Errcode);

	MyAssert(BindSocket(sock, 25565, 0, i32Errcode), "BindSocket ErrCode: %d", i32Errcode);
	MyAssert(ListenSocket(sock, 2, i32Errcode), "ListenSocket ErrCode: %d", i32Errcode);

	while (true)
	{
		printf("Listening...\n");

		SOCKET_T sockclient{};
		uint16_t u16ClientPort{};
		uint32_t u32ClientAddr{};
		MyAssert(AcceptSocket(sock, sockclient, u16ClientPort, u32ClientAddr, i32Errcode), "AcceptSocket ErrCode: %d", i32Errcode);

		printf("Client Connection: [%d.%d.%d.%d]:[%d]\n", 
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
			int32_t i32BufferSize = RECV_SIZE;
			bool b = RecvData(sockclient, charArrRecvData, i32BufferSize, i32Errcode);
			if (!b)
			{
				printf("Recv Error: %d\n", i32Errcode);
				MyAssert(CloseSocket(sockclient, i32Errcode), "CloseSocket ErrCode: %d", i32Errcode);
				break;
			}

			if (i32BufferSize == 0)
			{
				printf("Client Disconnect!\n");
				break;
			}

			printf("Recv Data: [\n");
			MyAssert(fwrite(charArrRecvData, sizeof(charArrRecvData[0]), i32BufferSize, stdout) == i32BufferSize);
			printf("] Recv End\n\n");

			i32BufferSize = sizeof(rsp) - 1;
			b = SendData(sockclient, rsp, i32BufferSize, i32Errcode);
			if (!b)
			{
				printf("Send Error: %d\n", i32Errcode);
				MyAssert(CloseSocket(sockclient, i32Errcode), "CloseSocket ErrCode: %d", i32Errcode);
				break;
			}
		}
	}

	MyAssert(CloseSocket(sock, i32Errcode), "CloseSocket ErrCode: %d", i32Errcode);
	MyAssert(Cleanup(), "Cleanup Error");
	return 0;
}
