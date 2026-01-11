#include "Windows_TcpSocket.h"
#include "MyAssert.hpp"

#include "TcpSocket.hpp"

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

#define CALL_FUNC_ASSERT(func_name,...)\
do\
{\
	SocketError e = func_name(__VA_ARGS__);\
	if (e.IsError())\
	{\
		MyAssert(false, "[" #func_name "] Error [%d]: %s", e.GetSysErrorCode(), e.ToErrMessage().GetStrView().data());\
	}\
} while (0)

int main(void)
{
	//先设置为本地化信息为用户环境变量中的默认设置
	const char *pcLocale = setlocale(LC_ALL, "");

	if (pcLocale != NULL)
	{
		std::string strLocale(pcLocale);
		strLocale.replace(strLocale.find_last_of('.') + 1, strLocale.size(), "UTF-8");
		pcLocale = setlocale(LC_ALL, strLocale.c_str());
	}

	//如果上面失败，则设置为预设值
	if (pcLocale == NULL)//注意这里不是else if，因为上面的if内部有可能重新赋值pcLocale
	{
		MyAssert(setlocale(LC_ALL, "en_US.UTF-8"), "setlocale fail");//再次失败触发assert
	}

	CALL_FUNC_ASSERT(Startup);

	SOCKET_T sock{};

	CALL_FUNC_ASSERT(OpenSocket, sock);
	CALL_FUNC_ASSERT(BindSocket, sock, BIND_PORT, 0);
	CALL_FUNC_ASSERT(ListenSocket, sock, 2);

	while (true)
	{
		printf("Listening [0.0.0.0:%d] ...\n", BIND_PORT);

		SOCKET_T sockclient{};
		uint16_t u16ClientPort{};
		uint32_t u32ClientAddr{};
		CALL_FUNC_ASSERT(AcceptSocket, sock, sockclient, u16ClientPort, u32ClientAddr);

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
			SocketError e{};

			uint32_t u32BufferSize = RECV_SIZE;
			e = SocketRecvPartial(sockclient, charArrRecvData, u32BufferSize);
			if (e)
			{
				printf("[RecvDataPartial] Error [%d]: %s\n", e.GetSysErrorCode(), e.ToErrMessage().GetStrView().data());
				CALL_FUNC_ASSERT(CloseSocket, sockclient);
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
			e = SocketSendAll(sockclient, rsp, u32BufferSize, bClientClose);
			if (e)
			{
				printf("[SendDataAll] Error [%d]: %s\n", e.GetSysErrorCode(), e.ToErrMessage().GetStrView().data());
				CALL_FUNC_ASSERT(CloseSocket, sockclient);
				break;
			}

			if (bClientClose)
			{
				printf("Connect Closed\n");
				CALL_FUNC_ASSERT(CloseSocket, sockclient);
				break;
			}
		}
	}

	CALL_FUNC_ASSERT(CloseSocket,sock);
	CALL_FUNC_ASSERT(Cleanup);
	return 0;
}
