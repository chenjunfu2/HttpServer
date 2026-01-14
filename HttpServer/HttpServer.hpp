#pragma once

#include "Windows_TcpSocket.h"
#include "VirtualFileSystem.hpp"


class HttpRequest
{
public:
	enum class Method
	{
		UNKNOWN = 0,	//未知请求
		GET,			//获取数据，包括响应头和响应体
		HEAD,			//获取数据，仅响应头
	};

	enum class ConnectionType
	{
		UNKNOWN = 0,
		KEEP_ALIVE,
		CLOSE,
	};

	class StateContext
	{
		friend class HttpRequest;
	public:
		enum class ParseState
		{
			READY = 0,		//就绪状态

			METHOD,			//解析方法
			PATH,			//解析路径
			VERSION,		//解析版本

			HEADER_KEY,		//解析header键
			HEADER_VAL,		//解析header值
			HEADER_END,		//解析header行结束

			BODY,			//解析body

			COMPLETE,		//解析完成（两个连续的HEADER_END即为结束）

			ERROR,			//解析错误
		};

	protected:
		ParseState enParseState = ParseState::READY;

		size_t szContentLength = 0;
		size_t szHeaderSize = 0;

		std::string strCurHeaderKey{};
		std::string strCurHeaderVal{};

	protected:
		void Clear(void) noexcept
		{
			enParseState = ParseState::READY;

			szContentLength = 0;
			szHeaderSize = 0;

			strCurHeaderKey.clear();
			strCurHeaderVal.clear();
		}

		bool TransLateState(ParseState enNextState)
		{
			switch (enParseState)
			{
			case ParseState::READY:
				if (enNextState != ParseState::METHOD)
				{
					return false;
				}
				break;
			case ParseState::METHOD:
				if (enNextState != ParseState::PATH)
				{
					return false;
				}
				break;
			case ParseState::PATH:
				if (enNextState != ParseState::VERSION)
				{
					return false;
				}
				break;
			case ParseState::VERSION:
				if (enNextState != ParseState::HEADER_KEY)//下一个必然要是HEADER_KEY，因为请求头至少包含一个Host，如若没有，则错误
				{
					return false;
				}
				break;
			case ParseState::HEADER_KEY:
				if (enNextState != ParseState::HEADER_VAL)
				{
					return false;
				}
				break;
			case ParseState::HEADER_VAL:
				if (enNextState != ParseState::HEADER_END)//下一个必须是结束，http1.1起禁止多行值
				{
					return false;
				}
				break;
			case ParseState::HEADER_END:
				if (enNextState != ParseState::HEADER_KEY && enNextState != ParseState::BODY)
				{
					return false;
				}
				break;
			case ParseState::BODY:
				if (enNextState != ParseState::COMPLETE)
				{
					return false;
				}
				break;
			case ParseState::COMPLETE://成功后不允许进行任何转换
			case ParseState::ERROR://出错后不允许进行任何转换
			default://未知枚举异常
				{
					return false;
				}
				break;
			}

			//执行到此，转换成功，进行赋值
			enParseState = enNextState;
			return true;
		}


	public:
		DELETE_COPY(StateContext);
		DEFAULT_MOVE(StateContext);

		GETTER_COPY(ParseState, enParseState);

		GETTER_COPY(ContentLength, szContentLength);
		GETTER_COPY(HeaderSize, szHeaderSize);

		GETTER_CREF(CurHeaderKey, strCurHeaderKey);
		GETTER_CREF(CurHeaderVal, strCurHeaderVal);
	};

private:
	Method enMethod = Method::UNKNOWN;
	ConnectionType enConnectionType = ConnectionType::UNKNOWN;

	std::string strPath{};
	std::string strVersion{};
	std::string strHost{};
	std::string strBody{};

	std::unordered_map<std::string, std::string> mapHeaders{};

public:
	void Clear(void) noexcept
	{
		enMethod = Method::UNKNOWN;
		enConnectionType = ConnectionType::UNKNOWN;

		strPath.clear();
		strVersion.clear();
		strHost.clear();
		strBody.clear();

		mapHeaders.clear();
	}





};

class HttpResponse
{
public:
	enum class Status : uint32_t
	{
		OK								= 200,	//请求成功

		NOT_MODIFIED					= 304,	//之前请求的内容未改变，无需重复发送

		BAD_REQUEST						= 400,	//非法的请求，服务端拒绝处理
		FORBIDDEN						= 403,	//没有访问权限
		NOT_FOUND						= 404,	//未找到请求的资源
		METHOD_NOT_ALLOWED				= 405,	//不支持、禁止的请求方法
		REQUEST_TIMEOUT					= 408,	//请求超时（服务器关闭无用链接）

		CONTENT_TOO_LARGE				= 413,	//请求体过大（也可以叫PAYLOAD_TOO_LARGE）
		URI_TOO_LONG					= 414,	//请求URL过长
		TOO_MANY_REQUESTS				= 429,	//短时间发送过多请求
		REQUEST_HEADER_FIELDS_TOO_LARGE	= 431,	//请求头过大

		INTERNAL_ERROR					= 500,	//服务器意外
		NOT_IMPLEMENTED					= 501,	//不支持的请求方法
		SERVICE_UNAVAILABLE				= 503,	//服务器维护
		VERSION_NOT_SUPPORTED			= 505,	//不支持的http版本
		INSUFFICIENT_STORAGE			= 507,	//空间不足
	};

private:





};

class Connection
{

};


class HttpServer
{
private:
	size_t szMaxHeaderSize;//请求头最大值
	size_t szMaxBodySize;//请求体最大值



public:
	DELETE_CPMV(HttpServer);













};





