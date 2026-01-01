#pragma once

#if defined(_WIN32)
#include "Windows_Socket.h"
#elif defined(__linux__)
//TODO
#endif

#include "CPP_Helper.h"

class Socket
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
	enum class State : int32_t
	{
		CREATED,        // Socket已创建
		CONNECTED,      // 连接已建立
		DATA_EXCHANGE,  // 数据交换中
		PEER_CLOSING,   // 对方正在关闭
		PEER_CLOSED,    // 对方已关闭
		LOCAL_CLOSING,  // 本地正在关闭
		CLOSED,         // 连接已关闭
		ERROR           // 错误状态
	};

private:
	SOCKET_T socketData;
	int32_t i32ErrCode;
	State state;//TODO
	
protected:
	Socket(SOCKET_T _socketData, int32_t _i32ErrCode = 0) :socketData(_socketData), i32ErrCode(_i32ErrCode)
	{}
public:
	Socket(void) noexcept : socketData(GetUnInitSocket()), i32ErrCode(0)
	{}

	~Socket(void) noexcept
	{
		Close();
	}

	Socket(Socket &&_Move) noexcept :
		socketData(_Move.socketData),
		i32ErrCode(_Move.i32ErrCode)
	{
		_Move.socketData = GetUnInitSocket();
		_Move.i32ErrCode = 0;
	}

	Socket &operator=(Socket &&_Move) noexcept
	{
		socketData = _Move.socketData;
		i32ErrCode = _Move.i32ErrCode;

		_Move.socketData = GetUnInitSocket();
		_Move.i32ErrCode = 0;
	}

	DELETE_COPY(Socket);

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

	bool Bind(uint16_t u16ServerPort, uint32_t u32ServerAddr) noexcept
	{
		return BindSocket(socketData, u16ServerPort, u32ServerAddr, i32ErrCode);
	}

	bool Listen(int32_t i32MaxPendingConnections) noexcept
	{
		return ListenSocket(socketData, i32MaxPendingConnections, i32ErrCode);
	}

	bool Accept(Socket &socketClient, uint16_t &u16ClientPort, uint32_t &u32ClientAddr) noexcept
	{
		return AcceptSocket(socketData, socketClient.socketData, u16ClientPort, u32ClientAddr, i32ErrCode);
	}





};







