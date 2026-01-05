#include <stdint.h>
#include <stddef.h>

using SOCKET_T = void *;
SOCKET_T GetUnInitSocket(void);

enum class SocketError :uint8_t
{
	NO_ERR = 0,		//无错误

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

SocketError MapSocketError(int32_t i32ErrCode);

bool Startup(void);
bool Cleanup(void);

bool OpenSocket(SOCKET_T &socketOpen, int32_t &i32ErrorCode);
bool CloseSocket(SOCKET_T &socketClose, int32_t &i32ErrorCode);

bool ConnectSocket(SOCKET_T socketConnect, uint16_t u16ServerPort, uint32_t u32ServerAddr, int32_t &i32ErrorCode);
bool BindSocket(SOCKET_T socketBind, uint16_t u16ServerPort, uint32_t u32ServerAddr, int32_t &i32ErrorCode);
bool ListenSocket(SOCKET_T socketListen, int32_t i32MaxPendingConnections, int32_t &i32ErrorCode);
bool AcceptSocket(SOCKET_T socketAccept, SOCKET_T &socketClient, uint16_t &u16ClientPort, uint32_t &u32ClientAddr, int32_t &i32ErrorCode);

enum class ShutdownType : int { RECEIVE = 0, SEND = 1, BOTH = 2 };
bool ShutdownSocket(SOCKET_T socketShutdown, ShutdownType enShutdownType, int32_t &i32ErrorCode);

bool SendDataPartial(SOCKET_T socketSend, const void *pDataBuffer, int32_t &i32BufferSize, int32_t &i32ErrorCode);
bool SendDataAll(SOCKET_T socketSend, const void *pDataBuffer, int32_t i32BufferSize, bool &bClientClose, int32_t &i32ErrorCode);

bool RecvDataPartial(SOCKET_T socketRecv, void *pDataBuffer, int32_t &i32BufferSize, int32_t &i32ErrorCode);
bool RecvDataAll(SOCKET_T socketRecv, void *pDataBuffer, int32_t i32BufferSize, bool &bClientClose, int32_t &i32ErrorCode);

