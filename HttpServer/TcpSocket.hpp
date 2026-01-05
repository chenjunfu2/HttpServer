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
		bool bInit;
	public:
		SockInit(void) : bInit(Startup())
		{}
		~SockInit(void)
		{
			Cleanup();
		}

		DELETE_CPMV(SockInit);

		bool GetInit() const noexcept
		{
			return bInit;
		}
	};

	//全局单次初始化保证，线程安全保证，程序生命周期结束析构保证
	static inline const SockInit sockInit{};

public:


private:
	SOCKET_T socketData;
	SocketError socketError;
	
protected:
	TcpSocket(SOCKET_T _socketData, Error _stateSocket, int32_t _i32ErrCode = 0) :
		socketData(_socketData),
		stateSocket(_stateSocket),
		i32ErrCode(_i32ErrCode)
	{}

public:
	TcpSocket(void) noexcept :
		socketData(GetUnInitSocket()),
		stateSocket(State::UNINIT),
		i32ErrCode(0)
	{}

	~TcpSocket(void) noexcept
	{
		Close();
	}

	TcpSocket(TcpSocket &&_Move) noexcept :
		socketData(_Move.socketData),
		stateSocket(_Move.stateSocket),
		i32ErrCode(_Move.i32ErrCode)
	{
		_Move.socketData = GetUnInitSocket();
		_Move.stateSocket = State::UNINIT;
		_Move.i32ErrCode = 0;
	}

	TcpSocket &operator=(TcpSocket &&_Move) noexcept
	{
		socketData = _Move.socketData;
		stateSocket = _Move.stateSocket;
		i32ErrCode = _Move.i32ErrCode;

		_Move.socketData = GetUnInitSocket();
		_Move.stateSocket = State::UNINIT;
		_Move.i32ErrCode = 0;
	}

	DELETE_COPY(TcpSocket);

	bool Open(void) noexcept
	{
		return OpenSocket(socketData, i32ErrCode);
	}

	bool Close(void) noexcept//CloseSocket调用失败则保留错误值
	{
		bool bRet = true;

		if (socketData != GetUnInitSocket())
		{
			bRet = CloseSocket(socketData, i32ErrCode);
			socketData = GetUnInitSocket();//不论是否成功，一律销毁
		}

		if (bRet)
		{
			i32ErrCode = 0;
		}

		return bRet;
	}

	bool IsValid(void) const noexcept
	{
		return socketData != GetUnInitSocket();
	}

	operator bool(void) const noexcept
	{
		return IsValid();
	}

	int32_t GetErrCode(void) const noexcept
	{
		return i32ErrCode;
	}

	bool Connect(uint16_t u16ServerPort, uint32_t u32ServerAddr) noexcept
	{
		return ConnectSocket(socketData, u16ServerPort, u32ServerAddr, i32ErrCode);
	}

	bool Bind(uint16_t u16ServerPort, uint32_t u32ServerAddr) noexcept
	{
		return BindSocket(socketData, u16ServerPort, u32ServerAddr, i32ErrCode);
	}

	bool Listen(int32_t i32MaxPendingConnections) noexcept
	{
		return ListenSocket(socketData, i32MaxPendingConnections, i32ErrCode);
	}

	bool Accept(TcpSocket &socketClient, uint16_t &u16ClientPort, uint32_t &u32ClientAddr) noexcept
	{
		return AcceptSocket(socketData, socketClient.socketData, u16ClientPort, u32ClientAddr, i32ErrCode);
	}

	bool Shutdown(ShutdownType enShutdownType) noexcept
	{
		return ShutdownSocket(socketData, enShutdownType, i32ErrCode);
	}



};







