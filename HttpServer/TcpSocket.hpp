#pragma once

#if defined(_WIN32)
#include "Windows_TcpSocket.h"
#elif defined(__linux__)
//TODO
#endif

#include "CPP_Helper.h"

class TcpSocket
{
private:
	class SockInit
	{
	private:
		SocketError e;
	public:
		SockInit(void) : e(Startup())
		{}
		~SockInit(void)
		{
			Cleanup();
		}

		DELETE_CPMV(SockInit);

		SocketError GetError() const noexcept
		{
			return e;
		}
	};

	//全局单次初始化保证，线程安全保证，程序生命周期结束析构保证
	static inline const SockInit sockInit{};

public:
	static SocketError GetSockInitError(void) noexcept
	{
		return sockInit.GetError();
	}

private:
	SOCKET_T socketData;
	SocketError socketError;
	
protected:
	//直接从内部成员对象构造
	TcpSocket(SOCKET_T _socketData, SocketError _socketError) :
		socketData(_socketData),
		socketError(socketError)
	{}

public:
	TcpSocket(void) noexcept :
		socketData(GetUnInitSocket()),
		socketError()
	{}

	~TcpSocket(void) noexcept
	{
		Close();
	}

	TcpSocket(TcpSocket &&_Move) noexcept :
		socketData(_Move.socketData),
		socketError(std::move(_Move.socketError))
	{
		_Move.socketData = GetUnInitSocket();
	}

	TcpSocket &operator=(TcpSocket &&_Move) noexcept
	{
		socketData = _Move.socketData;
		socketError = std::move(_Move.socketError);

		_Move.socketData = GetUnInitSocket();
	}

	DELETE_COPY(TcpSocket);
	GETTER_COPY(GetSocketError, socketError);

	bool Open(void) noexcept
	{
		return !(socketError = OpenSocket(socketData));
	}

	bool Close(void) noexcept//CloseSocket调用失败则保留错误值
	{
		if (socketData != GetUnInitSocket())
		{
			socketError = CloseSocket(socketData);
			socketData = GetUnInitSocket();//不论是否成功，一律销毁
		}

		if (!socketError)
		{
			socketError = SocketError{};
		}

		return !socketError;
	}

	bool IsValid(void) const noexcept
	{
		return socketData != GetUnInitSocket();
	}

	explicit operator bool(void) const noexcept
	{
		return IsValid();
	}

	bool Connect(uint16_t u16ServerPort, uint32_t u32ServerAddr) noexcept
	{
		return !(socketError = ConnectSocket(socketData, u16ServerPort, u32ServerAddr));
	}

	bool Bind(uint16_t u16ServerPort, uint32_t u32ServerAddr) noexcept
	{
		return !(socketError = BindSocket(socketData, u16ServerPort, u32ServerAddr));
	}

	bool Listen(int32_t i32MaxPendingConnections) noexcept
	{
		return !(socketError = ListenSocket(socketData, i32MaxPendingConnections));
	}

	bool Accept(TcpSocket &socketClient, uint16_t &u16ClientPort, uint32_t &u32ClientAddr) noexcept
	{
		return !(socketError = AcceptSocket(socketData, socketClient.socketData, u16ClientPort, u32ClientAddr));
	}

	bool Shutdown(SocketShutdown enSocketShutdown) noexcept
	{
		return !(socketError = ShutdownSocket(socketData, enSocketShutdown));
	}

	bool SendPartial(const void *pDataBuffer, uint32_t &u32BufferSize) noexcept
	{
		return !(socketError = SocketSendPartial(socketData, pDataBuffer, u32BufferSize));
	}

	bool SendAll(const void *pDataBuffer, uint32_t u32BufferSize, bool &bClientClosed) noexcept
	{
		return !(socketError = SocketSendAll(socketData, pDataBuffer, u32BufferSize, bClientClosed));
	}

	bool RecvPartial(void *pDataBuffer, uint32_t &u32BufferSize) noexcept
	{
		return !(socketError = SocketRecvPartial(socketData, pDataBuffer, u32BufferSize));
	}

	bool RecvAll(void *pDataBuffer, uint32_t u32BufferSize, bool &bClientClosed) noexcept
	{
		return !(socketError = SocketRecvAll(socketData, pDataBuffer, u32BufferSize, bClientClosed));
	}
};


