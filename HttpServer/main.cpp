#include "Windows_TcpSocket.h"
#include "MyAssert.hpp"

#include "Windows_TcpSocket.h"

#include <stdio.h>
#include <string>
#include <locale.h>


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

#define CALL_FUNC_ASSERT(object_name, func_name, ...)\
do\
{\
	MyAssert(object_name.func_name(__VA_ARGS__), "[" #object_name "." #func_name "()] Error [%d]: %s",\
			object_name.GetSocketError().GetSysErrorCode(), \
			object_name.GetSocketError().ToErrorMessage().GetStrView().data());\
} while (0)

int main(void)
{
	//设置为UTF8编码
	MyAssert(setlocale(LC_ALL, ".UTF-8") != NULL, "setlocale fail");

	TcpSocket sockServer{};
	MyAssert(!sockServer.GetSockInitError(), "Socket Init Error [%d]: %s",
		sockServer.GetSockInitError().GetSysErrorCode(),
		sockServer.GetSockInitError().ToErrorMessage().GetStrView().data());


	CALL_FUNC_ASSERT(sockServer, Open);
	CALL_FUNC_ASSERT(sockServer, Bind, BIND_PORT, 0);
	CALL_FUNC_ASSERT(sockServer, Listen, 2);

	while (true)
	{
		printf("Listening [0.0.0.0:%d] ...\n", BIND_PORT);

		TcpSocket sockclient{};
		uint16_t u16ClientPort{};
		uint32_t u32ClientAddr{};
		CALL_FUNC_ASSERT(sockServer, Accept, sockclient, u16ClientPort, u32ClientAddr);

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
			if (！sockclient.RecvPartial(charArrRecvData, u32BufferSize))
			{
				SocketError e = sockclient.GetSocketError();
				printf("[sockclient.RecvPartial()] Error [%d]: %s\n", e.GetSysErrorCode(), e.ToErrorMessage().GetStrView().data());
				CALL_FUNC_ASSERT(sockclient, Close);
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
			if (!sockclient.SendAll(rsp, u32BufferSize, bClientClose))
			{
				SocketError e = sockclient.GetSocketError();
				printf("[SendDataAll] Error [%d]: %s\n", e.GetSysErrorCode(), e.ToErrorMessage().GetStrView().data());
				CALL_FUNC_ASSERT(sockclient, Close);
				break;
			}

			if (bClientClose)
			{
				printf("Connect Closed\n");
				CALL_FUNC_ASSERT(sockclient, Close);
				break;
			}
		}
	}

	CALL_FUNC_ASSERT(sockServer, Close);
	return 0;
}
