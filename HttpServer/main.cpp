#include "MyAssert.hpp"

#include "Windows_TcpSocket.h"

#include "HttpServer.hpp"

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
			object_name.GetSocketError().GetErrorCode(), \
			object_name.GetSocketError().GetErrorMessage().GetStrView().data());\
} while (0)

int main(void)
{
	//设置为UTF8编码
	MyAssert(setlocale(LC_ALL, ".UTF-8") != NULL, "setlocale fail");

	TcpSocket sockServer{};
	MyAssert(!sockServer.GetSockInitError(), "Socket Init Error [%d]: %s",
		sockServer.GetSockInitError().GetErrorCode(),
		sockServer.GetSockInitError().GetErrorMessage().GetStrView().data());

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
		std::string strRecv;
		strRecv.resize(RECV_SIZE);

		HttpRequest req;
		auto ctx = req.GetNewContext(255, 1024, 0);

		while (true)
		{
			uint32_t u32BufferSize = RECV_SIZE;
			if (!sockclient.RecvPartial(strRecv.data(), u32BufferSize))
			{
				SocketError e = sockclient.GetSocketError();
				printf("[sockclient.RecvPartial()] Error [%d]: %s\n", e.GetErrorCode(), e.GetErrorMessage().GetStrView().data());
				CALL_FUNC_ASSERT(sockclient, Close);
				break;
			}

			if (u32BufferSize == 0)
			{
				printf("Client Disconnect!\n");
				break;
			}

			printf("Recv Data: [\n");
			MyAssert(fwrite(strRecv.data(), sizeof(*strRecv.data()), u32BufferSize, stdout) == u32BufferSize);
			printf("\n] Recv End\n\n");

			//请求头：收到\r\n\r\n即为结束
			//限制：1kb，如果没找到结束，请求头过大，直接拒绝
			//超时：暂时不做，偷懒ing

			size_t szCL{};
			req.ParsingStream(ctx, strRecv, szCL);

			u32BufferSize = sizeof(rsp) - 1;
			bool bClientClose = false;
			if (!sockclient.SendAll(rsp, u32BufferSize, bClientClose))
			{
				SocketError e = sockclient.GetSocketError();
				printf("[SendDataAll] Error [%d]: %s\n", e.GetErrorCode(), e.GetErrorMessage().GetStrView().data());
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


//GET / HTTP/1.1
//Host: localhost:25565
//Connection: keep-alive
//sec-ch-ua: "Google Chrome";v="143", "Chromium";v="143", "Not A(Brand";v="24"
//sec-ch-ua-mobile: ?0
//sec-ch-ua-platform: "Windows"
//Upgrade-Insecure-Requests: 1
//User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/143.0.0.0 Safari/537.36
//Sec-Purpose: prefetch;prerender
//Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7
//Sec-Fetch-Site: none
//Sec-Fetch-Mode: navigate
//Sec-Fetch-User: ?1
//Sec-Fetch-Dest: document
//Accept-Encoding: gzip, deflate, br, zstd
//Accept-Language: zh-CN,zh;q=0.9
//
//


//GET /favicon.ico HTTP/1.1
//Host: localhost:25565
//Connection: keep-alive
//sec-ch-ua-platform: "Windows"
//User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/143.0.0.0 Safari/537.36
//sec-ch-ua: "Google Chrome";v="143", "Chromium";v="143", "Not A(Brand";v="24"
//sec-ch-ua-mobile: ?0
//Accept: image/avif,image/webp,image/apng,image/svg+xml,image/*,*/*;q=0.8
//Sec-Fetch-Site: same-origin
//Sec-Fetch-Mode: no-cors
//Sec-Fetch-Dest: image
//Referer: http://localhost:25565/
//Accept-Encoding: gzip, deflate, br, zstd
//Accept-Language: zh-CN,zh;q=0.9
//
//

